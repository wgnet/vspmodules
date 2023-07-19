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

template<class To>
struct CIsExplicitlyConvertible
{
	template<typename TChecked>
	auto Requires(TChecked& Value) -> decltype(static_cast<To>(Value));
};

template<typename T>
struct FVSPCheckValidator
{
	using TBase = typename TRemoveReference<typename TRemovePointer<typename TRemoveCV<T>::Type>::Type>::Type;
	static constexpr bool IsUObjectBased = TIsDerivedFrom<TBase, UObject>::Value;
	static constexpr bool IsIntrovert = !IsUObjectBased;
	static constexpr bool IsBoolConvertible = TModels<CIsExplicitlyConvertible<bool>, T>::Value;

	// note: always check VSPCheck calls expected IsValid overload when adding the new one

	// UObject derived
	template<bool IsEnabled = IsUObjectBased>
	static auto IsValid(const UObject* Value) -> typename TEnableIf<IsEnabled, bool>::Type
	{
		return ::IsValid(Value);
	}
	// TWeakPtr
	template<typename TInner>
	static bool IsValid(const TWeakPtr<TInner>& Value)
	{
		return Value.IsValid();
	}
	// TWeakObjectPtr
	template<typename TInner>
	static bool IsValid(const TWeakObjectPtr<TInner>& Value)
	{
		return Value.IsValid();
	}
	// TSoftClassPtr
	template<typename TInner>
	static bool IsValid(const TSoftClassPtr<TInner>& Value)
	{
		return !Value.IsNull();
	}
	// TSoftObjectPtr
	template<typename TInner>
	static bool IsValid(const TSoftObjectPtr<TInner>& Value)
	{
		return !Value.IsNull();
	}
	// FSoftClassPath
	static bool IsValid(const FSoftClassPath& Value)
	{
		return !Value.IsNull();
	}
	// FSoftObjectPath
	static bool IsValid(const FSoftObjectPath& Value)
	{
		return !Value.IsNull();
	}
	// Common: any bool convertible type
	template<typename TInner, bool IsEnabled = IsBoolConvertible&& IsIntrovert>
	static auto IsValid(const TInner& Value) -> typename TEnableIf<IsEnabled, bool>::Type
	{
		return static_cast<bool>(Value);
	}
};
