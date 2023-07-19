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

#include "Logging/LogVerbosity.h"
#include "VSPFormat.h"

namespace VSPLogDetails
{
	enum class EFormatStringCheckResult
	{
		Success,
		NumClosingGreaterNumOpening,
		NumOpeningGreaterNumClosing,
		NumBracesGreaterNumArgs
	};

	template<unsigned NumArgs>
	constexpr EFormatStringCheckResult CheckFormatString(const TCHAR* Format)
	{
		int32 BracesBalance = 0;
		uint32 NumBracesPairs = 0;
		for (int32 i = 0; Format[i] != TEXT('\0'); ++i)
		{
			if (Format[i] == TEXT('{'))
			{
				if (Format[i + 1] == TEXT('{'))
				{
					++i;
					continue;
				}

				++BracesBalance;
			}
			else if (Format[i] == TEXT('}'))
			{
				if (Format[i + 1] == TEXT('}'))
				{
					++i;
					continue;
				}

				if (BracesBalance > 0)
					++NumBracesPairs;
				else
					return EFormatStringCheckResult::NumClosingGreaterNumOpening;

				--BracesBalance;
			}
		}

		if (BracesBalance > 0)
			return EFormatStringCheckResult::NumOpeningGreaterNumClosing;

		if (NumArgs > NumBracesPairs)
			return EFormatStringCheckResult::NumBracesGreaterNumArgs;

		return EFormatStringCheckResult::Success;
	}

	template<ELogVerbosity::Type Verbosity>
	constexpr int32 GetVerbosityLength()
	{
		int32 VerbosityLength = 0;
		switch (Verbosity)
		{
		case ELogVerbosity::Fatal:
		case ELogVerbosity::Error:
			VerbosityLength = 5;
			break;

		case ELogVerbosity::Warning:
		case ELogVerbosity::Display:
		case ELogVerbosity::Verbose:
			VerbosityLength = 7;
			break;

		case ELogVerbosity::VeryVerbose:
			VerbosityLength = 11;
			break;

		default:
			// no ELogVerbosity::Log because it doesn't show up in the log
			break;
		}

		if (VerbosityLength != 0)
			VerbosityLength += 2; // ": "

		return VerbosityLength;
	}

	inline static FString& GetMessageBuffer()
	{
		// optimization to reduce number of allocations
		// safe until only CreateLogMessage() use it
		thread_local FString MessageBuffer;
		MessageBuffer.Reset();
		return MessageBuffer;
	}

	template<const TCHAR Format[]>
	struct FLogMessageCreator
	{
		template<ELogVerbosity::Type Verbosity, class FmtFormatType, class... ArgsType>
		static const TCHAR* CreateLogMessage(
			const TCHAR* Function,
			const TCHAR* File,
			int Line,
			const FLogCategoryBase& LogCategory,
			const FmtFormatType& FmtFormat,
			ArgsType&&... Args)
		{
			constexpr EFormatStringCheckResult FormatStringCheckResult =
				VSPLogDetails::CheckFormatString<sizeof...(ArgsType)>(Format);
			static_assert(
				FormatStringCheckResult != EFormatStringCheckResult::NumClosingGreaterNumOpening,
				"Format string error: num of closing braces > num of opening braces at some point of format string");
			static_assert(
				FormatStringCheckResult != EFormatStringCheckResult::NumOpeningGreaterNumClosing,
				"Format string error: num of opening braces > num of closing braces at the end of format string");
			static_assert(
				FormatStringCheckResult != EFormatStringCheckResult::NumBracesGreaterNumArgs,
				"Format string error: num of args > num of braces pairs");

			// alignment is the number of characters from UE frame counter to the corresponding column
			constexpr int32 FrameCounterAlignment = 30;
			constexpr int32 UserMessageAlignment = 90;
			constexpr int32 FileLineAlignment = 190;

			FString& MessageString = GetMessageBuffer();

			// UE appends category and verbosity, but we can calc length to align properly
			const int32 CategoryAndVerbosityLength =
				LogCategory.GetCategoryName().GetStringLength() + GetVerbosityLength<Verbosity>() + 2; // ": "

			auto AppendAligned =
				[&MessageString](int32 CurrentLen, int32 AlignLen, int32 MinSpaces, auto&&... FormatArgs)
			{
				const int32 NumSpacesToAppend = FMath::Max(AlignLen - CurrentLen, MinSpaces);
				for (int32 i = 0; i < NumSpacesToAppend; ++i)
					MessageString.AppendChar(TEXT(' '));

				_Detail_V_S_P_F_M_T::FormatTo(MessageString, Forward<decltype(FormatArgs)>(FormatArgs)...);
			};

			// append editor id and frame counter
			AppendAligned(CategoryAndVerbosityLength, FrameCounterAlignment, 0, TEXT("["));
#if WITH_EDITOR
			VSPFormatTo(MessageString, "{:>2}:", GPlayInEditorID);
#endif
			VSPFormatTo(MessageString, "{:0>6}] ", GFrameCounter);

			// append Function with replacing constructions like <lambda_5d8098dae58bb6b0cd6ed955849fa7da> with <lambda>
			for (const TCHAR* Symbol = Function; *Symbol != TEXT('\0'); ++Symbol)
			{
				if (FCString::Strncmp(Symbol, TEXT("<lambda_"), 8) != 0)
				{
					MessageString += *Symbol;
				}
				else
				{
					MessageString += TEXT("<lambda>");
					const TCHAR* PossibleLambdaStart = Symbol;
					for (int32 i = 0; i < 41 && PossibleLambdaStart[i] != TEXT('\0'); ++i)
						++Symbol;
				}
			}

			// append user message
			AppendAligned(
				CategoryAndVerbosityLength + MessageString.Len(),
				UserMessageAlignment,
				1,
				FmtFormat,
				Forward<ArgsType>(Args)...);

			// append file and line number
			AppendAligned(
				CategoryAndVerbosityLength + MessageString.Len(),
				FileLineAlignment,
				1,
				TEXT("[{}:{}]"),
				File,
				Line);

			return *MessageString;
		}
	};
}

