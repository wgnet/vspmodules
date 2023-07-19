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
	template<class FailureType>
	TFunction<void(const FString& TaskName, const FailureType& FailureReason)> AsyncTaskFailedCallback()
	{
		return [](const FString& TaskName, const FailureType& FailureReason)
		{
			VSPNoEntryCF(VSPAsyncLog, *FString::Printf(TEXT("Async task \"%s\" failed"), *TaskName));
		};
	}

	template<>
	inline TFunction<void(const FString& TaskName, const FString& FailureReason)> AsyncTaskFailedCallback()
	{
		return [](const FString& TaskName, const FString& FailureReason)
		{
			VSPNoEntryCF(VSPAsyncLog, *FString::Printf(TEXT("Async task \"%s\" failed: %s"), *TaskName, *FailureReason));
		};
	}

	template<class ArgType>
	void TArgumentDependentTaskImpl<ArgType>::StartBase()
	{
		FScopeLock ScopeLock(GetAsyncVarsMutex());

		VSPCheckReturnCF(
			this->GetBaseParent().Get() == this,
			VSPAsyncLogInternal,
			TEXT("Internal error: This method can only be used in the base task"));

		UE_LOG(VSPAsyncLog, Verbose, TEXT("Task graph with root \"%s\" has started"), *Super::GetName());
		Invoke(nullptr);
	}

	template<class RetValType, class ArgType, class FailureType>
	TAsyncTaskImpl<RetValType, ArgType, FailureType>::TAsyncTaskImpl(FString InName, EAsyncExecution InAsyncExecution)
		: Super(MoveTemp(InName))
		, AsyncExecution(InAsyncExecution)
	{
	}

	template<class RetValType, class ArgType, class FailureType>
	TAsyncTaskImpl<RetValType, ArgType, FailureType>::TAsyncTaskImpl(
		TaskImplFunctionType InFunction,
		TaskFailureFunctionType InFailureFunction,
		TaskCancellationFunctionType InCancellationFunction,
		FString InName,
		EAsyncExecution InAsyncExecution)
		: Super(MoveTemp(InName))
		, AsyncExecution(InAsyncExecution)
		, Function(MoveTemp(InFunction))
		, FailureFunction(InFailureFunction ? MoveTemp(InFailureFunction) : AsyncTaskFailedCallback<FailureType>())
		, CancellationFunction(MoveTemp(InCancellationFunction))
	{
		VSPCheckCF(static_cast<bool>(Function), VSPAsyncLog, TEXT("Null function was passed in the task"));
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::AddChild(TaskChildTypePtr Child, bool bAtStart)
	{
		VSPCheckReturnCF(Child.Get(), VSPAsyncLog, TEXT("Empty task was passed as continuation"));

		FScopeLock ChildScopeLock(Child->GetAsyncVarsMutex());

		switch (Child->GetState())
		{
		case EAsyncTaskState::Cancelled:
		case EAsyncTaskState::Successful:
		case EAsyncTaskState::Failed:
			VSPNoEntryReturnCF(VSPAsyncLog, TEXT("Adding finished or cancelled task as continuation makes no sence"));
			break;
		}

		FScopeLock ScopeLock(Super::GetAsyncVarsMutex());

		const auto BaseParent = Super::GetBaseParent();
		VSPCheckReturnCF(BaseParent.Get(), VSPAsyncLog, TEXT("Base task of this task is already destructed"));
		VSPCheckReturnCF(
			BaseParent.Get() != Child->GetBaseParent().Get(),
			VSPAsyncLog,
			TEXT("Task was already added to this graph"));

		switch (Super::GetState())
		{
		case EAsyncTaskState::Cancelled:
			VSPNoEntryReturnCF(VSPAsyncLog, TEXT("Trying to add a task to a cancelled task"));
			break;
		}

		Child->SetParent(Super::AsShared());

		if (bAtStart)
			Children.Insert(Child, 0);
		else
			Children.Add(Child);

		switch (Super::GetState())
		{
		case EAsyncTaskState::Successful:
			UE_LOG(
				VSPAsyncLog,
				Verbose,
				TEXT("Added child \"%s\" to successfully finished task \"%s\", started immediately"),
				*Child->GetName(),
				*Super::GetName());

			Child->Invoke(SuccessValueExtractor::Extract(*Result));
			break;

		case EAsyncTaskState::Failed:
			UE_LOG(
				VSPAsyncLog,
				Verbose,
				TEXT("Added child \"%s\" to failed task \"%s\", cancelled immediately"),
				*Child->GetName(),
				*Super::GetName());

			Child->Cancel(Super::GetName(), EAsyncCancellationReason::ParentFailed);
			break;

		default:
			UE_LOG(
				VSPAsyncLog,
				Verbose,
				TEXT("Added child \"%s\" to task \"%s\""),
				*Child->GetName(),
				*Super::GetName());

			break;
		}
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::SetResult(TaskResultType InResult)
	{
		FScopeLock ScopeLock(Super::GetAsyncVarsMutex());
		Result = MoveTemp(InResult);

		if (Result->IsSuccess())
		{
			Super::SwitchState(EAsyncTaskState::Successful);

			// children may be removed during invocation if it fast enough (for ex. ThreadPool)
			auto TempChildren = Children;
			for (TaskChildTypePtr& Child : TempChildren)
				Child->Invoke(SuccessValueExtractor::Extract(*Result));
		}
		else
		{
			Super::SwitchState(EAsyncTaskState::Failed);

			const auto& TaskFailure = Result->GetFailure();
			const FString Name_ = Super::GetName();
			if (FailureFunction)
				FailureFunction(Name_, TaskFailure);

			HandleChildrenCancellation(Name_, EAsyncCancellationReason::ParentFailed);
		}

		ResetFunctions();
	}

	template<class RetValType, class ArgType, class FailureType>
	const typename TAsyncTaskImpl<RetValType, ArgType, FailureType>::TaskSuccessType*
		TAsyncTaskImpl<RetValType, ArgType, FailureType>::GetSuccess() const
	{
		Wait();

		VSPCheckReturnCF(Result.IsSet(), VSPAsyncLogInternal, TEXT("Unexpected dead result after Wait()"), nullptr);
		return Result->IsSuccess() ? &Result->GetResult() : nullptr;
	}

	template<class RetValType, class ArgType, class FailureType>
	const typename TAsyncTaskImpl<RetValType, ArgType, FailureType>::TaskFailureType*
		TAsyncTaskImpl<RetValType, ArgType, FailureType>::GetFailure() const
	{
		Wait();

		VSPCheckReturnCF(Result.IsSet(), VSPAsyncLogInternal, TEXT("Unexpected dead result after Wait()"), nullptr);
		return Result->IsFailure() ? &Result->GetFailure() : nullptr;
	}

	template<class RetValType, class ArgType, class FailureType>
	EAsyncExecution TAsyncTaskImpl<RetValType, ArgType, FailureType>::GetAsyncExecution() const
	{
		return AsyncExecution;
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::Wait() const
	{
		UE_LOG(VSPAsyncLog, Verbose, TEXT("Started waiting on a task \"%s\""), *Super::GetName());

		if (const auto Parent_ = Super::GetParent())
			Parent_->Wait();

		if (FutureSet)
			Future.Wait();

		if (const auto Pin = AwaitTaskChild.Pin())
			Pin->Wait();

		UE_LOG(VSPAsyncLog, Verbose, TEXT("Finished waiting on a task \"%s\""), *Super::GetName());
	}

	template<class RetValType, class ArgType, class FailureType>
	EAsyncCancellationResult TAsyncTaskImpl<RetValType, ArgType, FailureType>::Cancel(
		const FString& CauserTaskName,
		EAsyncCancellationReason CancellationReason)
	{
		FScopeLock ScopeLock(Super::GetAsyncVarsMutex());

		switch (Super::GetState())
		{
		case EAsyncTaskState::InProgress:
			UE_LOG(VSPAsyncLog, Verbose, TEXT("Failed to cancel task \"%s\": already in progress"), *Super::GetName());
			return EAsyncCancellationResult::FailInProgress;

		case EAsyncTaskState::Successful:
		case EAsyncTaskState::Failed:
			UE_LOG(VSPAsyncLog, Verbose, TEXT("Failed to cancel task \"%s\": already finished"), *Super::GetName());
			return EAsyncCancellationResult::FailFinished;
		}

		if (CancellationFunction)
			CancellationFunction(CauserTaskName, CancellationReason);

		Super::SwitchState(EAsyncTaskState::Cancelled);

		UE_LOG(
			VSPAsyncLog,
			Verbose,
			TEXT("Task \"%s\" cancelled with reason \"%s\""),
			*Super::GetName(),
			*UEnum::GetValueAsString(CancellationReason));

		HandleChildrenCancellation(CauserTaskName, CancellationReason);
		// object is dead in this point, be aware of using "this"

		return EAsyncCancellationResult::Success;
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::Invoke(const TaskArgType* Arg)
	{
		VSPCheckReturnCF(
			IsArgTypeVoid::Value || Arg,
			VSPAsyncLogInternal,
			TEXT("Internal error: arg is null, but not void"));

		if (Function)
		{
			TAsyncSharedPtr<FBaseTaskImpl> BaseParentLock = Super::GetBaseParent();
			VSPCheckReturnCF(BaseParentLock.Get(), VSPAsyncLog, TEXT("Base task of this task is already destructed"));
			VSPCheckReturnCF(!FutureSet, VSPAsyncLogInternal, TEXT("Internal error: attempt to invoke a function again"));

			FutureSet = true;
			Future = Async(
				AsyncExecution,
				[this, BaseParentLock_ = MoveTemp(BaseParentLock), Arg]()
				{
					FScopeLock ScopeLock(Super::GetAsyncVarsMutex());
					auto FunctionReturn = ExecutionSelector::Execute(Function, Arg);
					HandleFunctionReturn(MoveTemp(FunctionReturn));
				});

			FScopeLock ScopeLock(Super::GetAsyncVarsMutex());
			Super::SwitchState(EAsyncTaskState::InProgress);
		}
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::Iterate(const TFunction<void(FBaseTaskImpl*)>& Callback)
	{
		FScopeLock ScopeLock(Super::GetAsyncVarsMutex());
		Callback(this);
		for (const TaskChildTypePtr& Child : Children)
			Child->Iterate(Callback);
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::RemoveChild(FBaseTaskImpl* ChildToRemove)
	{
		const int NumRemoved = Children.RemoveAll(
			[ChildToRemove](const TaskChildTypePtr& Child)
			{
				return Child.Get() == ChildToRemove;
			});

		VSPCheckCF(NumRemoved == 1, VSPAsyncLogInternal, TEXT("Internal error: unexpected number of removed children"));
	}

	template<class RetValType, class ArgType, class FailureType>
	template<class OtherTaskImplType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::SetAwaitTask(
		TAsyncSharedPtr<OtherTaskImplType> InAwaitTaskImpl)
	{
		static_assert(
			TIsSame<typename OtherTaskImplType::TaskSuccessType, TaskSuccessType>::Value,
			"Await task must have the same success type as the current task");

		static_assert(
			TIsSame<typename OtherTaskImplType::TaskFailureType, TaskFailureType>::Value,
			"Await task must have the same failure type as the current task");

		VSPCheckReturnCF(InAwaitTaskImpl.Get(), VSPAsyncLog, TEXT("InAwaitTaskImpl is null"));
		FScopeLock AwaitTaskScopeLock(InAwaitTaskImpl->GetAsyncVarsMutex());

		FScopeLock ScopeLock(Super::GetAsyncVarsMutex());

		TAsyncSharedPtr<FBaseTaskImpl> BaseParentLock = Super::GetBaseParent();
		VSPCheckReturnCF(BaseParentLock.Get(), VSPAsyncLog, TEXT("Base task of this task is already destructed"));

		auto AwaitTaskChild_ = AsyncMakeShared<AwaitTaskImplType>(
			[this, InAwaitTaskImpl, BaseParentLock_ = MoveTemp(BaseParentLock)](const auto&... Success)
			{
				auto SuccessResult = TaskResultType::MakeSuccess(Success...);
				SetResult(MoveTemp(SuccessResult));

				InAwaitTaskImpl->AwaitTaskLock.Reset();

				auto SuccessTask = AsyncMakeShared<AwaitTaskImplType>(TEXT("MakeSuccess"), DefaultAsyncExecution);
				SuccessTask->SetResult(AwaitTaskImplType::TaskResultType::MakeSuccess());
				return SuccessTask;
			},
			[](const FString&, const auto&)
			{
			},
			[this, InAwaitTaskImpl](const FString& CauserTaskName, EAsyncCancellationReason CancellationReason)
			{
				FScopeLock AwaitTaskScopeLock(InAwaitTaskImpl->GetAsyncVarsMutex());

				if (CancellationReason == EAsyncCancellationReason::ParentFailed && InAwaitTaskImpl->Result)
				{
					SetResult(*InAwaitTaskImpl->Result);
				}
				else
				{
					FScopeLock ScopeLock(Super::GetAsyncVarsMutex());
					Super::SwitchState(EAsyncTaskState::NotStarted);
					Cancel(CauserTaskName, EAsyncCancellationReason::AwaitTaskCancelled);
				}

				InAwaitTaskImpl->AwaitTaskLock.Reset();
			},
			FString(TEXT("Awaiting service task")),
			AsyncExecution);

		InAwaitTaskImpl->AwaitTaskLock = Super::AsShared();
		AwaitTaskChild = AwaitTaskChild_;
		InAwaitTaskImpl->AddChild(MoveTemp(AwaitTaskChild_), true);

		UE_LOG(
			VSPAsyncLog,
			Verbose,
			TEXT("Task \"%s\" is now awaiting task \"%s\""),
			*Super::GetName(),
			*InAwaitTaskImpl->GetName());
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::HandleFunctionReturn(
		TaskImplFunctionReturnType FunctionReturn)
	{
		if (!FunctionReturn.Get())
		{
			Super::SwitchState(EAsyncTaskState::NotStarted);
			Cancel(TEXT(""), EAsyncCancellationReason::AwaitTaskEmpty);
			VSPNoEntryReturnCF(VSPAsyncLog, TEXT("Empty await task was returned from the function"));
		}

		FScopeLock ScopeLock(FunctionReturn->GetAsyncVarsMutex());

		switch (FunctionReturn->GetState())
		{
		case EAsyncTaskState::NotStarted:
		case EAsyncTaskState::InProgress:
			UE_LOG(
				VSPAsyncLog,
				Verbose,
				TEXT("Task \"%s\" is finished with not finished task \"%s\""),
				*Super::GetName(),
				*FunctionReturn->GetName());

			SetAwaitTask(FunctionReturn);
			break;

		case EAsyncTaskState::Cancelled:
			Super::SwitchState(EAsyncTaskState::NotStarted);
			Cancel(FunctionReturn->GetName(), EAsyncCancellationReason::AwaitTaskCancelled);
			VSPNoEntryCF(VSPAsyncLog, TEXT("Trying to use cancelled task as async continuation"));
			break;

		case EAsyncTaskState::Successful:
		case EAsyncTaskState::Failed:
			UE_LOG(VSPAsyncLog, Verbose, TEXT("Task \"%s\" is finished with result"), *Super::GetName());
			SetResult(*FunctionReturn->Result);
			break;
		}
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::HandleChildrenCancellation(
		const FString& CauserTaskName,
		EAsyncCancellationReason CancellationReason)
	{
		while (Children.Num() != 0)
			Children[Children.Num() - 1]->Cancel(CauserTaskName, CancellationReason);

		if (const auto Parent_ = Super::GetParent())
			Parent_->RemoveChild(this);
	}

	template<class RetValType, class ArgType, class FailureType>
	void TAsyncTaskImpl<RetValType, ArgType, FailureType>::ResetFunctions()
	{
		Function = TaskImplFunctionType();
		FailureFunction = TaskFailureFunctionType();
		CancellationFunction = TaskCancellationFunctionType();
	}
}
