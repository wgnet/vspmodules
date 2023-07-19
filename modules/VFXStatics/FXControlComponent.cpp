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
#include "FXControlComponent.h"
#include "FXSpawnLibrary.h"
#include "Particles/ParticleSystemComponent.h"

namespace FFXControlComponentLocal
{
	static constexpr float TickInterval = 0.5f;
}

bool operator==(const UFXControlComponent::FFXLocationInfo& First, const UFXControlComponent::FFXLocationInfo& Second)
{
	return First.SocketName == Second.SocketName && First.AttachToComponent == Second.AttachToComponent
		&& First.AttachToComponent.IsValid();
}

uint32 GetTypeHash(const UFXControlComponent::FFXLocationInfo& FXLocation)
{
	uint32 Hash = GetTypeHash(FXLocation.SocketName);
	Hash = HashCombine(Hash, GetTypeHash(FXLocation.AttachToComponent.Get()));
	return Hash;
}

UFXControlComponent::FFXInfo::FFXInfo(uint8 EffectPriority, UFXSystemComponent* InFXComponent)
	: EffectPriority(EffectPriority)
	, FXComponent(InFXComponent)
{
}

bool UFXControlComponent::FFXInfo::IsValid() const
{
	return FXComponent.IsValid() && FXComponent.Get()->IsActive() && FXCounter > 0;
}

void UFXControlComponent::EvaluateFXStack()
{
	for (auto It = FXLocationToInfos.CreateIterator(); It; ++It)
	{
		EvaluateFXStack(It.Value());
	}
}

void UFXControlComponent::EvaluateFXStack(FFXStack& FXStack)
{
	check(FXStack.Num() != 0);

	Algo::Sort(
		FXStack,
		[](const FFXInfo& Left, const FFXInfo& Right)
		{
			return Left.EffectPriority > Right.EffectPriority;
		});

	const uint8 MaxPriority = FXStack[0].EffectPriority;
	for (FFXInfo FXInfo : FXStack)
	{
		if (!FXInfo.IsValid())
		{
			continue;
		}
		if (FXInfo.EffectPriority >= MaxPriority)
		{
			FXInfo.FXComponent->SetVisibility(true, false);
		}
		else
		{
			FXInfo.FXComponent->SetVisibility(false, false);
		}
	}
}

bool operator==(const UFXControlComponent::FFXInfo& First, const UFXSystemComponent* Second)
{
	return First.IsValid() && First.FXComponent.IsValid() && First.FXComponent.Get() == Second;
}

UFXSystemComponent* UFXControlComponent::TryIncreaseEffectsNum(
	UFXSystemAsset* FX,
	USceneComponent* AttachToComponent,
	const FName& SocketName)
{
	FFXLocationInfo LocationInfo { SocketName, AttachToComponent };
	FFXStack* FoundFXInfosPtr = FXLocationToInfos.Find(MoveTemp(LocationInfo));
	if (FoundFXInfosPtr)
	{
		FFXStack& FXInfos = *FoundFXInfosPtr;
		for (auto It = FXInfos.CreateIterator(); It; ++It)
		{
			if (It->FXComponent.IsValid() && It->FXComponent->GetFXSystemAsset() == FX)
			{
				if (It->IsValid())
				{
					It->FXCounter++;
					return It->FXComponent.Get();
				}

				// Since tick interval may be greater than one frame, the situation is possible, were FX gone to be FreeInPool (means bIsActive == false),
				// Then we call UFXControlComponent::TryIncreaseEffectsNum, after that the outside code spawns FX (now bIsActive == true),
				// Then UFXControlComponent::AddEffectInSocket called and, due to pooling we have the same exact FX in our array as "newly" spawned one
				// And we will not pass check for not containing FX to add, since we didn't tick yet, thus didn't have an opportunity to remove invalid FX
				// To prevent this situations we want to remove invalid FX at this point
				It.RemoveCurrent();
				return nullptr;
			}
		}
	}

	return nullptr;
}

void UFXControlComponent::AddEffectInSocket(
	UFXSystemComponent* FXComponent,
	USceneComponent* AttachToComponent,
	const FName& SocketName,
	uint8 EffectPriority)
{
	FFXLocationInfo LocationInfo { SocketName, AttachToComponent };
	FFXStack& FXInfos = FXLocationToInfos.FindOrAdd(MoveTemp(LocationInfo));

	check(!FXInfos.Contains(FXComponent));

	FXInfos.Emplace(EffectPriority, FXComponent);

	EvaluateFXStack(FXInfos);
}

void UFXControlComponent::DeactivateEffect(UFXSystemAsset const* FXToDeactivate, bool bReleaseToPool)
{
	for (auto LocationIt = FXLocationToInfos.CreateIterator(); LocationIt; ++LocationIt)
	{
		for (auto FXIt = LocationIt.Value().CreateIterator(); FXIt; ++FXIt)
		{
			if (FXIt->FXComponent.IsValid() && FXIt->FXComponent.Get()->GetFXSystemAsset() == FXToDeactivate)
			{
				FXIt->FXCounter--;
				if (FXIt->FXCounter == 0)
				{
					bReleaseToPool ? FXIt->FXComponent->ReleaseToPool() : FXIt->FXComponent->Deactivate();
					FXIt.RemoveCurrent();
				}
			}
		}
		if (LocationIt.Value().Num() == 0)
		{
			LocationIt.RemoveCurrent();
		}
	}
}

