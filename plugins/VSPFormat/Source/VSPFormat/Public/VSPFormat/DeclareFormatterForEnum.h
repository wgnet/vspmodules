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

#include "EmptyFmtParse.h"

#include "Details/StartFmtIncludes.inl"

#include "ThirdParty/fmt/Public/fmt/format.h"
#include "ThirdParty/fmt/Public/fmt/xchar.h" // wchar support

#include "Details/EndFmtIncludes.inl"

#include "Containers/UnrealString.h"
#include "UObject/Class.h"
#include "UObject/NameTypes.h"
#include "UObject/ReflectedTypeAccessors.h"

#define VSP_DECLARE_FORMATTER_FOR_UENUM(EnumType)                                                                      \
	template<>                                                                                                        \
	struct fmt::formatter<EnumType, TCHAR> : VSP::EmptyFmtParse                                                        \
	{                                                                                                                 \
		template<typename FormatContext>                                                                              \
		auto format(const EnumType EnumValue, FormatContext& Ctx) const                                               \
		{                                                                                                             \
			const FString EnumStr = StaticEnum<EnumType>()->GetNameByValue(static_cast<int64>(EnumValue)).ToString(); \
			return fmt::format_to(Ctx.out(), TEXT("{}"), *EnumStr);                                                   \
		}                                                                                                             \
	}
