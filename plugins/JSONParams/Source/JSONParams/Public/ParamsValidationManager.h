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

namespace ParamsValidation
{
	using FParamValidateFunction = bool (*)(uint8*);

	class JSONPARAMS_API FRegistrator
	{
	public:
		FRegistrator(const FString& ParamTypeName, FParamValidateFunction ValidateFunction);
	};

	class JSONPARAMS_API FParamsValidationManager
	{
		friend class FRegistrator;

	public:
		static FParamsValidationManager& Get();
		bool ValidateParam(const FString& ParamTypeName, uint8* Data);

	private:
		void AddValidateFunction(const FString& ParamTypeName, FParamValidateFunction ValidateFunction);

		TMap<FString, FParamValidateFunction> ParamValidateFunctions;
	};
}
