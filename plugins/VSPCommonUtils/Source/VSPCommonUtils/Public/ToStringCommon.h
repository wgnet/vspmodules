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

#include "VSPCheck.h"
#include "VSPVariant.h"
#include "StdX/Enum.h"

#include "CoreMinimal.h"

DEFINE_LOG_CATEGORY_STATIC(LogToStringCommon, Log, All);
VSP_DEFINE_FILE_CATEGORY(LogToStringCommon);

inline FString ToString(const int32 Value, const int32 Offset = 0)
{
	return FString::Printf(TEXT("%s%d"), *FString::ChrN(Offset, '\t'), Value);
}

template<bool Cond>
using TIntIf = typename TEnableIf<Cond, int32>::Type;

namespace Impl
{
	template<
		typename TValue,
		TIntIf<TIsSame<decltype(std::declval<const TValue>().ToString(std::declval<int32>())), FString>::Value> = 0>
	FString TryToString(const TValue& Value, const int32 Offset, const int32)
	{
		return Value.ToString(Offset);
	}

	template<
		typename TValue,
		TIntIf<TIsSame<decltype(ToString(std::declval<const TValue>(), std::declval<int32>())), FString>::Value> = 0>
	FString TryToString(const TValue& Value, const int32 Offset, const int32)
	{
		return ToString(Value, Offset);
	}

	template<typename TValue>
	FString TryToString(const TValue& Value, const int32 Offset, ...)
	{
		return FString::Printf(TEXT("%s<No-ToString>"), *FString::ChrN(Offset, '\t'));
	}
}

template<typename TValue>
FString TryToString(const TValue& Value, const int32 Offset = 0)
{
	return Impl::TryToString(Value, Offset, 0);
}

// -------------------------

//TODO: Use SFINAE to resolve this template calls
template<typename TValue>
FString PtrToString(const TValue* Value, const int32 Offset = 0)
{
	return Value ? TryToString(*Value, Offset) : FString::Printf(TEXT("%s<None>"), *FString::ChrN(Offset, '\t'));
}

template<typename TValue>
FString PtrToString(TValue* Value, const int32 Offset = 0)
{
	return PtrToString(const_cast<const TValue*>(Value), Offset);
}

template<typename TValue>
FString ToString(const TWeakObjectPtr<TValue>& Value, const int32 Offset = 0)
{
	return Value.IsValid() ? TryToString(*Value) : FString::Printf(TEXT("%s<None>"), *FString::ChrN(Offset, '\t'));
}

inline FString ToString(const FSoftObjectPath& Value, const int32 Offset = 0)
{
	const FString PathString = Value.ToString();
	return FString::Printf(
		TEXT("%s%s"),
		*FString::ChrN(Offset, '\t'),
		PathString.IsEmpty() ? TEXT("<None>") : *PathString);
}

template<typename TValue>
inline FString ToString(const TSoftObjectPtr<TValue>& Value, const int32 Offset = 0)
{
	const FString ValueString = Value.ToString();
	return FString::Printf(
		TEXT("%s%s"),
		*FString::ChrN(Offset, '\t'),
		ValueString.IsEmpty() ? TEXT("<None>") : *ValueString);
}

// -------------------------

template<typename TValue, typename TAllocator>
FString ToString(
	const TArray<TValue, TAllocator>& Value,
	const TFunction<FString(TValue, int32)>& ToStringLambda,
	const int32 Offset)
{
	FString Result;
	const FString OffsetString = FString::ChrN(Offset, '\t');
	static const FString OpeningString = TEXT("[");
	static const FString ClosingString = TEXT("]");
	static const FString EmptyString = TEXT("(EmptyArray)");

	if (Value.Num() > 0)
	{
		Result.Appendf(TEXT("%s%s\n"), *OffsetString, *OpeningString);

		for (const TValue& Element : Value)
			Result.Appendf(TEXT("%s\n"), *ToStringLambda(Element, Offset + 1));

		Result.Appendf(TEXT("%s%s"), *OffsetString, *ClosingString);
	}
	else
	{
		Result = FString::Printf(TEXT("%s%s"), *OffsetString, *EmptyString);
	}

	return Result;
}

