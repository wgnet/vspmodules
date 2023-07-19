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

#include "SimpleConstCast.h"
#include "TryStaticCast.h"

#include "Misc/TVariant.h"
#include "Templates/UnrealTypeTraits.h"

// Wrapper around TVariant that adds possibility to use
//
// template<typename U>
// TVSPVariant(const U& Value)
//
// and
//
// template<typename U>
// TVSPVariant(U&& Value)
//
// additionally to other TVariant methods
template<typename T, typename... Ts>
class TVSPVariant
{
public:
	TVSPVariant();

	template<
		typename U,
		class = typename TEnableIf<!TIsSame<typename TRemoveReference<U>::Type, TVSPVariant>::Value>::Type>
	TVSPVariant(const U& Value);

	template<
		typename U,
		class = typename TEnableIf<!TIsSame<typename TRemoveReference<U>::Type, TVSPVariant>::Value>::Type>
	TVSPVariant(U&& Value);

	TVSPVariant(const TVSPVariant& Other) = default;
	TVSPVariant(TVSPVariant&& Other) = default;

	TVSPVariant& operator=(const TVSPVariant& Other) = default;
	TVSPVariant& operator=(TVSPVariant&& Other) = default;

	template<typename U>
	bool IsType() const;

	template<typename U>
	U& Get();
	template<typename U>
	const U& Get() const;

	template<typename U>
	U* TryGet();
	template<typename U>
	const U* TryGet() const;

	template<typename U>
	void Set(typename TIdentity<U>::Type&& Value);
	template<typename U>
	void Set(const typename TIdentity<U>::Type& Value);

	template<typename U, typename... TArgs>
	U& Emplace(TArgs&&... Args);

	SIZE_T GetIndex() const;

	template<typename U>
	const U* TryGetCasting() const;
	template<typename U>
	U* TryGetCasting();

	template<typename U>
	const U& GetCasting() const;
	template<typename U>
	U& GetCasting();

private:
	template<typename U, typename ArgsHead, typename... ArgsTail>
	const U* TryGetCasting_Impl() const;

	template<typename U>
	const U* TryGetCasting_Impl() const;

	using ImplementationType = TVariant<T, Ts...>;
	ImplementationType Impl;
};

#include "VSPVariant.inl"
