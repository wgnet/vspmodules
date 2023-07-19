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

#include "FXConfigTypes.h"
#include "Particles/ParticleSystem.h"

FFXIntParam::FFXIntParam(const FName& ParameterName, int32 Value) : ParameterName(ParameterName), Value(Value)
{
}

FFXFloatParam::FFXFloatParam(const FName& ParameterName, float Value) : ParameterName(ParameterName), Value(Value)
{
}

FFXVectorParam::FFXVectorParam(const FName& ParameterName, const FVector& Value)
	: ParameterName(ParameterName)
	, Value(Value)
{
}

bool FFXEffectData::operator==(const FFXEffectData& FXData) const
{
	return FXEnemy == FXData.FXEnemy && FXAlly == FXData.FXAlly;
}

bool FFXEffectData::operator==(const UFXSystemAsset* FXSystemAsset) const
{
	return FXAlly == FXSystemAsset || FXEnemy == FXSystemAsset;
}

bool FFXEffectData::operator==(UFXSystemComponent const* FXSystemComponent) const
{
	return IsValid(FXSystemComponent) && *this == FXSystemComponent->GetFXSystemAsset();
}

bool FFXEffectData::IsEmpty() const
{
	return !FXAlly && !FXEnemy;
}

bool FFXEffectDataSimple::operator==(FFXEffectDataSimple const& FXData) const
{
	return FXSystem == FXData.FXSystem;
}

bool FFXEffectDataSimple::operator==(UFXSystemAsset const* FXSystemAsset) const
{
	return FXSystem == FXSystemAsset;
}

bool FFXEffectDataSimple::operator==(UFXSystemComponent const* FXSystemComponent) const
{
	return IsValid(FXSystemComponent) && *this == FXSystemComponent->GetFXSystemAsset();
}

FFXEffectDataAttached::FFXEffectDataAttached(const FFXEffectData& InFXData) : FXData(InFXData)
{
}

bool FFXEffectDataAttached::operator==(FFXEffectDataAttached const& FXDataAttached) const
{
	return FXData == FXDataAttached.FXData;
}

bool FFXEffectDataAttached::operator==(UFXSystemAsset const* FXSystemAsset) const
{
	return FXData == FXSystemAsset;
}

bool FFXEffectDataAttached::operator==(UFXSystemComponent const* FXSystemComponent) const
{
	return IsValid(FXSystemComponent) && *this == FXSystemComponent->GetFXSystemAsset();
}

bool FFXEffectDataAttached::IsEmpty() const
{
	return FXData.IsEmpty();
}
