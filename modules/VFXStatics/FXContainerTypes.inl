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
#include "Particles/ParticleSystemComponent.h"

template<typename TComparisonType>
bool FFXComponentContainer::Contains(const TComparisonType& Item)
{
	bool bResult = false;
	for (auto It = FXComponents.CreateIterator(); It; ++It)
	{
		UFXSystemComponent* FXComponent = *It;
		if (IsValid(FXComponent) && FXComponent->IsActive())
		{
			if (Item == FXComponent->GetFXSystemAsset())
			{
				bResult = true;
			}
		}
		else
		{
			It.RemoveCurrent();
		}
	}

	return bResult;
}

template<typename TRemoveType>
void FFXComponentContainer::RemoveFXComponents(TRemoveType const& InFXData)
{
	for (auto It = FXComponents.CreateIterator(); It; ++It)
	{
		UFXSystemComponent* FXComponent = *It;
		if (IsValid(FXComponent) && FXComponent->IsActive())
		{
			if (InFXData == FXComponent->GetFXSystemAsset())
			{
				FXComponent->ReleaseToPool();
				It.RemoveCurrent();
			}
		}
		else
		{
			It.RemoveCurrent();
		}
	}
}