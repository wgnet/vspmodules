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

#include "FluidSurfaces/Private/FluidSimulationRenderer.h"
#include "FluidSurfaceSubsystem.generated.h"

class AFluidSurfaceActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnFluidSurfaceScalabilityChanged);

UCLASS()
class FLUIDSURFACES_API UFluidSurfaceSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	UFluidSurfaceSubsystem();

	virtual bool IsTickable() const override;
	virtual bool IsTickableInEditor() const override;
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	static UFluidSurfaceSubsystem* Get(const UWorld* InWorld);

	void SetFluidSurfaceActor(AFluidSurfaceActor* Actor) const;
	AFluidSurfaceActor* GetFluidSurfaceActor() const;
	static int32 GetFluidSimulationRenderTargetSize();
	static int32 GetFluidSimulationWorldSize();
	static int32 GetFluidSimulationVirtualFPS();
	static int32 GetFluidSimulationMaxSubstepsPerFrame();
	static bool IsFluidSimulationEnabled();
	static FName GetCollisionProfileName();
	FTraceDatum& GetLastTraceDatum()
	{
		return TraceDatum;
	}
	TSet<UPrimitiveComponent*>& GetCachedPrimitives()
	{
		return CachedPrimitives;
	};

	UPROPERTY(BlueprintAssignable, Category = "FluidSurface")
	FOnFluidSurfaceScalabilityChanged OnFluidSurfaceScalabilityChanged;

	FSimpleMulticastDelegate OnCachedPrimitivesUpdate;

private:
	DECLARE_DELEGATE_OneParam(FOutPrimsSignature, TSet<UPrimitiveComponent*>)
	FOutPrimsSignature OutPrimsSignature;

	UPROPERTY(Transient)
	mutable AFluidSurfaceActor* FluidSurfaceActor;

	UPROPERTY(Transient)
	TSet<UPrimitiveComponent*> CachedPrimitives;

	void CachePrimitives(TSet<UPrimitiveComponent*> InComponents);
	void NotifyScalabilityChangedInternal(IConsoleVariable* CVar) const;
	void OnLoadProfileConfig(UCollisionProfile* CollisionProfile);
	void AddFluidSurfaceCollisionProfile();

private:
	TArray<TEnumAsByte<ECollisionChannel> > TraceTypes { ECC_PhysicsBody, ECC_Pawn };
	FTraceHandle LastTraceHandle;
	FTraceDelegate TraceDelegate;
	FTraceDatum TraceDatum;
	FTraceHandle RequestTrace();
	bool bOverlapAsync = true;
};
