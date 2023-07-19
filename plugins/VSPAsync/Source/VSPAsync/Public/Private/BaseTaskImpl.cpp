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
#include "BaseTaskImpl.h"
#include "VSPCheck.h"

DEFINE_LOG_CATEGORY(VSPAsyncLog);
DEFINE_LOG_CATEGORY(VSPAsyncLogInternal);

namespace VSPAsync
{
	FCriticalSection FBaseTaskImpl::AliveTasksMutex;
	TArray<const FBaseTaskImpl*> FBaseTaskImpl::AliveTasks;

	void FBaseTaskImpl::StartGraph()
	{
		FScopeLock ScopeLock(GetAsyncVarsMutex());

		const auto BaseParent = GetBaseParent();
		VSPCheckReturnCF(BaseParent.Get(), VSPAsyncLog, TEXT("Base task of this task is already destructed"));
		VSPCheckReturnCF(!IsGraphStarted(), VSPAsyncLog, TEXT("Trying to start already started task graph"));
		BaseParent->StartBase();
	}

	EAsyncTaskState FBaseTaskImpl::GetState() const
	{
		FScopeLock ScopeLock(GetAsyncVarsMutex());
		return TaskState;
	}

	bool FBaseTaskImpl::IsStarted() const
	{
		const EAsyncTaskState State = GetState();
		return State != EAsyncTaskState::NotStarted && State != EAsyncTaskState::Cancelled;
	}

	bool FBaseTaskImpl::IsCancelled() const
	{
		return GetState() == EAsyncTaskState::Cancelled;
	}

	bool FBaseTaskImpl::IsInProgress() const
	{
		return GetState() == EAsyncTaskState::InProgress;
	}

	bool FBaseTaskImpl::IsFinished() const
	{
		const EAsyncTaskState State = GetState();
		return State == EAsyncTaskState::Successful || State == EAsyncTaskState::Failed;
	}

	bool FBaseTaskImpl::IsSuccessful() const
	{
		return GetState() == EAsyncTaskState::Successful;
	}

	bool FBaseTaskImpl::IsFailed() const
	{
		return GetState() == EAsyncTaskState::Failed;
	}

	bool FBaseTaskImpl::IsGraphStarted() const
	{
		FScopeLock ScopeLock(GetAsyncVarsMutex());
		const auto BaseParent = GetBaseParent();
		VSPCheckReturnCF(BaseParent.Get(), VSPAsyncLog, TEXT("Base task of this task is already destructed"), false);
		return BaseParent->IsStarted();
	}

	bool FBaseTaskImpl::IsGraphFinished() const
	{
		FScopeLock ScopeLock(GetAsyncVarsMutex());
		const auto BaseParent = GetBaseParent();
		VSPCheckReturnCF(BaseParent.Get(), VSPAsyncLog, TEXT("Base task of this task is already destructed"), false);
		return BaseParent->IsFinishedWithChildren();
	}

	void FBaseTaskImpl::SetName(FString InName)
	{
		FScopeLock ScopeLock(&NameMutex);
		Name = MoveTemp(InName);
	}

	FString FBaseTaskImpl::GetName() const
	{
		FScopeLock ScopeLock(&NameMutex);
		return Name;
	}

	TAsyncSharedPtr<const FBaseTaskImpl> FBaseTaskImpl::GetBaseParent() const
	{
		if (!Parent.IsSet())
			return AsShared();

		const auto ParentPin = Parent->Pin();
		if (!ParentPin.Get())
			return nullptr;

		return ParentPin->GetBaseParent();
	}

	TAsyncSharedPtr<FBaseTaskImpl> FBaseTaskImpl::GetBaseParent()
	{
		if (!Parent.IsSet())
			return AsShared();

		const auto ParentPin = Parent->Pin();
		if (!ParentPin.Get())
			return nullptr;

		return ParentPin->GetBaseParent();
	}

	void FBaseTaskImpl::Iterate(const TFunction<void(const FBaseTaskImpl*)>& Callback) const
	{
		const_cast<FBaseTaskImpl*>(this)->Iterate(
			[&Callback](FBaseTaskImpl* Child)
			{
				Callback(Child);
			});
	}

	void FBaseTaskImpl::SetParent(TAsyncWeakPtr<FBaseTaskImpl> InParent)
	{
		Parent = InParent;
	}

	FBaseTaskImpl* FBaseTaskImpl::GetParent() const
	{
		return Parent ? Parent->Pin().Get() : nullptr;
	}

	void FBaseTaskImpl::SwitchState(EAsyncTaskState NewState)
	{
		UE_LOG(
			VSPAsyncLog,
			Verbose,
			TEXT("Switched state of task \"%s\": \"%s\" -> \"%s\""),
			*GetName(),
			*UEnum::GetValueAsString(TaskState),
			*UEnum::GetValueAsString(NewState));

		TaskState = NewState;
	}

	FCriticalSection* FBaseTaskImpl::GetAsyncVarsMutex() const
	{
		return &AsyncVarsMutex;
	}

	bool FBaseTaskImpl::IsFinishedWithChildren() const
	{
		if (IsFinished())
		{
			bool bAllFinished = true;
			Iterate(
				[this, &bAllFinished](const FBaseTaskImpl* Child)
				{
					if (Child != this)
						if (!Child->IsFinishedWithChildren())
							bAllFinished = false;
				});

			return bAllFinished;
		}

		return false;
	}
}
