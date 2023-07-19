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
#include "FXDynamicUpdateLibrary.generated.h"

class UFXSystemComponent;
class UCurveVector;
struct FFXComponentContainer;

USTRUCT()
struct FFXDynamicUpdateParam_Float
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	UCurveFloat* DynamicUpdateCurve = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<FName> DynamicUpdateParametersName {};
};

USTRUCT()
struct FFXDynamicUpdateParam_Vector
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	UCurveVector* DynamicUpdateCurve = nullptr;

	UPROPERTY(EditDefaultsOnly)
	TArray<FName> DynamicUpdateParametersName {};
};

USTRUCT()
struct FFXDynamicUpdateParam_Emitter
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	float ValueThreshold = 0.0f;

	UPROPERTY(EditDefaultsOnly)
	bool bEnableEmitter = false;

	UPROPERTY(EditDefaultsOnly)
	TArray<FName> DynamicUpdateEmittersName {};
};

USTRUCT()
struct FFXDynamicUpdateData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly)
	TArray<FFXDynamicUpdateParam_Float> DynamicUpdateFloats {};

	UPROPERTY(EditDefaultsOnly)
	TArray<FFXDynamicUpdateParam_Vector> DynamicUpdateVectors {};

	// **NOTE**: this works only for cascade. Niagara do not implement enabling/disabling emitters for system instance.
	// See FNiagaraSystemInstance::SetEmitterEnable for possible implementation
	UPROPERTY(EditDefaultsOnly)
	TArray<FFXDynamicUpdateParam_Emitter> DynamicUpdateEmitters {};

	bool IsEmpty() const;
};

namespace FFXDynamicUpdateLibrary
{
	void UpdateFXWithNormalizedValue(
		FFXComponentContainer& FXContainer,
		const FFXDynamicUpdateData& DynamicUpdateData,
		float NormalizedValue);
	void UpdateFXWithNormalizedValue(
		UFXSystemComponent* FXComponent,
		const FFXDynamicUpdateData& DynamicUpdateData,
		float NormalizedValue);
}
