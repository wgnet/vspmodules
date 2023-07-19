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
#include "VSPFormat/Details/FormatDetail.h"

#include "CoreMinimal.h"
#include "Misc/TVariant.h"

namespace fmt
{
	template<class PointerType>
	struct formatter<VSP::PtrWrapper<PointerType>, TCHAR> : VSP::EmptyFmtParse
	{
		template<typename FormatContext>
		auto format(const VSP::PtrWrapper<PointerType> PointerWrapper, FormatContext& Ctx) const
		{
			if (PointerWrapper.Pointer == nullptr)
				return format_to(Ctx.out(), TEXT("{}"), TEXT("nullptr"));

			if constexpr (TIsDerivedFrom<typename TRemovePointer<PointerType>::Type, UObject>::Value)
				if (!IsValid(PointerWrapper.Pointer))
					return format_to(Ctx.out(), TEXT("{}"), TEXT("Invalid"));

			return format_to(Ctx.out(), TEXT("{}"), *PointerWrapper.Pointer);
		}
	};

	//-------------------------------------- String like types
	template<>
	struct formatter<FString, TCHAR> : public VSP::EmptyFmtParse
	{
		template<typename FormatContext>
		auto format(const FString& UnrealString, FormatContext& Ctx) const
		{
			return format_to(Ctx.out(), TEXT("{}"), *UnrealString);
		}
	};

	template<>
	struct formatter<FName, TCHAR> : public VSP::EmptyFmtParse
	{
		template<typename FormatContext>
		auto format(const FName Name, FormatContext& Ctx) const
		{
			// Maybe there is a better way?
			return format_to(Ctx.out(), TEXT("{}"), Name.ToString());
		}
	};

	template<>
	struct formatter<FText, TCHAR> : public VSP::EmptyFmtParse
	{
		template<typename FormatContext>
		auto format(const FText& Text, FormatContext& Ctx) const
		{
			// Maybe there is a better way?
			return format_to(Ctx.out(), TEXT("{}"), Text.ToString());
		}
	};

	//-------------------------------------- Math types
	template<>
	struct formatter<FVector, TCHAR> : public VSP::EmptyFmtParse
	{
		template<typename FormatContext>
		auto format(const FVector Vec, FormatContext& Ctx) const
		{
			return format_to(Ctx.out(), TEXT("({},{},{})"), Vec.X, Vec.Y, Vec.Z);
		}
	};

	//-------------------------------------- Utility types

	// NOTE: requires to define formatter for every type in Variant<...Types>
	//TODO remove then TVSPVariant will support Visitor
	template<class... Types>
	struct formatter<TVariant<Types...>, TCHAR> : public VSP::EmptyFmtParse
	{
		template<typename FormatContext>
		auto format(const TVariant<Types...>& Variant, FormatContext& Ctx) const
		{
			return Visit(
				[&Ctx](const auto& StoredTypeInVariant)
				{
					return fmt::format_to(Ctx.out(), TEXT("{}"), StoredTypeInVariant);
				},
				Variant);
		}
	};

	//-------------------------------------- UObject types

	// This is default formatter for all classes, inherited from UObject
	template<class UObjectT>
	struct formatter<UObjectT, typename TEnableIf<TIsDerivedFrom<UObjectT, UObject>::Value, TCHAR>::Type>
		: public VSP::EmptyFmtParse
	{
		//TODO custom parse to allow formatting using UE reflection?

		template<typename FormatContext>
		auto format(const UObject& Object, FormatContext& Ctx) const
		{
			return format_to(Ctx.out(), TEXT("{}"), Object.GetName());
		}
	};
}
