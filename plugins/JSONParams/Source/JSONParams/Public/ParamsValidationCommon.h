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

#include "ParamsValidationManager.h"

#define _VSP_MAKE_LAMBDA_VALIDATE_FUNCTION(ParamType)                      \
	[](uint8* Data) -> bool                                               \
	{                                                                     \
		auto ParamDataPtr = reinterpret_cast<ParamType*>(Data);           \
		return ParamDataPtr && ParamsValidation::Validate(*ParamDataPtr); \
	}

namespace ParamsValidation
{
	template<typename TParamType>
	bool Validate(const TParamType& Param)
	{
		return true;
	}
}

/**
 * This define allows to define and register in ParamsValidationManager validation function for ParamType class.
 * It provides "const ParamType& Param" variable to validate.
 * Real signature is "bool Validate(const ParamType& Param)".
 * 
 * Usage sample:
 * USTRUCT()
 * struct FHealBonus 
 * {
 *		GENERATED_BODY()
 *		int Value = 1;
 * }
 * 
 *VSP_DEFINE_PARAM_VALIDATION_FUNCTION(FHealBonus)
 * {
 *		// Param type is "FHealBonus".
 *		return Param.Value > 0;
 * }
 **/
#define VSP_DEFINE_PARAM_VALIDATION_FUNCTION(ParamType)                         \
	template<>                                                                 \
	inline bool ParamsValidation::Validate<ParamType>(const ParamType& Param); \
	namespace                                                                  \
	{                                                                          \
		ParamsValidation::FRegistrator Registrator##ParamType(                 \
			#ParamType,                                                        \
			_VSP_MAKE_LAMBDA_VALIDATE_FUNCTION(ParamType));                     \
	}                                                                          \
	template<>                                                                 \
	inline bool ParamsValidation::Validate<ParamType>(const ParamType& Param)

struct FParamsValidationTestClass
{
	int Value = 0;
};

VSP_DEFINE_PARAM_VALIDATION_FUNCTION(FParamsValidationTestClass)
{
	return Param.Value == 0;
}
