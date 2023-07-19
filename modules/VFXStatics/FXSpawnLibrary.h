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

#include "FXContainerTypes.h"
#include "Particles/ParticleSystemComponent.h"

#include "CoreMinimal.h"

struct FFXParametersParams;
struct FFXVectorParam;
struct FFXFloatParam;
struct FFXEffectDataAttached;
struct FFXEffectData;
struct FFXEffectDataSimple;
struct FFXBeamData;
class UFXSystemComponent;
class UFXControlComponent;
class UFXSystemAsset;

namespace FFXSpawnLibrary
{
	FFXComponentContainer SpawnFXAttachedToActor(
		const FFXEffectDataAttached& Config,
		UWorld* World,
		const AActor* Instigator,
		AActor* AttachToActor,
		EPSCPoolMethod PoolMethod,
		EAttachLocation::Type AttachType = EAttachLocation::Type::KeepRelativeOffset,
		const FVector& Location = FVector::ZeroVector,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector,
		bool bAutoActivate = true,
		bool bAutoDestroy = true,
		UFXControlComponent* VFXControlComponent = nullptr);

	FFXComponentContainer SpawnFXAttachedToComponent(
		const FFXEffectDataAttached& Config,
		UWorld* World,
		const AActor* Instigator,
		USceneComponent* AttachToComponent,
		EPSCPoolMethod PoolMethod,
		EAttachLocation::Type AttachType = EAttachLocation::Type::KeepRelativeOffset,
		const FVector& Location = FVector::ZeroVector,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector,
		bool bAutoActivate = true,
		bool bAutoDestroy = true,
		UFXControlComponent* FXControlComponent = nullptr);

	FFXComponentContainer SpawnFXAttachedToNoneSocket(
		const FFXEffectDataAttached& Config,
		UWorld* World,
		const AActor* Instigator,
		USceneComponent* AttachToComponent,
		EPSCPoolMethod PoolMethod,
		EAttachLocation::Type AttachType = EAttachLocation::Type::KeepRelativeOffset,
		const FVector& Location = FVector::ZeroVector,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector,
		bool bAutoActivate = true,
		bool bAutoDestroy = true,
		UFXControlComponent* FXControlComponent = nullptr);

	UFXSystemComponent* SpawnFXAttachedToNoneSocket(
		const FFXEffectDataSimple& Config,
		UWorld* World,
		USceneComponent* AttachToComponent,
		EPSCPoolMethod PoolMethod,
		EAttachLocation::Type AttachType = EAttachLocation::Type::KeepRelativeOffset,
		const FVector& Location = FVector::ZeroVector,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector,
		bool bAutoActivate = true,
		bool bAutoDestroy = true,
		UFXControlComponent* FXControlComponent = nullptr);

	UFXSystemComponent* SpawnFXAttachedToNoneSocket(
		UFXSystemAsset* FXSystemAsset,
		UWorld* World,
		USceneComponent* AttachToComponent,
		EPSCPoolMethod PoolMethod,
		EAttachLocation::Type AttachType = EAttachLocation::Type::KeepRelativeOffset,
		const FVector& Location = FVector::ZeroVector,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector,
		bool bAutoActivate = true,
		bool bAutoDestroy = true,
		UFXControlComponent* FXControlComponent = nullptr);


	FFXComponentContainer SpawnFXContainerAtLocation(
		const FFXEffectData& Config,
		UWorld* World,
		AActor* Instigator,
		EPSCPoolMethod PoolMethod,
		const FVector& SpawnLocation,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector,
		bool bAutoActivate = true,
		bool bAutoDestroy = true);

	UFXSystemComponent* SpawnFXAtLocation(
		const FFXEffectData& Config,
		UWorld* World,
		AActor* Instigator,
		EPSCPoolMethod PoolMethod,
		const FVector& SpawnLocation,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector,
		bool bAutoActivate = true,
		bool bAutoDestroy = true);

	UFXSystemComponent* SpawnFXAtLocation(
		const FFXEffectDataSimple& Config,
		UWorld* World,
		EPSCPoolMethod PoolMethod,
		const FVector& SpawnLocation,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector,
		bool bAutoActivate = true,
		bool bAutoDestroy = true);


	FFXBeamContainer SpawnFXBeam(
		const FFXBeamData& Config,
		UWorld* World,
		AActor* Instigator,
		AActor* SourceActor,
		AActor* TargetActor,
		EPSCPoolMethod PoolMethod,
		bool bAutoActivate = true,
		bool bAutoDestroy = true);

	FFXBeamContainer SpawnFXBeam(
		const FFXBeamData& Config,
		UWorld* World,
		AActor* Instigator,
		AActor* SourceActor,
		const FVector& TargetLocation,
		EPSCPoolMethod PoolMethod,
		bool bAutoActivate = true,
		bool bAutoDestroy = true);

	FFXBeamContainer SpawnFXBeam(
		const FFXBeamData& Config,
		UWorld* World,
		AActor* Instigator,
		USceneComponent* SourceComponent,
		const FVector& TargetLocation,
		EPSCPoolMethod PoolMethod,
		bool bAutoActivate = true,
		bool bAutoDestroy = true);

	FFXBeamContainer SpawnFXBeam(
		const FFXBeamData& Config,
		UWorld* World,
		AActor* Instigator,
		const FVector& SourceLocation,
		const FVector& TargetLocation,
		EPSCPoolMethod PoolMethod,
		bool bAutoActivate = true,
		bool bAutoDestroy = true);

	void UpdateFXLocation(
		TArray<UFXSystemComponent*>& InUpdatingSystems,
		const FVector& InNewLocation,
		bool bRelative = false);

	template<typename KeyType>
	void UpdateFXLocation(
		TMap<KeyType, UFXSystemComponent*>& InUpdatingSystems,
		const FVector InNewLocation,
		bool bRelative = false);

	template<typename FFXComponentType>
	void UpdateFXParameters(FFXComponentType& FXComponent, const FFXParametersParams& Params);
	template<typename FFXComponentType>
	void UpdateFXVectorParameter(FFXComponentType& FXComponent, const FFXVectorParam& VectorParam);
	template<typename FFXComponentType>
	void UpdateFXFloatParameter(FFXComponentType& FXComponent, const FFXFloatParam& FloatParam);
	void UpdateFXBeam(const FFXBeamContext& BeamContext);

	TArray<USceneComponent*> CollectSceneComponentsForAttach(AActor* AttachToActor);
}

#if CPP
	#include "FXSpawnLibrary.inl"
#endif
