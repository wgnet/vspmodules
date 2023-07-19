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
#include "FXControlComponent.generated.h"

struct FFXEffectDataAttached;
class UFXSystemComponent;
class UFXSystemAsset;

UCLASS()
class UFXControlComponent
	: public UObject
	, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UFXSystemComponent* TryIncreaseEffectsNum(
		UFXSystemAsset* FX,
		USceneComponent* AttachToComponent,
		const FName& SocketName);
	void AddEffectInSocket(
		UFXSystemComponent* FXComponent,
		USceneComponent* AttachToComponent,
		const FName& SocketName,
		uint8 EffectPriority);

	void DeactivateEffect(UFXSystemAsset const* FXToDeactivate, bool bReleaseToPool);
	void DeactivateEffect(FFXEffectDataAttached const* FXToDeactivate, bool bReleaseToPool);
	void DeactivateAllEffects(TArray<UFXSystemAsset*> const& FXToDeactivate, bool bReleaseToPool);
	void DeactivateAllEffects(bool bReleaseToPool);

	virtual void Tick(float DeltaTime) override;
	virtual ETickableTickType GetTickableTickType() const override;
	virtual bool IsTickableInEditor() const override;
	virtual TStatId GetStatId() const override;

private:
	struct FFXLocationInfo
	{
		FName SocketName = NAME_None;
		TWeakObjectPtr<USceneComponent> AttachToComponent = nullptr;
	};
	friend bool operator==(const FFXLocationInfo& First, const FFXLocationInfo& Second);
	friend uint32 GetTypeHash(const FFXLocationInfo& FXLocation);

	struct FFXInfo
	{
		uint32 FXCounter = 1;
		uint8 EffectPriority = 0;
		TWeakObjectPtr<UFXSystemComponent> FXComponent = nullptr;

		FFXInfo() = default;
		FFXInfo(uint8 EffectPriority, UFXSystemComponent* InFXComponent);

		bool IsValid() const;
	};
	friend bool operator==(const FFXInfo& First, const UFXSystemComponent* Second);

	using FFXStack = TArray<FFXInfo>;
	// TODO: consider adding iterator for this type to reduce the amount of copy-paste
	using FFXLocationToInfos = TMap<FFXLocationInfo, FFXStack>;

	FFXLocationToInfos FXLocationToInfos {};
	float TimeSinceLastTick = 0.0f;

	void EvaluateFXStack();
	void EvaluateFXStack(FFXStack& FXStack);
};
