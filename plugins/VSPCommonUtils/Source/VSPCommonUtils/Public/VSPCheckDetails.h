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
#include "VSPCheckFailCounter.h"
#include "VSPCheckValidator.h"
#include "VSPPerfTrace.h"

VSP_PERF_SCOPE_META_BEGIN_MODULE_EXTERN(VSPCOMMONUTILS_API, VSPCheckDebuggerBreak)
VSP_PERF_SCOPE_META_FIELD(Trace::WideString, File)
VSP_PERF_SCOPE_META_FIELD(Trace::WideString, Function)
VSP_PERF_SCOPE_META_FIELD(int32, Line)
VSP_PERF_SCOPE_META_FIELD(Trace::WideString, Condition)
VSP_PERF_SCOPE_META_FIELD(Trace::WideString, UserMessage)
VSP_PERF_SCOPE_META_END()

namespace VSPCheckDetails
{
	VSPCOMMONUTILS_API FString GetMessage(
		const TCHAR* File,
		const TCHAR* Function,
		int32 Line,
		const TCHAR* Condition,
		const FString& UserMessage);

	VSPCOMMONUTILS_API void LogToScreen(const FString& Message);

	VSPCOMMONUTILS_API void PrintCallStackToLog(
		const TCHAR* InFile,
		const TCHAR* InFunction,
		int32 InLine,
		const TCHAR* InCondition,
		const TCHAR* InUserMessage);
};

#define _VSPCheckCF(InCondition, InCategory, InUserMessage)                                                         \
	(LIKELY(FVSPCheckValidator<decltype(InCondition)>::IsValid(InCondition)) ||                                     \
	 [&]()                                                                                                         \
	 {                                                                                                             \
		 const FString _File = ANSI_TO_TCHAR(__FILE__);                                                            \
		 const FString _Function = ANSI_TO_TCHAR(__FUNCTION__);                                                    \
		 const auto _Line = __LINE__;                                                                              \
		 const auto _ConditionTxt = TEXT(#InCondition);                                                            \
		 const auto _MessageForLog =                                                                               \
			 VSPCheckDetails::GetMessage(*_File, *_Function, _Line, _ConditionTxt, InUserMessage);                  \
		 const auto _CheckFailCounter = UVSPCheckFailCounter::Get();                                                \
		 const auto _ShouldPrintToLog = !_CheckFailCounter || _CheckFailCounter->TestAndSetNumLogMessages();       \
		 const auto _ShouldPrintToScreen = !_CheckFailCounter || _CheckFailCounter->TestAndSetNumScreenMessages(); \
		 const auto _ShouldBreak = FPlatformMisc::IsDebuggerPresent()                                              \
			 && (!_CheckFailCounter || _CheckFailCounter->TestAndSetNumDebugBreaks());                             \
                                                                                                                   \
		 if (_ShouldPrintToLog)                                                                                    \
		 {                                                                                                         \
			 /* log to file */                                                                                     \
			 _UE_LOG(InCategory, ELogVerbosity::Error, TEXT("%s"), *_MessageForLog);                               \
			 VSPCheckDetails::PrintCallStackToLog(*_File, *_Function, _Line, _ConditionTxt, InUserMessage);         \
		 }                                                                                                         \
		 if (_ShouldPrintToScreen)                                                                                 \
		 {                                                                                                         \
			 /* log to screen */                                                                                   \
			 VSPCheckDetails::LogToScreen(_MessageForLog);                                                          \
		 }                                                                                                         \
		 if (_ShouldBreak)                                                                                         \
		 {                                                                                                         \
			VSP_PERF_SCOPE_META(VSPCheckDebuggerBreak)                                                              \
			VSP_PERF_SCOPE_META_ADD(VSPCheckDebuggerBreak, File, *_File)                                            \
			VSP_PERF_SCOPE_META_ADD(VSPCheckDebuggerBreak, Function, *_Function)                                    \
			VSP_PERF_SCOPE_META_ADD(VSPCheckDebuggerBreak, Line, _Line)                                             \
			VSP_PERF_SCOPE_META_ADD(VSPCheckDebuggerBreak, Condition, _ConditionTxt)                                \
			VSP_PERF_SCOPE_META_ADD(VSPCheckDebuggerBreak, UserMessage, *_MessageForLog);                           \
                                                                                                                   \
			 PLATFORM_BREAK();                                                                                     \
		 }                                                                                                         \
                                                                                                                   \
		 return false;                                                                                             \
	 }()                                                                                                           \
	 || (!UVSPCheckFailCounter::Get() || UVSPCheckFailCounter::Get()->TestAndSetNumCatReports())                    \
	)

	// in the following macros if and else are used to make macros work fine in some cases of if/else chain
	// semicolons in the end are missing to force user to paste it yourself and make code more consistent

#define _VSPCheckReturnCF(Condition, Category, UserMessage, ...) \
	if (UNLIKELY(!VSPCheckCF(Condition, Category, UserMessage))) \
		return __VA_ARGS__;                                     \
	else                                                        \
		volatile auto __stub = ((void*)0)

#define _VSPCheckContinueCF(Condition, Category, UserMessage)    \
	if (UNLIKELY(!VSPCheckCF(Condition, Category, UserMessage))) \
		continue;                                               \
	else                                                        \
		volatile auto __stub = ((void*)0)

#define _VSPCheckBreakCF(Condition, Category, UserMessage)       \
	if (UNLIKELY(!VSPCheckCF(Condition, Category, UserMessage))) \
		break;                                                  \
	else                                                        \
		volatile auto __stub = ((void*)0)

#define _VSP_GET_CHECK_DEFAULT_CATEGORY() _VSP_GET_FILE_CATEGORY(FFileCategoryLocal::FFileCategoryWithDefault)
