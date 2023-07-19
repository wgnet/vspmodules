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

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "HydraulicErosionComponent.generated.h"

class AHydraulicErosion;

USTRUCT(BlueprintType)
struct FHydraulicErosionDada
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* HeightRTA = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* HeightRTB = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* FluidVelocityA = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* FluidVelocityB = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* PositionRTA = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* PositionRTB = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* MeshDepthRT = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UTextureRenderTarget2D* NormalRT = nullptr;
};

UENUM(BlueprintType)
enum EHydraulicErosionMode
{
	// Uses individual, non colliding particles. Useful for carving lots of detailed rivers.
	FreeParticles = 0,
	// Performs a 2d fluid sim using Advection. This sim is very accurate but requires a very small time step (DeltaT < 0.016).
	// Larger time steps will cause the sim to 'blow up'. Currently Velocity Advection Erosion works better for this mode than sediment transport.
	ShallowWater = 1,
	// Performs a 2d fluid sim using particles that gather back to grid points.
	// This simulation is not quite accurate in terms of fluid motion but is much more robust to larger time steps (DeltaT can get near 0.05).
	// It is slower to compute but the time step flexibility makes up for that.
	// Currently Velocity Advection Erosion works better for this mode than sediment transport.
	ScatterGather = 2,
	// Simple water levelling does not use velocity, so only the standard Solubility,
	// Evaporation and Slope controls will work. Velocity advection is disabled for this mode. It is by far the fastest to compute.
	SimpleWaterLevelling = 3,
};

UCLASS(config = Engine, Blueprintable, BlueprintType)
class UHydraulicErosionComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UHydraulicErosionComponent();

	UFUNCTION(BlueprintCallable)
	void Constructor(FHydraulicErosionDada InHydraulicErosionDada);

	UFUNCTION(BlueprintCallable)
	void TickErosion(int32 InIterations);

	UFUNCTION(BlueprintCallable)
	void TickErosionOnce(int32 InIterations);

	UFUNCTION(BlueprintCallable)
	void StartErode(int32 InIterations);

	UFUNCTION(BlueprintCallable)
	void SeedParticlePositions();

	UFUNCTION(BlueprintCallable)
	void CreateMIDs();

	UFUNCTION(BlueprintCallable)
	void SetMIDParams();

	UFUNCTION(BlueprintCallable)
	void LoadMaterials(); // SetupDefaultMaterials

	UFUNCTION(BlueprintPure)
	UTextureRenderTarget2D* PingPongPositionSource() const;

	UFUNCTION(BlueprintPure)
	UTextureRenderTarget2D* PingPongPositionTarget() const;

	UFUNCTION(BlueprintPure)
	UTextureRenderTarget2D* PingPongVelocitySource() const;

	UFUNCTION(BlueprintPure)
	UTextureRenderTarget2D* PingPongVelocityTarget() const;

	UFUNCTION(BlueprintPure)
	UTextureRenderTarget2D* PingPongHeightSource() const;

	UFUNCTION(BlueprintPure)
	UTextureRenderTarget2D* PingPongHeightTarget() const;

	UFUNCTION(BlueprintCallable)
	void FreeParticlesScatter();

	UFUNCTION(BlueprintCallable)
	void ShallowWaterAdvection();

	UFUNCTION(BlueprintCallable)
	void ScatterGatherMethod();

	UFUNCTION(BlueprintCallable)
	void SimpleWaterLevelling();

	UFUNCTION(BlueprintCallable)
	void RenderWeightmapEffects();

	// Hydraulic Erosion Mode
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SimulationSettings")
	TEnumAsByte<EHydraulicErosionMode> HydraulicErosionMode = ShallowWater;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AdvancedEffects")
	float VelocityAdvectionErosion;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AdvancedEffects")
	float NormalAdvectionErosion;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "AdvancedEffects")
	float SlopeSmoothThreshold;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* VelocityAccelerationMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* AdvectVelocityMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* AdvectHeightMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* ScatterParticlePreviewMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* ScatterParticleRenderMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* ScatterVelocityMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* ScatterPositionMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* ParticlePositionSeedMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* DivergenceHeightUpdateMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* ScatterHeightErosionMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* WeightmapEffectsMID;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "MIDs")
	UMaterialInstanceDynamic* WaterLevelMID;


	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Debug")
	int32 VelocityIdx;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Debug")
	int32 PositionIdx;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Debug")
	int32 IterationIdx;

	// Used internally to track ping ponging and number of iterations.
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Debug")
	int32 StackHeightIdx = 0;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "RenderTargets")
	FHydraulicErosionDada HydraulicErosionDada;

	// Materials
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Advect")
	UMaterialInterface* GravAccelerationMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|ScatterGather")
	UMaterialInterface* ScatterGatherVelocityMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|ScatterGather")
	UMaterialInterface* GatherDataMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|ScatterGather")
	UMaterialInterface* GatherVelocityMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials")
	UMaterialInterface* WaterLevelMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Levelling")
	UMaterialInterface* ErosionWeightmapMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Brushes")
	UMaterialInterface* CalcDivergenceMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Scatter")
	UMaterialInterface* ParticlePreviewMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Scatter")
	UMaterialInterface* ParticleRenderMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Scatter")
	UMaterialInterface* ScatterVelocityMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Scatter")
	UMaterialInterface* ScatterPositionMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Scatter")
	UMaterialInterface* ParticleWaterSeedMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Scatter")
	UMaterialInterface* StartPositionErodeMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Overrides")
	UMaterialInterface* ShallowWaterAdvectHeightMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Overrides")
	UMaterialInterface* ShallowWaterAdvectVelocityMat;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Materials|Overrides")
	UMaterialInterface* UnpackRG8HeightMat;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Generation")
	UMaterialParameterCollection* ErosionMPC;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	AHydraulicErosion* OwningLayerStack { nullptr };
};
