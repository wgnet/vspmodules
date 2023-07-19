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
	template<class IndexType, class StepType, class FunctionType>
	TTaskPtr<> For(
		IndexType First,
		IndexType Last,
		StepType Step,
		const FunctionType& InFunction,
		EAsyncExecution InAsyncExecution)
	{
		VSPCheckReturnCF((Last - First) % Step == 0, VSPAsyncLog, TEXT("Infinite loop"), nullptr);

		auto CreateForTask = [InAsyncExecution, &InFunction](IndexType Index)
		{
			return Run(
					   [Index]()
					   {
						   return Index;
					   },
					   InAsyncExecution)
				->Next(InFunction, InAsyncExecution);
		};

		TArray<typename TInvokeResult<decltype(CreateForTask), IndexType>::Type> Tasks;
		for (IndexType Index = First; Index != Last; Index += Step)
			Tasks.Add(CreateForTask(Index));

		return WhenAll(Tasks);
	}

	template<class IndexType, class StepType, class FunctionType>
	void ForNoTask(
		IndexType First,
		IndexType Last,
		StepType Step,
		const FunctionType& InFunction,
		EAsyncExecution InAsyncExecution)
	{
		VSPCheckReturnCF((Last - First) % Step == 0, VSPAsyncLog, TEXT("Infinite loop"));

		for (IndexType Index = First; Index != Last; Index += Step)
		{
			Run(
				[Index]()
				{
					return Index;
				},
				InAsyncExecution)
				->Next(InFunction, InAsyncExecution);
		}
	}

	template<class ContainerType, class FunctionType>
	TTaskPtr<> ForEachNoCopy(ContainerType& Container, const FunctionType& InFunction, EAsyncExecution InAsyncExecution)
	{
		auto CreateForEachTask = [InAsyncExecution, &InFunction](decltype(Container.begin()) It)
		{
			return Run(
				[InFunction, It]()
				{
					InFunction(*It);
				},
				InAsyncExecution);
		};

		TArray<typename TInvokeResult<decltype(CreateForEachTask), decltype(Container.begin())>::Type> Tasks;
		for (auto It = Container.begin(); It != Container.end(); ++It)
			Tasks.Add(CreateForEachTask(It));

		return WhenAll(Tasks);
	}

	template<class ContainerType, class FunctionType>
	void ForEachNoCopyNoTask(ContainerType& Container, const FunctionType& InFunction, EAsyncExecution InAsyncExecution)
	{
		for (auto It = Container.begin(); It != Container.end(); ++It)
		{
			Run(
				[InFunction, It]()
				{
					InFunction(*It);
				},
				InAsyncExecution);
		}
	}

	template<class ContainerType, class FunctionType>
	TTaskPtr<> ForEach(ContainerType& Container, const FunctionType& InFunction, EAsyncExecution InAsyncExecution)
	{
		auto CreateForEachTask = [InAsyncExecution, &InFunction](decltype(Container.begin()) It)
		{
			return Run(
				[InFunction, Value = *It]()
				{
					InFunction(Value);
				},
				InAsyncExecution);
		};

		TArray<typename TInvokeResult<decltype(CreateForEachTask), decltype(Container.begin())>::Type> Tasks;
		for (auto It = Container.begin(); It != Container.end(); ++It)
			Tasks.Add(CreateForEachTask(It));

		return WhenAll(Tasks);
	}

	template<class ContainerType, class FunctionType>
	void ForEachNoTask(ContainerType& Container, const FunctionType& InFunction, EAsyncExecution InAsyncExecution)
	{
		for (auto It = Container.begin(); It != Container.end(); ++It)
		{
			Run(
				[InFunction, Value = *It]()
				{
					InFunction(Value);
				},
				InAsyncExecution);
		}
	}
}