// redefine UE_LOG and TRACE_LOG_MESSAGE to fix category and verbosity usage without engine changing
// disable formatting to keep original style
// clang-format off

#if LOGTRACE_ENABLED

#define _TRACE_LOG_MESSAGE(Category, Verbosity, Format, ...) \
	static bool PREPROCESSOR_JOIN(__LogPoint, __LINE__); \
	if (!PREPROCESSOR_JOIN(__LogPoint, __LINE__)) \
	{ \
		FLogTrace::OutputLogMessageSpec(&PREPROCESSOR_JOIN(__LogPoint, __LINE__), &Category, Verbosity, __FILE__, __LINE__, Format); \
		PREPROCESSOR_JOIN(__LogPoint, __LINE__) = true; \
	} \
	FLogTrace::OutputLogMessage(&PREPROCESSOR_JOIN(__LogPoint, __LINE__), ##__VA_ARGS__);

#else

#define _TRACE_LOG_MESSAGE(Category, Verbosity, Format, ...)

#endif

#define _UE_LOG(CategoryName, Verbosity, Format, ...) \
{ \
	static_assert(TIsArrayOrRefOfType<decltype(Format), TCHAR>::Value, "Formatting string must be a TCHAR array."); \
	static_assert((Verbosity & ELogVerbosity::VerbosityMask) < ELogVerbosity::NumVerbosity && Verbosity > 0, "Verbosity must be constant and in range."); \
	CA_CONSTANT_IF((Verbosity & ELogVerbosity::VerbosityMask) <= ELogVerbosity::COMPILED_IN_MINIMUM_VERBOSITY && (ELogVerbosity::Warning & ELogVerbosity::VerbosityMask) <= CategoryName.CompileTimeVerbosity) \
	{ \
		if (!CategoryName.IsSuppressed(Verbosity) || Verbosity == ELogVerbosity::Fatal) \
		{ \
			auto UE_LOG_noinline_lambda = [](const auto& LCategoryName, const auto& LFormat, const auto&... UE_LOG_Args) FORCENOINLINE \
			{ \
				_TRACE_LOG_MESSAGE(LCategoryName, Verbosity, LFormat, UE_LOG_Args...) \
				if constexpr (Verbosity == ELogVerbosity::Fatal) \
				{ \
					FMsg::Logf_Internal(UE_LOG_SOURCE_FILE(__FILE__), __LINE__, LCategoryName.GetCategoryName(), Verbosity, LFormat, UE_LOG_Args...); \
					_DebugBreakAndPromptForRemote(); \
					FDebug::ProcessFatalError(); \
				} \
				else \
				{ \
					FMsg::Logf_Internal(nullptr, 0, LCategoryName.GetCategoryName(), Verbosity, LFormat, UE_LOG_Args...); \
				} \
			}; \
			UE_LOG_noinline_lambda(CategoryName, Format, ##__VA_ARGS__); \
			CA_ASSUME(Verbosity != ELogVerbosity::Fatal); \
		} \
	} \
}
// clang-format on
