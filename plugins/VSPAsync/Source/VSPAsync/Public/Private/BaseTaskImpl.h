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

#include "AsyncPointers.h"
#include "Logging/LogMacros.h"

#include "BaseTaskImpl.generated.h"

VSPASYNC_API DECLARE_LOG_CATEGORY_EXTERN(VSPAsyncLog, Log, All);
VSPASYNC_API DECLARE_LOG_CATEGORY_EXTERN(VSPAsyncLogInternal, Log, All);

UENUM()
enum class EAsyncCancellationReason : uint8
{
	Cancelled,
	ParentFailed,
	AwaitTaskCancelled,
	AwaitTaskEmpty,
};

UENUM()
enum class EAsyncTaskState : uint8
{
	NotStarted,
	Cancelled,
	InProgress,
	Successful,
	Failed,
};

UENUM()
enum class EAsyncCancellationResult : uint8
{
	Success,
	FailInProgress,
	FailFinished,
};

namespace VSPAsync
{
	class VSPASYNC_API FBaseTaskImpl : public TSharedFromThis<FBaseTaskImpl, AsyncESPMode>
	{
	public:
		FBaseTaskImpl(FString InName);
		virtual ~FBaseTaskImpl();

		void StartGraph();

		EAsyncTaskState GetState() const;
		bool IsStarted() const;
		bool IsCancelled() const;
		bool IsInProgress() const;
		bool IsFinished() const;
		bool IsSuccessful() const;
		bool IsFailed() const;
		bool IsGraphStarted() const;
		bool IsGraphFinished() const;

		void SetName(FString InName);
		FString GetName() const;

#ifdef VSP_ASYNC_TESTING
		static FString GetTasksAlive();
#endif

	protected:
		virtual void StartBase() = 0;
		virtual void Wait() const = 0;
		virtual EAsyncCancellationResult Cancel(
			const FString& CauserTaskName,
			EAsyncCancellationReason CancellationReason) = 0;
		virtual void RemoveChild(FBaseTaskImpl* ChildToRemove) = 0;
		virtual void Iterate(const TFunction<void(FBaseTaskImpl*)>& Callback) = 0;

		TAsyncSharedPtr<const FBaseTaskImpl> GetBaseParent() const;
		TAsyncSharedPtr<FBaseTaskImpl> GetBaseParent();

		void Iterate(const TFunction<void(const FBaseTaskImpl*)>& Callback) const;

		void SetParent(TAsyncWeakPtr<FBaseTaskImpl> InParent);
		FBaseTaskImpl* GetParent() const;

		void SwitchState(EAsyncTaskState NewState);

		FCriticalSection* GetAsyncVarsMutex() const;
		bool IsFinishedWithChildren() const;

	private:
		// friend template
		template<class RetValType, class ArgType, class FailureType>
		friend class TAsyncTaskImpl;

		// friend template
		template<class ArgType>
		friend class TArgumentDependentTaskImpl;

	private:
		mutable FCriticalSection NameMutex;
		FString Name;

		mutable FCriticalSection AsyncVarsMutex;
		EAsyncTaskState TaskState = EAsyncTaskState::NotStarted;
		TOptional<TAsyncWeakPtr<FBaseTaskImpl>> Parent;

		static FCriticalSection AliveTasksMutex;
		static TArray<const FBaseTaskImpl*> AliveTasks;
	};

	inline FBaseTaskImpl::FBaseTaskImpl(FString InName) : Name(MoveTemp(InName))
	{
#ifdef VSP_ASYNC_TESTING
		FScopeLock ScopeLock(&AliveTasksMutex);
		AliveTasks.Add(this);
#endif
	}

	inline FBaseTaskImpl::~FBaseTaskImpl()
	{
#ifdef VSP_ASYNC_TESTING
		FScopeLock ScopeLock(&AliveTasksMutex);
		AliveTasks.Remove(this);
#endif
	}

#ifdef VSP_ASYNC_TESTING
	inline FString FBaseTaskImpl::GetTasksAlive()
	{
		FScopeLock ScopeLock(&AliveTasksMutex);
		FString TasksAlive;
		for (const auto Task : AliveTasks)
			TasksAlive += Task->GetName() + TEXT("\n");

		return TasksAlive;
	}
#endif
}