template<typename TValue, typename TAllocator>
FString ToString(const TArray<TValue, TAllocator>& Value, const int32 Offset = 0)
{
	auto DefaultToStringLambda = [](TValue Value, const int32 Offset) -> FString
	{
		return TryToString(Value, Offset);
	};
	return ToString<TValue>(Value, DefaultToStringLambda, Offset);
}

template<typename TypeKey, typename TypeValue>
FString ToString(const TMap<TypeKey, TypeValue>& Value, const int32 Offset = 0)
{
	FString Result;

	for (const TPair<TypeKey, TypeValue>& Element : Value)
		Result.Appendf(
			TEXT("%s\n") TEXT("%s:\n") TEXT("%s\n"),
			*TryToString(Element.Key, Offset),
			*FString::ChrN(Offset, '\t'),
			*TryToString(Element.Value, Offset + 1));

	Result.RemoveAt(Result.Len() - 1); //Remove last new line

	return Result;
}

template<typename Type>
FString ToString(const TOptional<Type>& Optional, const int32 Offset = 0)
{
	const FString OffsetString { FString::ChrN(Offset, '\t') };
	if (Optional.IsSet())
	{
		return FString::Printf(TEXT("%svalue:\n") TEXT("%s"), *OffsetString, *TryToString(Optional.GetValue(), Offset));
	}
	else
	{
		return FString::Printf(TEXT("%s{unset}"), *OffsetString);
	}
}

// ------------------------------------

template<typename T>
void ToStringProperty(TMap<FString, FString>& StringProperties, const FString& PropertyName, const T& Value)
{
	StringProperties.Add(PropertyName, ToString(Value));
}

// ------------------------------------

template<typename EnumType>
class TEnumToStringMapping
{
public:
	TEnumToStringMapping(const std::initializer_list<TPairInitializer<const FString&, const EnumType&>>& InitList)
		: Mapping(InitList)
	{
	}

	TEnumToStringMapping(
		const std::initializer_list<TPairInitializer<const FString&, const EnumType&>>& InitList,
		const EnumType DefaultFromStringValue,
		const FString& EnumTypeName)
		: Mapping(InitList)
		, DefaultFromStringValue(DefaultFromStringValue)
		, EnumTypeName(EnumTypeName)
	{
	}

	const FString& ToString(const EnumType Enum) const
	{
		static const FString Dummy;
		const FString* Result = Mapping.FindKey(Enum);
		VSPCheckReturnF(
			Result,
			*FString::Printf(TEXT("Enum value [%d] has no mapping"), StdX::ToUnderlying(Enum)),
			Dummy);
		return *Result;
	}

	FString ToString(const EnumType Enum, const int32 Offset) const
	{
		return FString::Printf(TEXT("%s%s"), *FString::ChrN(Offset, '\t'), *ToString(Enum));
	}

	EnumType FromString(const FString& String) const
	{
		const EnumType* Result = Mapping.Find(String);
		VSPCheckReturnF(
			Result,
			*FString::Printf(
				TEXT("Could not find Enum of type %s by string %s. Used %s instead as a fallback."),
				*EnumTypeName,
				*String,
				*ToString(DefaultFromStringValue.Get(EnumType {}))),
			DefaultFromStringValue.Get(EnumType {}));
		return *Result;
	}

private:
	const TMap<FString, EnumType> Mapping;
	const TOptional<EnumType> DefaultFromStringValue;
	const FString EnumTypeName;
};

// ------------------------------------

template<typename... Types>
FString ToString(const TVSPVariant<Types...>& Variant, const int32 Offset = 0)
{
	FString VariantString;

	const int32 _[] { Variant.template IsType<Types>()
						  ? (VariantString = TryToString(Variant.template Get<Types>(), Offset + 1), 0)
						  : 0 ... };

	const FString OffsetString { FString::ChrN(Offset, '\t') };
	return FString::Printf(TEXT("%sVariant:") TEXT("%s"), *OffsetString, *VariantString);
}
