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
#include "VSPCheckDetails.h"

VSP_PERF_SCOPE_META_DEFINE(VSPCheckDebuggerBreak)

FString VSPCheckDetails::GetMessage(
	const TCHAR* File,
	const TCHAR* Function,
	int32 Line,
	const TCHAR* Condition,
	const FString& UserMessage)
{
	return FString::Printf(
		TEXT("[%2i:%06lli] %-50s %100s [%s:%i]"),
		GPlayInEditorID,
		GFrameCounter,
		Function,
		*FString::Printf(TEXT("Failed condition '%s'. %s"), Condition, *UserMessage),
		File,
		Line);
}

void VSPCheckDetails::LogToScreen(const FString& Message)
{
	if (GEngine)
		GEngine->AddOnScreenDebugMessage(-1, 20.f, FColor::Red, *Message);
}


VSP_PERF_SCOPE_META_BEGIN(VSPCheckDumpStack)
VSP_PERF_SCOPE_META_FIELD(Trace::WideString, File)
VSP_PERF_SCOPE_META_FIELD(Trace::WideString, Function)
VSP_PERF_SCOPE_META_FIELD(int32, Line)
VSP_PERF_SCOPE_META_FIELD(Trace::WideString, Condition)
VSP_PERF_SCOPE_META_FIELD(Trace::WideString, UserMessage)
VSP_PERF_SCOPE_META_END()

void VSPCheckDetails::PrintCallStackToLog(
	const TCHAR* InFile,
	const TCHAR* InFunction,
	int32 InLine,
	const TCHAR* InCondition,
	const TCHAR* InUserMessage)
{
	VSP_PERF_SCOPE_META(VSPCheckDumpStack)
	VSP_PERF_SCOPE_META_ADD(VSPCheckDumpStack, File, InFile)
	VSP_PERF_SCOPE_META_ADD(VSPCheckDumpStack, Function, InFunction)
	VSP_PERF_SCOPE_META_ADD(VSPCheckDumpStack, Line, InLine)
	VSP_PERF_SCOPE_META_ADD(VSPCheckDumpStack, Condition, InCondition)
	VSP_PERF_SCOPE_META_ADD(VSPCheckDumpStack, UserMessage, InUserMessage);

	if (!FPlatformMisc::IsDebuggerPresent())
		FDebug::DumpStackTraceToLog(ELogVerbosity::Error);
}
