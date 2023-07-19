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

#include "VSPFormatBackInserter.h"

#include "VSPFormat/Details/StartFmtIncludes.inl"

#include "ThirdParty/fmt/Public/fmt/compile.h"
#include "ThirdParty/fmt/Public/fmt/xchar.h" // wchar support

#include "VSPFormat/Details/EndFmtIncludes.inl"

#include "Containers/UnrealString.h"

namespace VSP
{
	/**
	* @brief - This is wrapper around a pointer to format it in a safe way.
	* 
	* fmt library does not support formatting of pointers, so u CAN'T define fmt::formatter<MyObject*>
	* If you want to format MyObject* and it might be nullptr, you need to write:
	*
	* MyObject * ObjectPointer = GetObjectPointer();
	* VSPFormat("My object is {}", VSP::FmtPtr(ObjectPointer));
	*
	* @note This is happening automatically in VSPFormat macro, so you don't have to do it manually  
	* 
	* @tparam T - Type of pointer to a object
	*/
	template<class T>
	struct PtrWrapper
	{
		static_assert(TIsPointer<T>::Value, "This is a pointer wrapper to VSPFormat it in a safe way");
		T Pointer;
	};

	template<class T>
	PtrWrapper<T> FmtPtr(T Pointer)
	{
		return PtrWrapper<T> { Pointer };
	}
}

namespace _Detail_V_S_P_F_M_T // dirty namespace to prevent Rider's code completion then typing VSPF.. and use tab
{
	template<typename T>
	struct TFmtFormattablePtr
	{
		static constexpr bool Value = false;
	};

	template<>
	struct TFmtFormattablePtr<void>
	{
		static constexpr bool Value = true;
	};

	template<>
	struct TFmtFormattablePtr<TCHAR>
	{
		static constexpr bool Value = true;
	};

	template<typename T>
	struct TShouldMakePointerWrapper
	{
		using DecayedType = typename TDecay<T>::Type;
		using RawType = typename TDecay<typename TRemovePointer<typename TDecay<T>::Type>::Type>::Type;

		static constexpr bool Value = TIsPointer<DecayedType>::Value && !TFmtFormattablePtr<RawType>::Value;
	};

	template<class T, typename TEnableIf<TShouldMakePointerWrapper<T>::Value>::Type* = nullptr>
	auto TransformArg(T&& Arg)
	{
		return VSP::FmtPtr(Arg);
	}

	template<class T, typename TEnableIf<!TShouldMakePointerWrapper<T>::Value>::Type* = nullptr>
	decltype(auto) TransformArg(T&& Arg)
	{
		return Forward<T>(Arg);
	}

	template<class StringT, class... FormatArgs>
	void FormatTo(FString& Dest, const StringT& Format, FormatArgs&&... Args)
	{
		fmt::format_to(BackInserter(Dest), Format, TransformArg(Forward<FormatArgs>(Args))...);
	}

	/**
	 * @brief   Formats all arguments in passed string.
	 * @example VSPFormatTest.cpp
	 * 
	 * @param  Format - string to format. Example: VSP::Format(TEXT("Number is {}"), 42);
	 * @param  Args   - arguments to format in passed Format string
	 * @return Formatted corresponding to the syntax string (@see https://fmt.dev/9.0.0/syntax.html)
	 *
	 * @warning DO NOT USE DIRECTLY because of different types of characters and absence of compile time checks
	 */
	template<class StringT, class... FormatArgs>
	FString Format(const StringT& Format, FormatArgs&&... Args)
	{
		FString Buffer;
		FormatTo(Buffer, Format, Forward<FormatArgs>(Args)...);
		return Buffer;
	}
}
