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

#include "BaseTaskImpl.h"
#include "VSPResult.h"
#include "TypeName.h"

#include "Async/Async.h"

#include "TaskImplDetail.inl"

namespace VSPAsync
{
	static constexpr auto DefaultAsyncExecution = EAsyncExecution::TaskGraphMainThread;

	// add specialization for your custom FailureType for default failure callback
	template<class FailureType>
	TFunction<void(const FString& TaskName, const FailureType& FailureReason)> AsyncTaskFailedCallback();

	// tasks that can take same arguments
	template<class ArgType>
	class TArgumentDependentTaskImpl : public FBaseTaskImpl
	{
	public:
		using Super = FBaseTaskImpl;
		using TaskArgType = ArgType;

		using Super::Super;

	protected:
		virtual void StartBase() override;
		virtual void Invoke(const TaskArgType*) = 0;

	private:
		// friend template
		template<class _RetValType, class _ArgType, class _FailureType>
		friend class TAsyncTaskImpl;
	};

	// task async logic
	template<class RetValType, class ArgType, class FailureType>
	class TAsyncTaskImpl : public TArgumentDependentTaskImpl<ArgType>
	{
	public:
		using Super = TArgumentDependentTaskImpl<ArgType>;
		using TaskSuccessType = RetValType;
		using TaskArgType = typename Super::TaskArgType;
		using TaskFailureType = FailureType;
		using TaskResultType = TVSPResult<TaskFailureType, TaskSuccessType>;
		using TaskChildType = TArgumentDependentTaskImpl<TaskSuccessType>;
		using TaskChildTypePtr = TAsyncSharedPtr<TaskChildType>;
		using IsArgTypeVoid = TIsSame<TaskArgType, void>;

	private:
		using SuccessValueExtractor = Detail::SuccessValueExtractor<TIsSame<TaskSuccessType, void>::Value>;
		using ExecutionSelector = Detail::ExecutionSelector<TaskArgType, IsArgTypeVoid::Value>;
		using AwaitTaskImplType = TAsyncTaskImpl<void, TaskSuccessType, TaskFailureType>;

	public:
		using TaskImplFunctionReturnType = TAsyncSharedPtr<TAsyncTaskImpl>;
		using TaskImplFunctionType = typename Detail::FunctionVoidSelector<
			TaskImplFunctionReturnType,
			TaskArgType,
			IsArgTypeVoid::Value>::Function;
		using TaskFailureFunctionType = TFunction<void(const FString& TaskName, const TaskFailureType& FailureReason)>;
		using TaskCancellationFunctionType =
			TFunction<void(const FString& CauserTaskName, EAsyncCancellationReason CancellationReason)>;

	public:
		TAsyncTaskImpl(FString InName, EAsyncExecution InAsyncExecution);
		TAsyncTaskImpl(
			TaskImplFunctionType InFunction,
			TaskFailureFunctionType InFailureFunction,
			TaskCancellationFunctionType InCancellationFunction,
			FString InName,
			EAsyncExecution InAsyncExecution);

		void AddChild(TaskChildTypePtr Child, bool bAtStart);
		void SetResult(TaskResultType InResult);
		template<class OtherTaskImplType>
		void SetAwaitTask(TAsyncSharedPtr<OtherTaskImplType> InAwaitTaskImpl);

		const TaskSuccessType* GetSuccess() const;
		const TaskFailureType* GetFailure() const;
		EAsyncExecution GetAsyncExecution() const;

		virtual void Wait() const override;
		virtual EAsyncCancellationResult Cancel(
			const FString& CauserTaskName,
			EAsyncCancellationReason CancellationReason) override;
		virtual void Invoke(const TaskArgType* Arg) override;
		virtual void Iterate(const TFunction<void(FBaseTaskImpl*)>& Callback) override;

	private:
		virtual void RemoveChild(FBaseTaskImpl* ChildToRemove) override;

		void HandleFunctionReturn(TaskImplFunctionReturnType FunctionReturn);
		void HandleChildrenCancellation(const FString& CauserTaskName, EAsyncCancellationReason CancellationReason);
		void ResetFunctions();

	private:
		// friend template
		template<class _RetValType, class _ArgType, class _FailureType>
		friend class TAsyncTaskImpl;

	private:
		const EAsyncExecution AsyncExecution = DefaultAsyncExecution;

		TAtomic<bool> FutureSet { false };
		TSharedFuture<void> Future;

		TAsyncWeakPtr<FBaseTaskImpl> AwaitTaskChild;
		TAsyncSharedPtr<FBaseTaskImpl> AwaitTaskLock;
		TArray<TaskChildTypePtr> Children;
		TOptional<TaskResultType> Result;
		TaskImplFunctionType Function;
		TaskFailureFunctionType FailureFunction;
		TaskCancellationFunctionType CancellationFunction;
	};

	template<class RetValType, class ArgType, class FailureType>
	using TTaskImpl = TAsyncTaskImpl<RetValType, ArgType, FailureType>;
}

#include "TaskImpl.inl"
