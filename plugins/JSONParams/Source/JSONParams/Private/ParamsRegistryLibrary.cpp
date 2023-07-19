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
#include "ParamsRegistryLibrary.h"

#include "JsonObjectConverter.h"
#include "ParamsRegistry.h"


TMap<FString, UObject*> UParamsRegistryLibrary::GetRegisteredTypes()
{
	TMap<FString, UObject*> Result;
	for (UScriptStruct* Type : FParamsRegistry::Get().GetRegisteredTypes())
	{
		Result.Add(Type->GetName(), Type);
	}
	return Result;
}

TArray<FName> UParamsRegistryLibrary::GetRegisteredNames(UObject* Type)
{
	if (const UScriptStruct* RealType = Cast<UScriptStruct>(Type))
		return FParamsRegistry::Get().GetRegisteredNames(RealType);
	return {};
}

FString UParamsRegistryLibrary::GetParamFieldsString(UObject* Type, const FName& Name)
{
	const UScriptStruct* RealType = Cast<UScriptStruct>(Type);
	if (!RealType)
		return "Type are invalid!";
	const FParamRegistryDataPtr Param = FParamsRegistry::GetParam(RealType, Name);
	if (!Param)
		return "Param not found!";

	const FString HeaderTemplate = TEXT("Param '{0}' [{1}]:\n");
	const FString Header = FString::Format(*HeaderTemplate, { RealType->GetName(), Name.ToString() });
	FString Data;
	FJsonObjectConverter::UStructToJsonObjectString(RealType, Param->Data.GetData(), Data, 0, 0, 0, nullptr, true);
	return Header + Data;
}

bool UParamsRegistryLibrary::IsEqual(const FParamRegistryInfo& ParamA, const FParamRegistryInfo& ParamB)
{
	return ParamA == ParamB;
}

bool UParamsRegistryLibrary::IsValid(const FParamRegistryInfo& ParamInfo)
{
	return ParamInfo.IsValid();
}

bool UParamsRegistryLibrary::GetUParam(const FParamRegistryInfo& Param, uint8& OutStruct)
{
	// We should never hit this!  stubs to avoid NoExport on the class.
	check(false);
	return false;
}

DEFINE_FUNCTION(UParamsRegistryLibrary::execGetUParam)
{
	P_GET_STRUCT_REF(FParamRegistryInfo, ParamInfo);

	Stack.StepCompiledIn<FStructProperty>(NULL);
	void* OutStructPtr = Stack.MostRecentPropertyAddress;

	P_FINISH;

	P_NATIVE_BEGIN;

	if (!ParamInfo.Type)
	{
		const FString ErrFormat = "Invalid Param type: Type={0}";
		const FString ErrMsg = FString::Format(*ErrFormat, { ParamInfo.Name.ToString() });
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			FText::FromString(ErrMsg));
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*(bool*)RESULT_PARAM = false;
		return;
	}

	const FStructProperty* OutStructProp = CastField<FStructProperty>(Stack.MostRecentProperty);
	if (!OutStructProp || !OutStructPtr)
	{
		const FString ErrFormat = "Invalid output struct: Type={0}";
		const FString ErrMsg = FString::Format(*ErrFormat, { OutStructProp->Struct->GetName() });
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			FText::FromString(ErrMsg));
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*(bool*)RESULT_PARAM = false;
		return;
	}

	const bool bCompatible = (OutStructProp->Struct == ParamInfo.Type)
		|| (OutStructProp->Struct->IsChildOf(ParamInfo.Type)
			&& FStructUtils::TheSameLayout(OutStructProp->Struct, ParamInfo.Type));
	if (!bCompatible)
	{
		const FString ErrFormat = "Type of Param didn't match output struct type: Param Type={0}, Out Type={1}";
		const FString ErrMsg =
			FString::Format(*ErrFormat, { ParamInfo.Type->GetName(), OutStructProp->Struct->GetName() });
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			FText::FromString(ErrMsg));
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*(bool*)RESULT_PARAM = false;
		return;
	}

	const FParamRegistryDataPtr Param = FParamsRegistry::GetParam(ParamInfo);
	if (!Param)
	{
		const FString ErrFormat = "Can't find Param: Type={0}, Name={1}";
		const FString ErrMsg = FString::Format(*ErrFormat, { ParamInfo.Type->GetName(), ParamInfo.Name.ToString() });
		const FBlueprintExceptionInfo ExceptionInfo(
			EBlueprintExceptionType::AccessViolation,
			FText::FromString(ErrMsg));
		FBlueprintCoreDelegates::ThrowScriptException(P_THIS, Stack, ExceptionInfo);
		*(bool*)RESULT_PARAM = false;
		return;
	}

	OutStructProp->CopyCompleteValueFromScriptVM(OutStructPtr, Param->Data.GetData());

	*(bool*)RESULT_PARAM = true;

	P_NATIVE_END;
}
