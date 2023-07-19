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

#include "For.h"

namespace VSPAsync
{
	/**
		@brief   Conditionally and asynchronously applies a transform to a range and stores the results into a container
		@warning No input container elements copying is performed, container must be valid till algorithm completion
		@note    Adding new elements to the Output will be guarded by mutex
		@tparam  InContainerType       - input container type. must support begin() and end()
		@tparam  OutContainerType      - output container type. must support Add()
		@tparam  PredicateType         - callable that takes value type of input container and returns bool
		@tparam  TransformFunctionType - callable that takes value type of input container and returns value type of output container
		@param   Input                 - container to transform
		@param   Output                - container to hold the output
		@param   Predicate             - condition which returns true for elements that should be transformed
		@param   Transform             - transformation function
		@param   InAsyncExecution      - execution type
        @retval                        - a task that will be executed when all iterations are finished
	**/
	template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
	UE_NODISCARD TTaskPtr<> TransformIfNoCopy(
		const InContainerType& Input,
		OutContainerType& Output,
		const PredicateType& Predicate,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief   Conditionally and asynchronously applies a transform to a range and stores the results into a container (no "when all" task spawned)
		@warning No input container elements copying is performed, container must be valid till algorithm completion
		@note    Adding new elements to the Output will be guarded by mutex
		@tparam  InContainerType       - input container type. must support begin() and end()
		@tparam  OutContainerType      - output container type. must support Add()
		@tparam  PredicateType         - callable that takes value type of input container and returns bool
		@tparam  TransformFunctionType - callable that takes value type of input container and returns value type of output container
		@param   Input                 - container to transform
		@param   Output                - container to hold the output
		@param   Predicate             - condition which returns true for elements that should be transformed
		@param   Transform             - transformation function
		@param   InAsyncExecution      - execution type
	**/
	template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
	void TransformIfNoCopyNoTask(
		const InContainerType& Input,
		OutContainerType& Output,
		const PredicateType& Predicate,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief  Conditionally and asynchronously applies a transform to a range and stores the results into a container
		@note   Adding new elements to the Output will be guarded by mutex
		@tparam InContainerType       - input container type. must support begin() and end()
		@tparam OutContainerType      - output container type. must support Add()
		@tparam PredicateType         - callable that takes value type of input container and returns bool
		@tparam TransformFunctionType - callable that takes value type of input container and returns value type of output container
		@param  Input                 - container to transform
		@param  Output                - container to hold the output
		@param  Predicate             - condition which returns true for elements that should be transformed
		@param  Transform             - transformation function
		@param  InAsyncExecution      - execution type
        @retval                       - a task that will be executed when all iterations are finished
	**/
	template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
	UE_NODISCARD TTaskPtr<> TransformIf(
		const InContainerType& Input,
		OutContainerType& Output,
		const PredicateType& Predicate,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief  Conditionally and asynchronously applies a transform to a range and stores the results into a container (no "when all" task spawned)
		@note   Adding new elements to the Output will be guarded by mutex
		@tparam InContainerType       - input container type. must support begin() and end()
		@tparam OutContainerType      - output container type. must support Add()
		@tparam PredicateType         - callable that takes value type of input container and returns bool
		@tparam TransformFunctionType - callable that takes value type of input container and returns value type of output container
		@param  Input                 - container to transform
		@param  Output                - container to hold the output
		@param  Predicate             - condition which returns true for elements that should be transformed
		@param  Transform             - transformation function
		@param  InAsyncExecution      - execution type
	**/
	template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
	void TransformIfNoTask(
		const InContainerType& Input,
		OutContainerType& Output,
		const PredicateType& Predicate,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief   Asynchronously applies a transform to a range and stores the results into a container
		@note    Adding new elements to the Output will be guarded by mutex
		@warning No input container elements copying is performed, container must be valid till algorithm completion
		@tparam  InContainerType       - input container type. must support begin() and end()
		@tparam  OutContainerType      - output container type. must support Add()
		@tparam  TransformFunctionType - callable that takes value type of input container and returns value type of output container
		@param   Input                 - container to transform
		@param   Output                - container to hold the output
		@param   Transform             - transformation function
		@param   InAsyncExecution      - execution type
        @retval                        - a task that will be executed when all iterations are finished
	**/
	template<class InContainerType, class OutContainerType, class TransformFunctionType>
	UE_NODISCARD TTaskPtr<> TransformNoCopy(
		const InContainerType& Input,
		OutContainerType& Output,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief   Asynchronously applies a transform to a range and stores the results into a container (no "when all" task spawned)
		@note    Adding new elements to the Output will be guarded by mutex
		@warning No input container elements copying is performed, container must be valid till algorithm completion
		@tparam  InContainerType       - input container type. must support begin() and end()
		@tparam  OutContainerType      - output container type. must support Add()
		@tparam  TransformFunctionType - callable that takes value type of input container and returns value type of output container
		@param   Input                 - container to transform
		@param   Output                - container to hold the output
		@param   Transform             - transformation function
		@param   InAsyncExecution      - execution type
	**/
	template<class InContainerType, class OutContainerType, class TransformFunctionType>
	void TransformNoCopyNoTask(
		const InContainerType& Input,
		OutContainerType& Output,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief  Asynchronously applies a transform to a range and stores the results into a container
		@note   Adding new elements to the Output will be guarded by mutex
		@tparam InContainerType       - input container type. must support begin() and end()
		@tparam OutContainerType      - output container type. must support Add()
		@tparam TransformFunctionType - callable that takes value type of input container and returns value type of output container
		@param  Input                 - container to transform
		@param  Output                - container to hold the output
		@param  Transform             - transformation function
		@param  InAsyncExecution	  - execution type
        @retval                       - a task that will be executed when all iterations are finished
	**/
	template<class InContainerType, class OutContainerType, class TransformFunctionType>
	UE_NODISCARD TTaskPtr<> Transform(
		const InContainerType& Input,
		OutContainerType& Output,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
		@brief  Asynchronously applies a transform to a range and stores the results into a container (no "when all" task spawned)
		@note   Adding new elements to the Output will be guarded by mutex
		@tparam InContainerType       - input container type. must support begin() and end()
		@tparam OutContainerType      - output container type. must support Add()
		@tparam TransformFunctionType - callable that takes value type of input container and returns value type of output container
		@param  Input                 - container to transform
		@param  Output                - container to hold the output
		@param  Transform             - transformation function
		@param  InAsyncExecution	  - execution type
	**/
	template<class InContainerType, class OutContainerType, class TransformFunctionType>
	void TransformNoTask(
		const InContainerType& Input,
		OutContainerType& Output,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);
}

#include "Transform.inl"
