/*
* Copyright 2023 Wargaming.net Limited
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* https://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/ 

#pragma once
#include "FluidSurfaceActor.generated.h"

class UBoxComponent;
class UDepthRenderComponent;
class FFluidSimulationRenderer;
class ALandscape;

struct FluidSurfaceDamageData
{
	FVector Location;
	float Intensity;
	float Radius;
};

struct FFluidSimulationVolume
{
	FVector SimulationOrigin;
	FVector PreviousFrameOffset1;
	FVector PreviousFrameOffset2;
	FVector PreviousFrameLocation1;
	FVector PreviousFrameLocation2;
};

struct FSynchronizeFluidSimulation
{
	int32 SynchronizeSimulationSubsteps(const float DeltaTime)
	{
		CurrentPreviewTime += DeltaTime;
		CurrentAccumulatedFrame = DeltaTime * (IterationsPerSecond + FrameThreshold);
		CurrentAccumulatedFrame += CurrentAccumulatedFrame;
		const int32 IterationsMax = IterationsRememberedMax + IterationsPerFrameMax;
		CurrentAccumulatedFrame = FMath::Min<float>(CurrentAccumulatedFrame, IterationsMax);

		const int32 AccumulatedIterationsMax =
			FMath::Min<int32>(IterationsPerFrameMax, FMath::TruncToInt(CurrentAccumulatedFrame));
		CurrentAccumulatedFrame -= AccumulatedIterationsMax;
		Iterations = FMath::Max(AccumulatedIterationsMax, 1);
		return Iterations;
	}

	int64 IncrementPass()
	{
		return PassCounter++;
	}
	int64 GetCurrentPass() const
	{
		return PassCounter;
	}
	int32 GetIterationsNum() const
	{
		return Iterations;
	}
	void SetIterationsPerSecond(int32 Value)
	{
		IterationsPerSecond = Value;
	}
	void SetIterationsRememberedMax(int32 Value)
	{
		IterationsRememberedMax = Value;
	}
	void SetIterationsPerFrameMax(int32 Value)
	{
		IterationsPerFrameMax = Value;
	}

private:
	int32 FrameThreshold = 1;
	int32 IterationsPerSecond = 60;
	int32 IterationsRememberedMax = 4;
	int32 IterationsPerFrameMax = 4;
	int32 Iterations = 1;
	int64 PassCounter = 0;
	float CurrentPreviewTime = 0.f;
	float CurrentAccumulatedFrame = 0.f;
};

UCLASS(NotBlueprintable, HideCategories = (Rendering, Replication, Collision, Actor, Cooking, "Tags", "AssetUserData", HLOD, Input, LOD))
class FLUIDSURFACES_API AFluidSurfaceActor : public AActor
{
	GENERATED_BODY()
public:
	AFluidSurfaceActor();
#if WITH_EDITOR

	UFUNCTION(CallInEditor, Category = "Settings")
	void SnapBounds();
#endif

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	TSoftObjectPtr<ALandscape> TargetLandscape;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	FVector LandscapePosition;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	FIntPoint LandscapeSize;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	FVector LandscapeScale;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	float WaterLevel = 0.f;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	float SimulationWorldSize = 20480.f;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	bool bUseCameraTransform = false;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	bool bUseSimData = true;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	float MinSimulationSpeed = 75.f;

	UPROPERTY(EditInstanceOnly, Category = "Settings")
	bool bBoxCollisionDetection = false;

	UPROPERTY(EditAnywhere, Category = "Simulation|Foam")
	float RippleFoamErasure = 0.1f;

	UPROPERTY(EditAnywhere, Category = "Simulation|Foam")
	float FoamDamping = 0.98f;

	UPROPERTY(EditAnywhere, Category = "Simulation|Fluid")
	float FluidDamping = 0.025f;

	UPROPERTY(EditAnywhere, Category = "Simulation|Fluid")
	float TravelSpeed = 0.9f;

	UPROPERTY(EditAnywhere, Category = "Simulation|Force")
	float ForceMultiplier = 10.f;

	UPROPERTY(EditAnywhere, Category = "Simulation|Force")
	float ImpulseMultiplier = 50.f;

	UPROPERTY(EditAnywhere, Category = "Simulation|Force")
	float DamageRadiusMin = 1.f;

	UPROPERTY(EditAnywhere, Category = "Simulation|Force")
	float DamageRadiusMax = 2048.f;

	UPROPERTY(EditAnywhere, Category = "Simulation|Force")
	UTexture2D* SplashTexture;

protected:
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostInitializeComponents() override;
	virtual void BeginPlay() override;
	virtual void TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction) override;
	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;
	friend class AFlowMapActor;

public:
	float GetWaterLevel() const;

private:
	UPROPERTY(VisibleAnywhere, Category = "Buffers")
	UTextureRenderTarget2D* ImpulseRT;

	UPROPERTY(VisibleAnywhere, Category = "Buffers")
	UTextureRenderTarget2D* DepthRT;

	UPROPERTY(VisibleAnywhere, Category = "Buffers")
	UTextureRenderTarget2D* VelocityRT;

	UPROPERTY(VisibleAnywhere, Category = "Buffers")
	UTextureRenderTarget2D* ForcesRT;

	UPROPERTY(VisibleAnywhere, Category = "Buffers")
	UTextureRenderTarget2D* SimulationRT;

	UPROPERTY(VisibleAnywhere, Category = "Buffers")
	TArray<UTextureRenderTarget2D*> RippleRTs;

	UPROPERTY(VisibleAnywhere, Category = "Buffers")
	UMaterialParameterCollection* FluidSimulationMPC;

	TWeakObjectPtr<UBoxComponent> SurfaceCollision;

	int32 NormalResolution = 2048;
	int32 DepthResolution = 1024;
	int32 VelocityResolution = 512;
	int32 ImpulseResolution = 1024;

	FVector SimulationOrigin;
	FVector PreviousFrameOffset1;
	FVector PreviousFrameOffset2;
	FVector PreviousFrameLocation1;
	FVector PreviousFrameLocation2;

	FSynchronizeFluidSimulation SynchronizeFluidSimulation;

	volatile bool bNeedUpdateImpulseData = false;
	volatile bool bNeedUpdateProjectileData = false;
	volatile bool bNeedUpdateForceData = false;

	TArray<FluidSurfaceDamageData> DamageData;

	bool DrawDepth(
		TSet<FPrimitiveSceneProxy*> InStaticPrimitives,
		TSet<FPrimitiveSceneProxy*> InDynamicPrimitives,
		UTextureRenderTarget2D* InDepth,
		FTransform InTransform,
		float InOrthoWidth) const;

	void DrawVelocity(
		TSet<FPrimitiveSceneProxy*> InStaticPrimitives,
		TSet<FPrimitiveSceneProxy*> InDynamicPrimitives,
		UTextureRenderTarget2D* InRenderTarget,
		FTransform InTransform,
		float InOrthoWidth) const;

	void Simulate(const float DeltaTime);
	void AllocateRTs();
	void DrawImpulseForces();
	void UpdateSimulationData() const;
	void UpdateGridPosition();
	FBox GetSimulationVolume() const;
};
