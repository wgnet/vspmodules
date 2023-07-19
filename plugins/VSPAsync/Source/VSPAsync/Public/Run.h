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

#include "CreateTask.h"

namespace VSPAsync
{
	/**
        @brief  Create a task with function and custom FailureType and run it
        @tparam FailureType      - custom type to use as error marker in FailureFunctionType callable
        @tparam FunctionType     - input callable type. arg must be void or prev chain element success result
        @param  InFunction       - callable for the task
        @param  InAsyncExecution - execution type
        @retval                  - created TTaskPtr<>
    **/
	template<class FailureType, class FunctionType>
	auto Run(FunctionType InFunction, EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
        @brief  Create a task with function and run it
        @tparam FunctionType     - input callable type. arg must be void or prev chain element success result
        @param  InFunction       - callable for the task
        @param  InAsyncExecution - execution type
        @retval                  - created TTaskPtr<>
    **/
	template<class FunctionType>
	auto Run(FunctionType InFunction, EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
        @brief  Create a task with function and failure callback function and custom FailureType and run it
        @tparam FailureType         - custom type to use as error marker in FailureFunctionType callable
        @tparam FunctionType        - input callable type. arg must be void or prev chain element success result
        @tparam FailureFunctionType - failure callable type 
        @param  InFunction          - callable for the task
        @param  InFailureFunction   - callable for the task failure callback
        @param  InAsyncExecution    - execution type
        @retval                     - created TTaskPtr<>
    **/
	template<class FailureType, class FunctionType, class FailureFunctionType>
	auto Run(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/** 
        @brief  Create a task with function and failure callback function and run it
        @tparam FunctionType        - input callable type. arg must be void or prev chain element success result
        @tparam FailureFunctionType - failure callable type 
        @param  InFunction          - callable for the task
        @param  InFailureFunction   - callable for the task failure callback
        @param  InAsyncExecution    - execution type
        @retval                     - created TTaskPtr<>
    **/
	template<class FunctionType, class FailureFunctionType>
	auto Run(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
        @brief  Create a task with function, failure callback, cancellation callback and custom FailureType and run it
        @tparam FailureType              - custom type to use as error marker in FailureFunctionType callable
        @tparam FunctionType             - input callable type. arg must be void or prev chain element success result
        @tparam FailureFunctionType      - failure callable type 
        @tparam CancellationFunctionType - cancellation callable type 
        @param  InFunction               - callable for the task
        @param  InFailureFunction        - callable for the task failure callback
        @param  InCancellationFunction   - callable for the task cancellation callback
        @param  InAsyncExecution         - execution type
        @retval                          - created TTaskPtr<>
    **/
	template<class FailureType, class FunctionType, class FailureFunctionType, class CancellationFunctionType>
	auto Run(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		CancellationFunctionType InCancellationFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

	/**
        @brief  Create a task with function, failure callback and cancellation callback and run it
        @tparam FunctionType             - input callable type. arg must be void or prev chain element success result
        @tparam FailureFunctionType      - failure callable type 
        @tparam CancellationFunctionType - cancellation callable type 
        @param  InFunction               - callable for the task
        @param  InFailureFunction        - callable for the task failure callback
        @param  InCancellationFunction   - callable for the task cancellation callback
        @param  InAsyncExecution         - execution type
        @retval                          - created TTaskPtr<>
    **/
	template<class FunctionType, class FailureFunctionType, class CancellationFunctionType>
	auto Run(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		CancellationFunctionType InCancellationFunction,
		EAsyncExecution InAsyncExecution = DefaultAsyncExecution);
}

#include "Run.inl"
