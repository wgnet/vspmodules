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

template<typename T, typename... Ts>
TVSPVariant<T, Ts...>::TVSPVariant() : Impl()
{
}

template<typename T, typename... Ts>
template<typename U, class>
TVSPVariant<T, Ts...>::TVSPVariant(const U& Value) : Impl(TInPlaceType<U>(), Value)
{
}

template<typename T, typename... Ts>
template<typename U, class>
TVSPVariant<T, Ts...>::TVSPVariant(U&& Value) : Impl(TInPlaceType<typename TRemoveReference<U>::Type>(), MoveTemp(Value))
{
}

template<typename T, typename... Ts>
template<typename U>
bool TVSPVariant<T, Ts...>::IsType() const
{
	return Impl.template IsType<U>();
}

template<typename T, typename... Ts>
template<typename U>
U& TVSPVariant<T, Ts...>::Get()
{
	return Impl.template Get<U>();
}

template<typename T, typename... Ts>
template<typename U>
const U& TVSPVariant<T, Ts...>::Get() const
{
	return Impl.template Get<U>();
}

template<typename T, typename... Ts>
template<typename U>
U* TVSPVariant<T, Ts...>::TryGet()
{
	return Impl.template TryGet<U>();
}

template<typename T, typename... Ts>
template<typename U>
const U* TVSPVariant<T, Ts...>::TryGet() const
{
	return Impl.template TryGet<U>();
}

template<typename T, typename... Ts>
template<typename U>
void TVSPVariant<T, Ts...>::Set(typename TIdentity<U>::Type&& Value)
{
	Impl.Set(MoveTemp(Value));
}

template<typename T, typename... Ts>
template<typename U>
void TVSPVariant<T, Ts...>::Set(const typename TIdentity<U>::Type& Value)
{
	Impl.Set(Value);
}

template<typename T, typename... Ts>
template<typename U, typename... TArgs>
U& TVSPVariant<T, Ts...>::Emplace(TArgs&&... Args)
{
	Impl.template Emplace<U>(Forward<TArgs>(Args)...);
	return Get<U>();
}

template<typename T, typename... Ts>
SIZE_T TVSPVariant<T, Ts...>::GetIndex() const
{
	return Impl.GetIndex();
}

template<typename T, typename... Ts>
template<typename U>
const U* TVSPVariant<T, Ts...>::TryGetCasting() const
{
	return TryGetCasting_Impl<U, T, Ts...>();
}

template<typename T, typename... Ts>
template<typename U>
U* TVSPVariant<T, Ts...>::TryGetCasting()
{
	return VSPUtils::ConstCast(VSPUtils::ConstCast(this)->template TryGetCasting<U>());
}

template<typename T, typename... Ts>
template<typename U>
const U& TVSPVariant<T, Ts...>::GetCasting() const
{
	//TODO: Make static check of provided type
	const U* Result = TryGetCasting<U>();
	check(Result);
	return *Result;
}

template<typename T, typename... Ts>
template<typename U>
U& TVSPVariant<T, Ts...>::GetCasting()
{
	return VSPUtils::ConstCast(VSPUtils::ConstCast(this)->template GetCasting<U>());
}

template<typename T, typename... Ts>
template<typename U, typename ArgsHead, typename... ArgsTail>
const U* TVSPVariant<T, Ts...>::TryGetCasting_Impl() const
{
	const auto* PossibleCastedHeadValue = TryStaticCast<const U*>(TryGet<ArgsHead>());
	return PossibleCastedHeadValue ? PossibleCastedHeadValue : TryGetCasting_Impl<U, ArgsTail...>();
}

template<typename T, typename... Ts>
template<typename U>
const U* TVSPVariant<T, Ts...>::TryGetCasting_Impl() const
{
	return nullptr;
}
