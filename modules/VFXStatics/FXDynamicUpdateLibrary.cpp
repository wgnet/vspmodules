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


#include "FXDynamicUpdateLibrary.h"
#include "FXConfigTypes.h"
#include "FXContainerTypes.h"

#include "Curves/CurveFloat.h"
#include "Curves/CurveVector.h"
#include "Particles/ParticleSystemComponent.h"

namespace FFXDynamicUpdateLibraryLocal
{
	template<typename TFXDynamicUpdateParamType, typename TFXParamType, typename TCurveType>
	void UpdateParameter(
		float DynamicValue,
		const TArray<TFXDynamicUpdateParamType>& DynamicUpdateParameters,
		TFunction<void(const TFXParamType&)>&& UpdateCallback,
		decltype(TFXParamType::Value) (TCurveType::*GetCurveValue)(float) const)
	{
		static_assert(
			TIsDerivedFrom<TCurveType, UCurveBase>::Value,
			TEXT("CurveType must be inherited from UCurveBase"));
		using FValueType = decltype(TFXParamType::Value);
		TFXParamType FXParam {};
		for (const TFXDynamicUpdateParamType& DynamicUpdateParam : DynamicUpdateParameters)
		{
			FValueType ParameterValue { DynamicValue };
			if (IsValid(DynamicUpdateParam.DynamicUpdateCurve))
			{
				ParameterValue = (DynamicUpdateParam.DynamicUpdateCurve->*GetCurveValue)(DynamicValue);
			}

			FXParam.Value = ParameterValue;
			for (const FName& ParameterName : DynamicUpdateParam.DynamicUpdateParametersName)
			{
				FXParam.ParameterName = ParameterName;
				UpdateCallback(FXParam);
			}
		}
	}
}

void FFXDynamicUpdateLibrary::UpdateFXWithNormalizedValue(
	FFXComponentContainer& FXContainer,
	const FFXDynamicUpdateData& DynamicUpdateData,
	float NormalizedValue)
{
	FFXDynamicUpdateLibraryLocal::UpdateParameter<FFXDynamicUpdateParam_Float, FFXFloatParam, UCurveFloat>(
		NormalizedValue,
		DynamicUpdateData.DynamicUpdateFloats,
		[&FXContainer](const FFXFloatParam& Param)
		{
			FXContainer.UpdateFXFloatParameter(Param);
		},
		&UCurveFloat::GetFloatValue);

	FFXDynamicUpdateLibraryLocal::UpdateParameter<FFXDynamicUpdateParam_Vector, FFXVectorParam, UCurveVector>(
		NormalizedValue,
		DynamicUpdateData.DynamicUpdateVectors,
		[&FXContainer](const FFXVectorParam& Param)
		{
			FXContainer.UpdateFXVectorParameter(Param);
		},
		&UCurveVector::GetVectorValue);

	for (const FFXDynamicUpdateParam_Emitter& DynamicUpdateParam : DynamicUpdateData.DynamicUpdateEmitters)
	{
		bool bEnabled = !DynamicUpdateParam.bEnableEmitter;
		if (NormalizedValue >= DynamicUpdateParam.ValueThreshold)
		{
			bEnabled = DynamicUpdateParam.bEnableEmitter;
		}
		for (const FName& EmitterName : DynamicUpdateParam.DynamicUpdateEmittersName)
		{
			FXContainer.SetEmitterEnable(EmitterName, bEnabled);
		}
	}
}

void FFXDynamicUpdateLibrary::UpdateFXWithNormalizedValue(
	UFXSystemComponent* FXComponent,
	const FFXDynamicUpdateData& DynamicUpdateData,
	float NormalizedValue)
{
	check(FXComponent);

	if (DynamicUpdateData.IsEmpty())
	{
		return;
	}
	FFXDynamicUpdateLibraryLocal::UpdateParameter<FFXDynamicUpdateParam_Float, FFXFloatParam, UCurveFloat>(
		NormalizedValue,
		DynamicUpdateData.DynamicUpdateFloats,
		[&FXComponent](const FFXFloatParam& Param)
		{
			FXComponent->SetFloatParameter(Param.ParameterName, Param.Value);
		},
		&UCurveFloat::GetFloatValue);

	FFXDynamicUpdateLibraryLocal::UpdateParameter<FFXDynamicUpdateParam_Vector, FFXVectorParam, UCurveVector>(
		NormalizedValue,
		DynamicUpdateData.DynamicUpdateVectors,
		[&FXComponent](const FFXVectorParam& Param)
		{
			FXComponent->SetVectorParameter(Param.ParameterName, Param.Value);
		},
		&UCurveVector::GetVectorValue);

	for (const FFXDynamicUpdateParam_Emitter& DynamicUpdateParam : DynamicUpdateData.DynamicUpdateEmitters)
	{
		bool bEnabled = !DynamicUpdateParam.bEnableEmitter;
		if (NormalizedValue >= DynamicUpdateParam.ValueThreshold)
		{
			bEnabled = DynamicUpdateParam.bEnableEmitter;
		}
		for (const FName& EmitterName : DynamicUpdateParam.DynamicUpdateEmittersName)
		{
			FXComponent->SetEmitterEnable(EmitterName, bEnabled);
		}
	}
}

bool FFXDynamicUpdateData::IsEmpty() const
{
	return DynamicUpdateEmitters.Num() == 0 && DynamicUpdateFloats.Num() == 0 && DynamicUpdateVectors.Num() == 0;
}
