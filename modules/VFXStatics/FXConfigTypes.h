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
#include "GameplayTagContainer.h"
#include "Particles/ParticleSystemComponent.h"
#include "FXConfigTypes.generated.h"

class UFXSystemComponent;

namespace FFXSpawnLibrary
{
	static constexpr uint8 MinEffectPriority = 0;
}

class UFXSystemAsset;

USTRUCT()
struct FFXIntParam
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FName ParameterName = NAME_None;

	UPROPERTY(EditDefaultsOnly)
	int32 Value = 0;

	FFXIntParam() = default;
	FFXIntParam(const FName& ParameterName, int32 Value);
};

USTRUCT()
struct FFXFloatParam
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FName ParameterName = NAME_None;

	UPROPERTY(EditDefaultsOnly)
	float Value = 0.0f;

	FFXFloatParam() = default;
	FFXFloatParam(const FName& ParameterName, float Value);
};

USTRUCT()
struct FFXVectorParam
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FName ParameterName = NAME_None;

	UPROPERTY(EditDefaultsOnly)
	FVector Value = FVector::ZeroVector;

	FFXVectorParam() = default;
	FFXVectorParam(const FName& ParameterName, const FVector& Value);
};

USTRUCT()
struct FFXParametersParams
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TArray<FFXFloatParam> FloatParameters {};

	UPROPERTY(EditDefaultsOnly)
	TArray<FFXVectorParam> VectorParameters {};
};

USTRUCT(BlueprintType)
struct FFXEffectData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	UFXSystemAsset* FXAlly = nullptr;

	UPROPERTY(EditDefaultsOnly)
	UFXSystemAsset* FXEnemy = nullptr;

	// For now works only for Cascade Effects
	UPROPERTY(EditDefaultsOnly)
	float FXPlayRate = 1.0f;

	UPROPERTY(EditDefaultsOnly)
	FTransform FXOffset = FTransform::Identity;

	UPROPERTY(EditDefaultsOnly)
	FFXParametersParams FXParametersData {};

	bool operator==(FFXEffectData const& FXData) const;
	bool operator==(UFXSystemAsset const* FXSystemAsset) const;
	bool operator==(UFXSystemComponent const* FXSystemComponent) const;
	bool IsEmpty() const;
};

USTRUCT(BlueprintType)
struct FFXEffectDataSimple
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	UFXSystemAsset* FXSystem = nullptr;

	UPROPERTY(EditAnywhere)
	FTransform FXOffset = FTransform::Identity;

	bool operator==(FFXEffectDataSimple const& FXData) const;
	bool operator==(UFXSystemAsset const* FXSystemAsset) const;
	bool operator==(UFXSystemComponent const* FXSystemComponent) const;
};

USTRUCT(BlueprintType)
struct FFXEffectDataAttached
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FFXEffectData FXData {};

	// Spawns an Emitter Actor, which will deactivate system after destroy, allowing it to finish properly
	// NOTE: spawning additional actor comes with the costs, so it should be used only in absolutely necessary situations
	// NOTE 2: Emitter Actor do not support pooling
	UPROPERTY(EditDefaultsOnly)
	bool bUseEmitterActorForSpawn = false;

	// Helps how FX behaves, if they spawned at the same position (same socket, same attach component)
	// The rules are:
	// 1. We only show FXs with the highest priority
	// 2. We show all FXs with the same priority
	UPROPERTY(EditDefaultsOnly)
	uint8 EffectPriority = FFXSpawnLibrary::MinEffectPriority;

	FFXEffectDataAttached() = default;
	explicit FFXEffectDataAttached(const FFXEffectData& InFXData);

	bool operator==(FFXEffectDataAttached const& FXDataAttached) const;
	bool operator==(UFXSystemAsset const* FXSystemAsset) const;
	bool operator==(UFXSystemComponent const* FXSystemComponent) const;

	bool IsEmpty() const;
};

USTRUCT()
struct FFXBeamData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	FFXEffectData FXData {};

	UPROPERTY(EditDefaultsOnly)
	FName BeamSourceParameterName = NAME_None;

	UPROPERTY(EditDefaultsOnly)
	FName BeamTargetParameterName = NAME_None;

	UPROPERTY(EditDefaultsOnly)
	FName BeamSourceSocketName = NAME_None;

	UPROPERTY(EditDefaultsOnly)
	FName BeamTargetSocketName = NAME_None;
};
