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

#include "ParamsRegistry.h"

#include "ParamsRegistryLibrary.generated.h"


UCLASS()
class JSONPARAMS_API UParamsRegistryLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static TMap<FString, UObject*> GetRegisteredTypes();

	UFUNCTION(BlueprintCallable)
	static TArray<FName> GetRegisteredNames(UObject* Type);

	UFUNCTION(BlueprintCallable)
	static FString GetParamFieldsString(UObject* Type, const FName& Name);

	UFUNCTION(BlueprintPure, meta = (CompactNodeTitle = "Equal"))
	static bool IsEqual(const FParamRegistryInfo& ParamA, const FParamRegistryInfo& ParamB);

	UFUNCTION(BlueprintPure, meta = (CompactNodeTitle = "IsValid"))
	static bool IsValid(const FParamRegistryInfo& ParamInfo);

	/** Get a UParam Struct from ParamsRegistry */
	UFUNCTION(BlueprintCallable, CustomThunk, meta = (CustomStructureParam = "OutStruct", ExpandEnumAsExecs = "ReturnValue", BlueprintInternalUseOnly = "true"))
	static bool GetUParam(const FParamRegistryInfo& Param, uint8& OutStruct);
	DECLARE_FUNCTION(execGetUParam);
};