void UFXControlComponent::DeactivateEffect(FFXEffectDataAttached const* FXToDeactivate, bool bReleaseToPool)
{
	check(FXToDeactivate);
	bool bEffectRemoved = false;
	for (auto LocationIt = FXLocationToInfos.CreateIterator(); LocationIt; ++LocationIt)
	{
		for (auto FXIt = LocationIt.Value().CreateIterator(); FXIt; ++FXIt)
		{
			if (FXIt->FXComponent.IsValid() && *FXToDeactivate == FXIt->FXComponent.Get()->GetFXSystemAsset())
			{
				FXIt->FXCounter--;
				if (FXIt->FXCounter == 0)
				{
					bReleaseToPool ? FXIt->FXComponent->ReleaseToPool() : FXIt->FXComponent->Deactivate();
					FXIt.RemoveCurrent();
					bEffectRemoved = true;
				}
			}
		}
		if (LocationIt.Value().Num() == 0)
		{
			LocationIt.RemoveCurrent();
		}
	}

	if (bEffectRemoved)
	{
		EvaluateFXStack();
	}
}

void UFXControlComponent::DeactivateAllEffects(TArray<UFXSystemAsset*> const& FXToDeactivate, bool bReleaseToPool)
{
	if (FXToDeactivate.Num() == 0)
	{
		return;
	}
	for (auto LocationIt = FXLocationToInfos.CreateIterator(); LocationIt; ++LocationIt)
	{
		for (auto FXIt = LocationIt.Value().CreateIterator(); FXIt; ++FXIt)
		{
			if (FXIt->FXComponent.IsValid() && FXToDeactivate.Contains(FXIt->FXComponent.Get()->GetFXSystemAsset()))
			{
				FXIt->FXCounter--;
				if (FXIt->FXCounter == 0)
				{
					bReleaseToPool ? FXIt->FXComponent->ReleaseToPool() : FXIt->FXComponent->Deactivate();
					FXIt.RemoveCurrent();
				}
			}
		}
		if (LocationIt.Value().Num() == 0)
		{
			LocationIt.RemoveCurrent();
		}
	}
}

void UFXControlComponent::DeactivateAllEffects(bool bReleaseToPool)
{
	for (auto LocationIt = FXLocationToInfos.CreateIterator(); LocationIt; ++LocationIt)
	{
		for (auto FXIt = LocationIt.Value().CreateIterator(); FXIt; ++FXIt)
		{
			if (FXIt->FXComponent.IsValid())
			{
				FXIt->FXCounter--;
				if (FXIt->FXCounter == 0)
				{
					bReleaseToPool ? FXIt->FXComponent->ReleaseToPool() : FXIt->FXComponent->Deactivate();
					FXIt.RemoveCurrent();
				}
			}
		}
		if (LocationIt.Value().Num() == 0)
		{
			LocationIt.RemoveCurrent();
		}
	}
}

void UFXControlComponent::Tick(float DeltaTime)
{
	TimeSinceLastTick += DeltaTime;
	if (TimeSinceLastTick < FFXControlComponentLocal::TickInterval)
	{
		return;
	}

	for (auto LocationIt = FXLocationToInfos.CreateIterator(); LocationIt; ++LocationIt)
	{
		uint8 MaxRemovedEffectPriority = FFXSpawnLibrary::MinEffectPriority;
		uint8 MaxActiveEffectPriority = FFXSpawnLibrary::MinEffectPriority;
		for (auto FXIt = LocationIt.Value().CreateIterator(); FXIt; ++FXIt)
		{
			if (!FXIt->IsValid())
			{
				MaxRemovedEffectPriority = FMath::Max(FXIt->EffectPriority, MaxRemovedEffectPriority);

				// At this point the FX system has already completed, meaning it was either reclaimed by the pool or deactivated
				// We do not need to care about it and can safely remove
				FXIt.RemoveCurrent();
			}
			else
			{
				MaxActiveEffectPriority = FMath::Max(FXIt->EffectPriority, MaxActiveEffectPriority);
			}
		}
		if (LocationIt.Value().Num() == 0)
		{
			LocationIt.RemoveCurrent();
		}
		// We only evaluate FX stack if we removed the effect with the max priority
		else if (MaxRemovedEffectPriority > MaxActiveEffectPriority)
		{
			EvaluateFXStack(LocationIt.Value());
		}
	}


	TimeSinceLastTick = 0.0f;
}

ETickableTickType UFXControlComponent::GetTickableTickType() const
{
	return ETickableTickType::Conditional;
}

bool UFXControlComponent::IsTickableInEditor() const
{
	return true;
}

TStatId UFXControlComponent::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(UFXControlComponent, STATGROUP_Tickables);
}
