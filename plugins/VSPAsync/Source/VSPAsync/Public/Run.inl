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

namespace VSPAsync
{
	template<class FailureType, class FunctionType>
	auto Run(FunctionType InFunction, EAsyncExecution InAsyncExecution)
	{
		return Run<FailureType>(MoveTemp(InFunction), nullptr, nullptr, InAsyncExecution);
	}

	template<class FunctionType>
	auto Run(FunctionType InFunction, EAsyncExecution InAsyncExecution)
	{
		return Run<DefaultFailureType>(MoveTemp(InFunction), nullptr, nullptr, InAsyncExecution);
	}

	template<class FailureType, class FunctionType, class FailureFunctionType>
	auto Run(FunctionType InFunction, FailureFunctionType InFailureFunction, EAsyncExecution InAsyncExecution)
	{
		return Run<FailureType>(MoveTemp(InFunction), MoveTemp(InFailureFunction), nullptr, InAsyncExecution);
	}

	template<class FunctionType, class FailureFunctionType>
	auto Run(FunctionType InFunction, FailureFunctionType InFailureFunction, EAsyncExecution InAsyncExecution)
	{
		return Run<DefaultFailureType>(MoveTemp(InFunction), MoveTemp(InFailureFunction), nullptr, InAsyncExecution);
	}

	template<class FailureType, class FunctionType, class FailureFunctionType, class CancellationFunctionType>
	auto Run(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		CancellationFunctionType InCancellationFunction,
		EAsyncExecution InAsyncExecution)
	{
		auto Task = CreateTask<FailureType>(
			MoveTemp(InFunction),
			MoveTemp(InFailureFunction),
			MoveTemp(InCancellationFunction),
			InAsyncExecution);

		Task->StartGraph();
		return Task;
	}

	template<class FunctionType, class FailureFunctionType, class CancellationFunctionType>
	auto Run(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		CancellationFunctionType InCancellationFunction,
		EAsyncExecution InAsyncExecution)
	{
		return Run<DefaultFailureType>(
			MoveTemp(InFunction),
			MoveTemp(InFailureFunction),
			MoveTemp(InCancellationFunction),
			InAsyncExecution);
	}
}
