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

#include "Private/TaskImpl.h"
#include "TaskFwd.h"

namespace VSPAsync
{
	/**
        @brief   A task is an operation that can be performed asynchronously
        @details Typical pipeline is:
                 Use VSPAsync::CreateTask() to create base task
                 Use Next() and NextTask() to add continuation tasks
                 Use StartGraph() to start base task and all it's nested tasks
                 Wait() with a method or do some other useful work
                 GetSuccess() or GetFailure() when task IsFinished() 
        @tparam  RetValType  - task return type
        @tparam  ArgType     - task argument type
        @tparam  FailureType - task failure type used in failure callback
    **/
	template<class RetValType, class ArgType, class FailureType>
	class TTask
	{
		using ThisTaskImpl = TTaskImpl<RetValType, ArgType, FailureType>;

	public:
		using TTaskPtr = TTaskPtr<RetValType, ArgType, FailureType>;
		using TaskSuccessType = typename ThisTaskImpl::TaskSuccessType;
		using TaskArgType = typename ThisTaskImpl::TaskArgType;
		using TaskFailureType = typename ThisTaskImpl::TaskFailureType;
		using TaskFunctionType =
			typename Detail::FunctionVoidSelector<TTaskPtr, ArgType, ThisTaskImpl::IsArgTypeVoid::Value>::Function;
		using TaskFailureFunctionType = typename ThisTaskImpl::TaskFailureFunctionType;
		using TaskCancellationFunctionType = typename ThisTaskImpl::TaskCancellationFunctionType;

	public:
		TTask() = default;
		TTask(const TTask&) = delete;
		TTask(TTask&& Other) noexcept;
		TTask& operator=(const TTask&) = delete;
		TTask& operator=(TTask&& Other) noexcept;

