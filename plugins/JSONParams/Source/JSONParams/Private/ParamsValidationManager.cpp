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
#include "ParamsValidationManager.h"
#include "Utils/ParamsUtils.h"

namespace ParamsValidation
{
	FParamsValidationManager& FParamsValidationManager::Get()
	{
		static FParamsValidationManager FParamsValidationManager;
		return FParamsValidationManager;
	}

	void FParamsValidationManager::AddValidateFunction(
		const FString& ParamTypeName,
		FParamValidateFunction ValidateFunction)
	{
		if (ValidateFunction)
		{
			ParamValidateFunctions.Add(ParamTypeName, ValidateFunction);
		}
		else
		{
			UE_LOG(
				LogParams,
				Error,
				TEXT(
					"FParamsValidationManager::AddValidateFunction: trying to add nullptr as validate function for %s. Skipped."),
				*ParamTypeName);
		}
	}

	bool FParamsValidationManager::ValidateParam(const FString& ParamTypeName, uint8* Data)
	{
		bool bRes = true;

		const auto ValidateFunctionPtr = ParamValidateFunctions.Find(ParamTypeName);

		if (ValidateFunctionPtr)
		{
			bRes = (*ValidateFunctionPtr)(Data);
		}

		return bRes;
	}

	FRegistrator::FRegistrator(const FString& ParamTypeName, FParamValidateFunction ValidateFunction)
	{
		FParamsValidationManager::Get().AddValidateFunction(ParamTypeName, ValidateFunction);
	}
}
