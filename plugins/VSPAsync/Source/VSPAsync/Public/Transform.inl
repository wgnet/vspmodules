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
	namespace Detail
	{
		template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
		auto CreateTransformForEachFunction(
			OutContainerType& Output,
			const PredicateType& Predicate,
			const TransformFunctionType& Transform)
		{
			using InputValueType = decltype(*DeclVal<InContainerType>().begin());

			return [&Output, Predicate, Transform, Mutex = AsyncMakeShared<FCriticalSection>()](
					   InputValueType InputElement)
			{
				if (Predicate(InputElement))
				{
					auto OutputElement = Transform(InputElement);
					FScopeLock ScopeLock(Mutex.Get());
					Output.Add(MoveTemp(OutputElement));
				}
			};
		}
	}

	template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
	TTaskPtr<> TransformIfNoCopy(
		const InContainerType& Input,
		OutContainerType& Output,
		const PredicateType& Predicate,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution)
	{
		return ForEachNoCopy(
			Input,
			Detail::CreateTransformForEachFunction<const InContainerType&>(Output, Predicate, Transform),
			InAsyncExecution);
	}

	template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
	void TransformIfNoCopyNoTask(
		const InContainerType& Input,
		OutContainerType& Output,
		const PredicateType& Predicate,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution)
	{
		ForEachNoCopyNoTask(
			Input,
			Detail::CreateTransformForEachFunction<const InContainerType&>(Output, Predicate, Transform),
			InAsyncExecution);
	}

	template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
	TTaskPtr<> TransformIf(
		const InContainerType& Input,
		OutContainerType& Output,
		const PredicateType& Predicate,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution)
	{
		return ForEach(
			Input,
			Detail::CreateTransformForEachFunction<const InContainerType&>(Output, Predicate, Transform),
			InAsyncExecution);
	}

	template<class InContainerType, class OutContainerType, class PredicateType, class TransformFunctionType>
	void TransformIfNoTask(
		const InContainerType& Input,
		OutContainerType& Output,
		const PredicateType& Predicate,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution)
	{
		ForEachNoTask(
			Input,
			Detail::CreateTransformForEachFunction<const InContainerType&>(Output, Predicate, Transform),
			InAsyncExecution);
	}

	template<class InContainerType, class OutContainerType, class TransformFunctionType>
	TTaskPtr<> TransformNoCopy(
		const InContainerType& Input,
		OutContainerType& Output,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution)
	{
		return TransformIfNoCopy(
			Input,
			Output,
			[](const auto&)
			{
				return true;
			},
			Transform,
			InAsyncExecution);
	}

	template<class InContainerType, class OutContainerType, class TransformFunctionType>
	void TransformNoCopyNoTask(
		const InContainerType& Input,
		OutContainerType& Output,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution)
	{
		TransformIfNoCopyNoTask(
			Input,
			Output,
			[](const auto&)
			{
				return true;
			},
			Transform,
			InAsyncExecution);
	}

	template<class InContainerType, class OutContainerType, class TransformFunctionType>
	TTaskPtr<> Transform(
		const InContainerType& Input,
		OutContainerType& Output,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution)
	{
		return TransformIf(
			Input,
			Output,
			[](const auto&)
			{
				return true;
			},
			Transform,
			InAsyncExecution);
	}

	template<class InContainerType, class OutContainerType, class TransformFunctionType>
	void TransformNoTask(
		const InContainerType& Input,
		OutContainerType& Output,
		const TransformFunctionType& Transform,
		EAsyncExecution InAsyncExecution)
	{
		TransformIfNoTask(
			Input,
			Output,
			[](const auto&)
			{
				return true;
			},
			Transform,
			InAsyncExecution);
	}
}