		/**
            @brief  Create task and add it as continuation to the current one
            @note   This method is thread safe
            @tparam FunctionType     - input callable type. arg must be void or prev chain element success result
            @param  InFunction       - callable for the task
            @param  InAsyncExecution - execution type
            @retval                  - added TTaskPtr<>
        **/
		template<class FunctionType>
		auto Next(FunctionType InFunction, EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

		/**
            @brief  Create task and add it as continuation to the current one
            @note   This method is thread safe
            @tparam FunctionType        - input callable type. arg must be void or prev chain element success result
            @tparam FailureFunctionType - failure callable type 
            @param  InFunction          - callable for the task
            @param  InFailureFunction   - callable for the task failure callback
            @param  InAsyncExecution    - execution type
            @retval                     - added TTaskPtr<>
        **/
		template<class FunctionType, class FailureFunctionType>
		auto Next(
			FunctionType InFunction,
			FailureFunctionType InFailureFunction,
			EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

		/**
            @brief  Create task and add it as continuation to the current one
            @note   This method is thread safe
            @tparam FunctionType             - input callable type. arg must be void or prev chain element success result
            @tparam FailureFunctionType      - failure callable type 
            @tparam CancellationFunctionType - cancellation callable type 
            @param  InFunction               - callable for the task
            @param  InFailureFunction        - callable for the task failure callback
            @param  InCancellationFunction   - callable for the task cancellation callback
            @param  InAsyncExecution         - execution type
            @retval                          - added TTaskPtr<>
        **/
		template<class FunctionType, class FailureFunctionType, class CancellationFunctionType>
		auto Next(
			FunctionType InFunction,
			FailureFunctionType InFailureFunction,
			CancellationFunctionType InCancellationFunction,
			EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

		/**
            @brief  Add task as continuation to the current one
            @note   This method is thread safe
            @tparam NextTaskType - TTaskPtr<> specialization type
            @param  InNextTask   - task object
            @retval              - same InNextTask, but added
        **/
		template<class NextTaskType>
		NextTaskType NextTask(NextTaskType InNextTask);

		/**
            @brief   Await for a task and use its result
            @note    AwaitTask will be started if it hasn't started yet
                     This method is thread safe
            @tparam  AwaitTaskType - other task type with same TaskSuccessType and TaskFailureType
            @param   AwaitTask     - other task object
            @retval                - created TTaskPtr<> that will be executed when AwaitTask is completed

            @code
            FirstTask.Await(GetSecondTask());
            @endcode
            
            is equivalent of
            
            @code
            FirstTask.Next(
                [this]()
                {
                    return TTask<>::MakeAwait(GetSecondTask(), true);
                });
            @endcode
        **/
		template<class AwaitTaskType>
		auto Await(AwaitTaskType AwaitTask);

		/**
            @brief Start task graph
            @note  This will start base task and the task of this object will be executed when it is his turn
                   This method is thread safe
        **/
		void StartGraph();

		/**
            @brief   Wait until result is ready
            @warning Blocks this thread
                     This method is NOT thread safe
        **/
		void Wait() const;

		/**
            @brief   Get success result of this task or nullptr if task was failed or cancelled
            @warning Will block this thread until result is ready if it's not
                     This method is NOT thread safe
            @retval  - task success result
        **/
		const TaskSuccessType* GetSuccess() const;

		/**
            @brief   Get failure result of this task or nullptr if task was success or cancelled
            @warning Will block this thread until result is ready if it's not
                     This method is NOT thread safe
            @retval  - task failure result
        **/
		const TaskFailureType* GetFailure() const;

		/** 
            @brief  Cancel this task and all nested tasks scheduling
            @note   Will only work if this task hasn't started yet
                    This method is thread safe
            @retval  - cancellation result
        **/
		EAsyncCancellationResult Cancel();

		/** 
            @brief  Get task name
            @note   Default name is name of FunctionType which typically is lambda and looks like
                        clang:
                            (lambda at /app/example.cpp:103:28)
                        MSVC:
                            class FMyClass::Foo::<lambda>
                    This method is thread safe
            @retval - task name
        **/
		FString GetName() const;

		/** 
            @brief  Set task name
            @note   This method is thread safe
            @param  InName - new task name
            @retval        - this object ref to use in chain
        **/
		TTask* SetName(FString InName);

		/**
            @brief  Check if task started execution
            @note   This method is thread safe
            @retval  - true if started (InProgress || Successful || Failed)
        **/
		bool IsStarted() const;

		/**
            @brief  Check if task is cancelled
            @note   This method is thread safe
            @retval  - true if cancelled
        **/
		bool IsCancelled() const;

		/**
            @brief  Check if task execution is in progress, but not completed yet
            @note   This method is thread safe
            @retval  - true if task is in progress
        **/
		bool IsInProgress() const;

		/**
            @brief  Check if task execution has finished and result is ready
            @note   This method is thread safe
            @retval  - true if task is finished (Successful || Failed)
        **/
		bool IsFinished() const;

		/**
            @brief  Check if task is successfully completed
            @note   This method is thread safe
            @retval  - true if task is successfully completed
        **/
		bool IsSuccessful() const;

		/**
            @brief  Check if task is failed
            @note   This method is thread safe
            @retval  - true if failed
        **/
		bool IsFailed() const;

		/**
            @brief  Get task state
            @note   This method is thread safe
            @retval  - task state
        **/
		EAsyncTaskState GetState() const;

		/**
            @brief  Check if base task execution has started
            @note   This method is thread safe
            @retval  - true if base task execution has started
        **/
		bool IsGraphStarted() const;

		/**
            @brief  Check if base task and all nested tasks execution has finished or cancelled
            @note   This method is thread safe
            @retval  - true if base task and all nested tasks execution has finished or cancelled
        **/
		bool IsGraphCompleted() const;

		/**
            @brief  Make task and complete it successfully with TaskSuccessType value
            @tparam _TaskSuccessType - TaskSuccessType that can be void
            @param  TaskSuccess      - void or TaskSuccessType type object
            @retval                  - created task
        **/
		template<class... _TaskSuccessType>
		static TTaskPtr MakeSuccess(const _TaskSuccessType&... TaskSuccess);

		/**
            @brief  Make task and fail it with TaskFailureType value
            @param  TaskFailure - failure object
            @retval             - created task
        **/
		static TTaskPtr MakeFailure(const TaskFailureType& TaskFailure);

		/**
            @brief   Make await task of this type
            @details This is conversion method for tasks with same TaskSuccessType and TaskFailureType
            @tparam  AwaitTaskType      - other task type with same TaskSuccessType and TaskFailureType
            @param   AwaitTask          - other task object
            @param   bStartIfNotStarted - if true, AwaitTask task will be started if it hasn't started yet
            @retval                     - created task
        **/
		template<class AwaitTaskType>
		static TTaskPtr MakeAwait(AwaitTaskType&& AwaitTask, bool bStartIfNotStarted = false);

	private:
		// friend template
		template<class _RetValType, class _ArgType, class _FailureType>
		friend class TTask;

		// MakeTask
		template<class _FailureType, class _FunctionType, class _FailureFunctionType, class _CancellationFunctionType>
		friend auto CreateTask(_FunctionType, _FailureFunctionType, _CancellationFunctionType, EAsyncExecution);

		// NextAtStart
		template<class _FailureType, class _ContainerType>
		friend VSPAsync::TTaskPtr<void, void, _FailureType> WhenAny(_ContainerType& InputTasks);

		// NextAtStart
		template<class _FailureType, class _ContainerType>
		friend VSPAsync::TTaskPtr<void, void, _FailureType> WhenAll(_ContainerType& InputTasks);

		// NextAtStart
		template<
			class _NextFunctionConstructionDataType,
			class _NextFunctionCreatorType,
			class _HeadTaskType,
			class... _RestTasksType>
		friend void Detail::UnrollTasks(
			_NextFunctionConstructionDataType NextFunctionConstructionData,
			const _NextFunctionCreatorType& NextFunctionCreator,
			_HeadTaskType& HeadTask,
			_RestTasksType&&... RestTasks);

	private:
		// NextAtStart methods are not currently in the public API but they can be if we really want it
		// they can be also replaced with methods with "priority" param
		// {
		template<class FunctionType>
		auto NextAtStart(FunctionType InFunction, EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

		template<class FunctionType, class FailureFunctionType>
		auto NextAtStart(
			FunctionType InFunction,
			FailureFunctionType InFailureFunction,
			EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

		template<class FunctionType, class FailureFunctionType, class CancellationFunctionType>
		auto NextAtStart(
			FunctionType InFunction,
			FailureFunctionType InFailureFunction,
			CancellationFunctionType InCancellationFunction,
			EAsyncExecution InAsyncExecution = DefaultAsyncExecution);

		template<class NextTaskType>
		NextTaskType NextTaskAtStart(NextTaskType InNextTask);
		// }

		template<class Traits, class FunctionType, class FailureFunctionType, class CancellationFunctionType>
		static TTaskPtr MakeTask(
			FunctionType InFunction,
			FailureFunctionType InFailureFunction,
			CancellationFunctionType InCancellationFunction,
			EAsyncExecution InAsyncExecution);

		template<class FunctionType, class FailureFunctionType, class CancellationFunctionType>
		auto NextImpl(
			FunctionType InFunction,
			FailureFunctionType InFailureFunction,
			CancellationFunctionType InCancellationFunction,
			EAsyncExecution InAsyncExecution,
			bool bAtStart);

		template<class NextTaskType>
		NextTaskType NextTaskImpl(NextTaskType InNextTask, bool bAtStart);

		template<class AwaitTaskImplType>
		static TTaskPtr MakeAwaitImpl(AwaitTaskImplType AwaitTaskImpl, bool bStartIfNotStarted);

	private:
		TAsyncSharedPtr<ThisTaskImpl> TaskImpl;
	};
}

#include "Task.inl"
