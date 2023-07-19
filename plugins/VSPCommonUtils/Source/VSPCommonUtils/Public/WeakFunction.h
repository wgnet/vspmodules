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
#include "Templates/SharedPointer.h"

template<class T>
using TEnableIfUObject = typename TEnableIf<TIsDerivedFrom<T, UObject>::IsDerived, void*>::Type;

struct CIsDerivedFromSharedFromThis
{
	template<class T>
	auto Requires(T& Value) -> decltype(Value.AsShared());
};

template<class T>
using TEnableIfSharedFromThis = typename TEnableIf<TModels<CIsDerivedFromSharedFromThis, T>::Value, void*>::Type;

/**
    @brief  Create weak function which will be executed only if Object is alive (non-void return type overload)
    @param  Object            - UObject to track
    @param  InFunction        - function to execute
    @param  InFailureReturn   - return value to use in InFunction if Object is dead
    @param  InDeadWeakIsError - treat dead Object as error and throw VSPCheck
    @retval                   - weak function
**/
template<class UObjectType, class CallableType, class ReturnType>
auto MakeWeakFunction(
	UObjectType* Object,
	CallableType InFunction,
	ReturnType&& InFailureReturn,
	bool InDeadWeakIsError = true,
	TEnableIfUObject<UObjectType> = nullptr);

/**
    @brief  Create weak function which will be executed only if Object is alive (void return type overload)
    @param  Object            - UObject to track
    @param  InFunction        - function to execute
    @param  InDeadWeakIsError - treat dead Object as error and throw VSPCheck
    @retval                   - weak function
**/
template<class UObjectType, class CallableType>
auto MakeWeakFunction(
	UObjectType* Object,
	CallableType InFunction,
	bool InDeadWeakIsError = true,
	TEnableIfUObject<UObjectType> = nullptr);

/**
    @brief  Create weak function which will be executed only if Object is alive (non-void return type overload)
    @param  Object            - pointer to object of type inherited of TSharedFromThis to track
    @param  InFunction        - function to execute
    @param  InFailureReturn   - return value to use in InFunction if Object is dead
    @param  InDeadWeakIsError - treat dead Object as error and throw VSPCheck
    @retval                   - weak function
**/
template<class SharedFromThisObjectType, class CallableType, class ReturnType>
auto MakeWeakFunction(
	SharedFromThisObjectType* Object,
	CallableType InFunction,
	ReturnType&& InFailureReturn,
	bool InDeadWeakIsError = true,
	TEnableIfSharedFromThis<SharedFromThisObjectType> = nullptr);

/**
    @brief  Create weak function which will be executed only if Object is alive (void return type overload)
    @param  Object            - pointer to object of type inherited of TSharedFromThis to track
    @param  InFunction        - function to execute
    @param  InDeadWeakIsError - treat dead Object as error and throw VSPCheck
    @retval                   - weak function
**/
template<class SharedFromThisObjectType, class CallableType>
auto MakeWeakFunction(
	SharedFromThisObjectType* Object,
	CallableType InFunction,
	bool InDeadWeakIsError = true,
	TEnableIfSharedFromThis<SharedFromThisObjectType> = nullptr);

#include "WeakFunction.inl"
