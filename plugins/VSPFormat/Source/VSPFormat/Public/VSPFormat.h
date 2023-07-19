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

#include "VSPFormat/CommonFormatters.h"

/**
 * @brief   Formats all arguments in passed string.
 * @details https://fmt.dev/9.0.0/syntax.html
 * @example VSPFormatTest.cpp
 * 
 * @param  FormatStr - format string
 * @param  ...       - arguments for formatting
 * @return Formatted corresponding to the syntax string
 *
 * @code
 * FString Answer = VSPFormat("Number is {}", 42);
 * @endcode
 */
#define VSPFormat(FormatStr, ...) _Detail_V_S_P_F_M_T::Format(FMT_COMPILE(TEXT(FormatStr)), __VA_ARGS__)

/**
 * @brief   Formats all arguments in passed string and appends to the given DestinationStr
 * @details https://fmt.dev/9.0.0/syntax.html
 *
 * @param DestinationStr - string to append to
 * @param FormatStr      - format string
 * @param ...            - arguments for formatting
 *
 * @code
 * FString Answer = TEXT("Number is :");
 * VSPFormatTo(Answer, "{}", 42);
 * @endcode 
 */
#define VSPFormatTo(DestinationStr, FormatStr, ...) \
	_Detail_V_S_P_F_M_T::FormatTo(DestinationStr, FMT_COMPILE(TEXT(FormatStr)), __VA_ARGS__)
