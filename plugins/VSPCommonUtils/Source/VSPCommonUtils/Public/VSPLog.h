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

#include "Details/VSPLogDetails.h"
#include "FileCategory.h"

/**
 * @brief     VSP project version of UE_LOG. Prints the user's message to the log file and console.
 * @details   Has the following advantages over the default macro:
 *            - May be used without TEXT()
 *            - Uses VSPFormat as default formatter instead of sprintf
 *            - Checks number of passed arguments, arguments types and format string at compile time
 *            - Aligns messages to make them easier to read
 */
#define VSP_LOG_C(Category, Verbosity, Format, ...)                                                      \
	do                                                                                                  \
	{                                                                                                   \
		static constexpr const TCHAR _FormatArray[] = TEXT(Format);                                     \
		_UE_LOG(                                                                                        \
			Category,                                                                                   \
			ELogVerbosity::Verbosity,                                                                   \
			TEXT("%s"),                                                                                 \
			VSPLogDetails::FLogMessageCreator<_FormatArray>::CreateLogMessage<ELogVerbosity::Verbosity>( \
				ANSI_TO_TCHAR(__FUNCTION__),                                                            \
				ANSI_TO_TCHAR(__FILE__),                                                                \
				__LINE__,                                                                               \
				Category,                                                                               \
				FMT_COMPILE(TEXT(Format)),                                                              \
				##__VA_ARGS__));                                                                        \
	} while (false)

/**
 * @brief     VSP project version of UE_LOG. Prints the user's message to the log file and console.
 * @details   Has the following advantages over the default macro:
 *            - This overload uses category defined withVSP_DEFINE_FILE_CATEGORY() (if co category is defined, compilation error will occur)
 *            - May be used without TEXT()
 *            - Uses VSPFormat as default formatter instead of sprintf
 *            - Checks number of passed arguments, arguments types and format string at compile time
 *            - Aligns messages to make them easier to read
 */
#define VSP_LOG(Verbosity, Format, ...) \
	VSP_LOG_C(_VSP_GET_FILE_CATEGORY(FFileCategoryLocal::FFileCategoryWithoutDefault), Verbosity, Format, ##__VA_ARGS__)
