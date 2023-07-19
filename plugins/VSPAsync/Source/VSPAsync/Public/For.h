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

#include "Run.h"
#include "WhenAll.h"

#include "Templates/Invoke.h"

namespace VSPAsync
{
	/**
        @brief  Async for loop
        @tparam IndexType        - index type. for example may be an integer or iterator
        @tparam StepType         - a type for operator+= method of IndexType
        @tparam FunctionType     - input callable type. arg must be void or IndexType
        @param  First            - for loop index start value
        @param  Last             - for loop index end value
        @param  Step             - for loop index step
        @param  InFunction       - for body
        @param  InAsyncExecution - execution type
        @retval                  - a task that will be executed when all iterations are finished
    **/
	template<class IndexType, class StepType, class FunctionType>
	UE_NODISCARD TTaskPtr<> For(
		IndexType First,
		IndexType Last,
		StepType Step,
		const FunctionType& InFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief  Async for loop (no "when all" task spawned)
		@tparam IndexType        - index type. for example may be an integer or iterator
		@tparam StepType         - a type for operator+= method of IndexType
		@tparam FunctionType     - input callable type. arg must be void or IndexType
		@param  First            - for loop index start value
		@param  Last             - for loop index end value
		@param  Step             - for loop index step
		@param  InFunction       - for body
		@param  InAsyncExecution - execution type
	**/
	template<class IndexType, class StepType, class FunctionType>
	void ForNoTask(
		IndexType First,
		IndexType Last,
		StepType Step,
		const FunctionType& InFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
        @brief   Async for each loop without values copying
        @warning No input container elements copying is performed, container must be valid till algorithm completion
        @tparam  ContainerType    - input container type. must support begin() and end()
        @tparam  FunctionType     - input callable type. arg must be void or ContainerType value type
        @param   Container        - container to iterate
        @param   InFunction       - for each body 
        @param   InAsyncExecution - execution type
        @retval                   - a task that will be executed when all iterations are finished
    **/
	template<class ContainerType, class FunctionType>
	UE_NODISCARD TTaskPtr<> ForEachNoCopy(
		ContainerType& Container,
		const FunctionType& InFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief   Async for each loop without values copying (no "when all" task spawned)
		@warning No input container elements copying is performed, container must be valid till algorithm completion
		@tparam  ContainerType    - input container type. must support begin() and end()
		@tparam  FunctionType     - input callable type. arg must be void or ContainerType value type
		@param   Container        - container to iterate
		@param   InFunction       - for each body 
		@param   InAsyncExecution - execution type
	**/
	template<class ContainerType, class FunctionType>
	void ForEachNoCopyNoTask(
		ContainerType& Container,
		const FunctionType& InFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
        @brief   Async for each loop
        @tparam  ContainerType    - input container type. must support begin() and end()
        @tparam  FunctionType     - input callable type. arg must be void or ContainerType value type
        @param   Container        - container to iterate
        @param   InFunction       - for each body
        @param   InAsyncExecution - execution type
        @retval                   - a task that will be executed when all iterations are finished
    **/
	template<class ContainerType, class FunctionType>
	UE_NODISCARD TTaskPtr<> ForEach(
		ContainerType& Container,
		const FunctionType& InFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief   Async for each loop (no "when all" task spawned)
		@tparam  ContainerType    - input container type. must support begin() and end()
		@tparam  FunctionType     - input callable type. arg must be void or ContainerType value type
		@param   Container        - container to iterate
		@param   InFunction       - for each body
		@param   InAsyncExecution - execution type
	**/
	template<class ContainerType, class FunctionType>
	void ForEachNoTask(
		ContainerType& Container,
		const FunctionType& InFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);
}

#include "For.inl"
