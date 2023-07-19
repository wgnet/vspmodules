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
#include "FXConfigTypes.h"

namespace FFXSpawnLibrary
{
	template<typename KeyType>
	void UpdateFXLocation(
		TMap<KeyType, UFXSystemComponent*>& InUpdatingSystems,
		const FVector InNewLocation,
		bool bRelative)
	{
		for (auto& FXSystem : InUpdatingSystems)
		{
			const FVector CurrentLocation =
				bRelative ? FXSystem.Value->GetRelativeLocation() : FXSystem.Value->GetComponentLocation();
			if (IsValid(FXSystem.Value) && InNewLocation != CurrentLocation)
			{
				if (bRelative)
				{
					FXSystem.Value->SetRelativeLocation(InNewLocation, false, nullptr, ETeleportType::TeleportPhysics);
				}
				else
				{
					FXSystem.Value->SetWorldLocation(InNewLocation, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
		}
	}

	template<typename FFXComponentType>
	void UpdateFXParameters(FFXComponentType& FXComponent, const FFXParametersParams& Params)
	{
		check(FXComponent);
		for (const auto& FloatParam : Params.FloatParameters)
		{
			UpdateFXFloatParameter(FXComponent, FloatParam);
		}

		for (const auto& VectorParam : Params.VectorParameters)
		{
			UpdateFXVectorParameter(FXComponent, VectorParam);
		}
	}

	template<typename FFXComponentType>
	void UpdateFXVectorParameter(FFXComponentType& FXComponent, const FFXVectorParam& VectorParam)
	{
		check(FXComponent);
		FXComponent->SetVectorParameter(VectorParam.ParameterName, VectorParam.Value);
	}

	template<typename FFXComponentType>
	void UpdateFXFloatParameter(FFXComponentType& FXComponent, const FFXFloatParam& FloatParam)
	{
		check(FXComponent);
		FXComponent->SetFloatParameter(FloatParam.ParameterName, FloatParam.Value);
	}

	template<typename FFXComponentType>
	void UpdateFXIntParameter(FFXComponentType& FXComponent, const FFXIntParam& IntParam)
	{
		check(FXComponent);
		FXComponent->SetIntParameter(IntParam.ParameterName, IntParam.Value);
	}
}
