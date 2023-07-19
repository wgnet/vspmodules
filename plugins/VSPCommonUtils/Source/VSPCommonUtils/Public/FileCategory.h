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

#include "LogVSPDefault.h"

namespace FFileCategoryLocal
{
	template<uint32 Hash>
	struct FFileCategoryWithDefault
	{
		static inline auto& LogCategory = LogVSPDefault;
	};

	template<uint32 Hash>
	struct FFileCategoryWithoutDefault;

	template<typename TChar>
	constexpr uint32 DJB2aHash(const TChar* Str, uint32 Seed)
	{
		uint32 Hash = Seed;

		for (uint32 i = 0; Str[i]; ++i)
			Hash = Hash * 33 ^ Str[i];

		return Hash;
	}
}

#define _VSP_DEFINE_FILE_CATEGORY(InSourceStruct, InLogCategory)       \
	template<>                                                        \
	struct InSourceStruct<FFileCategoryLocal::DJB2aHash(__FILE__, 0)> \
	{                                                                 \
		static inline auto& LogCategory = InLogCategory;              \
	}

#define _VSP_GET_FILE_CATEGORY(InSourceStruct) InSourceStruct<FFileCategoryLocal::DJB2aHash(__FILE__, 0)>::LogCategory

/*
	@brief Define file category
		   This category will be used inVSP_LOGs and VSPChecks
		   This category will not be exported via #include
	@param InLogCategory - category to use in this file
*/
#define VSP_DEFINE_FILE_CATEGORY(InLogCategory)                                             \
	_VSP_DEFINE_FILE_CATEGORY(FFileCategoryLocal::FFileCategoryWithDefault, InLogCategory); \
	_VSP_DEFINE_FILE_CATEGORY(FFileCategoryLocal::FFileCategoryWithoutDefault, InLogCategory)
