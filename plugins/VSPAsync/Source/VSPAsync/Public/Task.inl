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
#include "Private/TaskDetail.inl"

namespace VSPAsync
{
	template<class RetValType, class ArgType, class FailureType>
	TTask<RetValType, ArgType, FailureType>::TTask(TTask&& Other) noexcept
	{
		// wtf why we need it
		*this = MoveTemp(Other);
	}

	template<class RetValType, class ArgType, class FailureType>
	TTask<RetValType, ArgType, FailureType>& TTask<RetValType, ArgType, FailureType>::operator=(TTask&& Other) noexcept
	{
		// wtf why we need it
		Swap(TaskImpl, Other.TaskImpl);
		return *this;
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class FunctionType>
	auto TTask<RetValType, ArgType, FailureType>::Next(FunctionType InFunction, EAsyncExecution InAsyncExecution)
	{
		return Next(MoveTemp(InFunction), nullptr, InAsyncExecution);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class FunctionType, class FailureFunctionType>
	auto TTask<RetValType, ArgType, FailureType>::Next(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		EAsyncExecution InAsyncExecution)
	{
		return Next(MoveTemp(InFunction), MoveTemp(InFailureFunction), nullptr, InAsyncExecution);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class FunctionType, class FailureFunctionType, class CancellationFunctionType>
	auto TTask<RetValType, ArgType, FailureType>::Next(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		CancellationFunctionType InCancellationFunction,
		EAsyncExecution InAsyncExecution)
	{
		return NextImpl(
			MoveTemp(InFunction),
			MoveTemp(InFailureFunction),
			MoveTemp(InCancellationFunction),
			InAsyncExecution,
			false);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class NextTaskType>
	NextTaskType TTask<RetValType, ArgType, FailureType>::NextTask(NextTaskType InNextTask)
	{
		return NextTaskImpl(MoveTemp(InNextTask), false);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class AwaitTaskType>
	auto TTask<RetValType, ArgType, FailureType>::Await(AwaitTaskType AwaitTask)
	{
		return Next(
			[AwaitTaskImpl = AwaitTask->TaskImpl]()
			{
				return MakeAwaitImpl(AwaitTaskImpl, true);
			});
	}

	template<class RetValType, class ArgType, class FailureType>
	void TTask<RetValType, ArgType, FailureType>::StartGraph()
	{
		VSPCheckReturnCF(TaskImpl, VSPAsyncLog, TEXT("Trying to start an empty task"));
		TaskImpl->StartGraph();
	}

	template<class RetValType, class ArgType, class FailureType>
	void TTask<RetValType, ArgType, FailureType>::Wait() const
	{
		VSPCheckReturnCF(TaskImpl, VSPAsyncLog, TEXT("Trying to wait an empty task"));
		TaskImpl->Wait();
	}

	template<class RetValType, class ArgType, class FailureType>
	const typename TTask<RetValType, ArgType, FailureType>::TaskSuccessType*
		TTask<RetValType, ArgType, FailureType>::GetSuccess() const
	{
		VSPCheckReturnCF(TaskImpl, VSPAsyncLog, TEXT("Trying to get result from an empty task"), nullptr);
		return TaskImpl->GetSuccess();
	}

	template<class RetValType, class ArgType, class FailureType>
	const typename TTask<RetValType, ArgType, FailureType>::TaskFailureType*
		TTask<RetValType, ArgType, FailureType>::GetFailure() const
	{
		VSPCheckReturnCF(TaskImpl, VSPAsyncLog, TEXT("Trying to get result from an empty task"), nullptr);
		return TaskImpl->GetFailure();
	}

	template<class RetValType, class ArgType, class FailureType>
	EAsyncCancellationResult TTask<RetValType, ArgType, FailureType>::Cancel()
	{
		if (!TaskImpl)
			return EAsyncCancellationResult::Success;

		return TaskImpl->Cancel(GetName(), EAsyncCancellationReason::Cancelled);
	}

	template<class RetValType, class ArgType, class FailureType>
	FString TTask<RetValType, ArgType, FailureType>::GetName() const
	{
		return TaskImpl ? TaskImpl->GetName() : FString();
	}

	template<class RetValType, class ArgType, class FailureType>
	TTask<RetValType, ArgType, FailureType>* TTask<RetValType, ArgType, FailureType>::SetName(FString InName)
	{
		if (VSPCheckCF(TaskImpl.Get(), VSPAsyncLog, TEXT("Trying to set name to an empty task")))
			TaskImpl->SetName(MoveTemp(InName));

		return this;
	}

	template<class RetValType, class ArgType, class FailureType>
	bool TTask<RetValType, ArgType, FailureType>::IsStarted() const
	{
		return TaskImpl && TaskImpl->IsStarted();
	}

	template<class RetValType, class ArgType, class FailureType>
	bool TTask<RetValType, ArgType, FailureType>::IsCancelled() const
	{
		return TaskImpl && TaskImpl->IsCancelled();
	}

	template<class RetValType, class ArgType, class FailureType>
	bool TTask<RetValType, ArgType, FailureType>::IsInProgress() const
	{
		return TaskImpl && TaskImpl->IsInProgress();
	}

	template<class RetValType, class ArgType, class FailureType>
	bool TTask<RetValType, ArgType, FailureType>::IsFinished() const
	{
		return TaskImpl && TaskImpl->IsFinished();
	}

	template<class RetValType, class ArgType, class FailureType>
	bool TTask<RetValType, ArgType, FailureType>::IsSuccessful() const
	{
		return TaskImpl && TaskImpl->IsSuccessful();
	}

	template<class RetValType, class ArgType, class FailureType>
	bool TTask<RetValType, ArgType, FailureType>::IsFailed() const
	{
		return TaskImpl && TaskImpl->IsFailed();
	}

	template<class RetValType, class ArgType, class FailureType>
	EAsyncTaskState TTask<RetValType, ArgType, FailureType>::GetState() const
	{
		return TaskImpl ? TaskImpl->GetState() : EAsyncTaskState::NotStarted;
	}

	template<class RetValType, class ArgType, class FailureType>
	bool TTask<RetValType, ArgType, FailureType>::IsGraphStarted() const
	{
		return TaskImpl && TaskImpl->IsGraphStarted();
	}

	template<class RetValType, class ArgType, class FailureType>
	bool TTask<RetValType, ArgType, FailureType>::IsGraphCompleted() const
	{
		return TaskImpl && TaskImpl->IsGraphFinished();
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class... _TaskSuccessType>
	typename TTask<RetValType, ArgType, FailureType>::TTaskPtr TTask<RetValType, ArgType, FailureType>::MakeSuccess(
		const _TaskSuccessType&... TaskSuccess)
	{
		auto Task = AsyncMakeShared<TTask>();
		Task->TaskImpl = AsyncMakeShared<ThisTaskImpl>(TEXT("MakeSuccess"), DefaultAsyncExecution);
		Task->TaskImpl->SetResult(ThisTaskImpl::TaskResultType::MakeSuccess(TaskSuccess...));
		return Task;
	}

	template<class RetValType, class ArgType, class FailureType>
	typename TTask<RetValType, ArgType, FailureType>::TTaskPtr TTask<RetValType, ArgType, FailureType>::MakeFailure(
		const TaskFailureType& TaskFailure)
	{
		auto Task = AsyncMakeShared<TTask>();
		Task->TaskImpl = AsyncMakeShared<ThisTaskImpl>(TEXT("MakeFailure"), DefaultAsyncExecution);
		Task->TaskImpl->SetResult(ThisTaskImpl::TaskResultType::MakeFailure(TaskFailure));
		return Task;
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class OtherTaskType>
	typename TTask<RetValType, ArgType, FailureType>::TTaskPtr TTask<RetValType, ArgType, FailureType>::MakeAwait(
		OtherTaskType&& AwaitTask,
		bool bStartIfNotStarted)
	{
		return MakeAwaitImpl(AwaitTask->TaskImpl, bStartIfNotStarted);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class FunctionType>
	auto TTask<RetValType, ArgType, FailureType>::NextAtStart(FunctionType InFunction, EAsyncExecution InAsyncExecution)
	{
		return NextAtStart(MoveTemp(InFunction), nullptr, InAsyncExecution);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class FunctionType, class FailureFunctionType>
	auto TTask<RetValType, ArgType, FailureType>::NextAtStart(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		EAsyncExecution InAsyncExecution)
	{
		return NextAtStart(MoveTemp(InFunction), MoveTemp(InFailureFunction), nullptr, InAsyncExecution);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class FunctionType, class FailureFunctionType, class CancellationFunctionType>
	auto TTask<RetValType, ArgType, FailureType>::NextAtStart(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		CancellationFunctionType InCancellationFunction,
		EAsyncExecution InAsyncExecution)
	{
		return NextImpl(
			MoveTemp(InFunction),
			MoveTemp(InFailureFunction),
			MoveTemp(InCancellationFunction),
			InAsyncExecution,
			true);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class NextTaskType>
	NextTaskType TTask<RetValType, ArgType, FailureType>::NextTaskAtStart(NextTaskType InNextTask)
	{
		return NextTaskImpl(MoveTemp(InNextTask), true);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class Traits, class FunctionType, class FailureFunctionType, class CancellationFunctionType>
	typename TTask<RetValType, ArgType, FailureType>::TTaskPtr TTask<RetValType, ArgType, FailureType>::MakeTask(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		CancellationFunctionType InCancellationFunction,
		EAsyncExecution InAsyncExecution)
	{
		typename ThisTaskImpl::TaskImplFunctionType TaskImplFunction =
			[Function = Detail::TaskFunctionCreator<Traits>::Create(MoveTemp(InFunction))](const auto&... Arg)
		{
			return Function(Arg...)->TaskImpl;
		};

		auto Task = AsyncMakeShared<TTask>();
		Task->TaskImpl = AsyncMakeShared<ThisTaskImpl>(
			typename ThisTaskImpl::TaskImplFunctionType(MoveTemp(TaskImplFunction)),
			TaskFailureFunctionType(MoveTemp(InFailureFunction)),
			TaskCancellationFunctionType(MoveTemp(InCancellationFunction)),
			VSPUtils::GetTypeName<FunctionType>(),
			InAsyncExecution);

		return Task;
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class FunctionType, class FailureFunctionType, class CancellationFunctionType>
	auto TTask<RetValType, ArgType, FailureType>::NextImpl(
		FunctionType InFunction,
		FailureFunctionType InFailureFunction,
		CancellationFunctionType InCancellationFunction,
		EAsyncExecution InAsyncExecution,
		bool bAtStart)
	{
		using Traits = Detail::ContinuationTypeTraits<FunctionType, RetValType, ArgType, FailureType>;
		auto Task = Traits::TaskType::template MakeTask<Traits>(
			MoveTemp(InFunction),
			MoveTemp(InFailureFunction),
			MoveTemp(InCancellationFunction),
			InAsyncExecution);

		return NextTaskImpl(MoveTemp(Task), bAtStart);
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class NextTaskType>
	NextTaskType TTask<RetValType, ArgType, FailureType>::NextTaskImpl(NextTaskType InNextTask, bool bAtStart)
	{
		VSPCheckReturnCF(TaskImpl, VSPAsyncLog, TEXT("Trying to continue an empty task"), MoveTemp(InNextTask));
		TaskImpl->AddChild(InNextTask->TaskImpl, bAtStart);
		return InNextTask;
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class AwaitTaskImplType>
	typename TTask<RetValType, ArgType, FailureType>::TTaskPtr TTask<RetValType, ArgType, FailureType>::MakeAwaitImpl(
		AwaitTaskImplType AwaitTaskImpl,
		bool bStartIfNotStarted)
	{
		VSPCheckReturnCF(AwaitTaskImpl, VSPAsyncLog, TEXT("Trying to make await task from an empty task"), nullptr);

		if (bStartIfNotStarted && !AwaitTaskImpl->IsGraphStarted())
			AwaitTaskImpl->StartGraph();

		auto Task = AsyncMakeShared<TTask>();
		Task->TaskImpl = AsyncMakeShared<ThisTaskImpl>(
			FString(TEXT("Awaiting task of ")) + AwaitTaskImpl->GetName(),
			AwaitTaskImpl->GetAsyncExecution());
		Task->TaskImpl->SetAwaitTask(AwaitTaskImpl);
		return Task;
	}
}
