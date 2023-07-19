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
#include "Private/AsyncDetail.h"

namespace VSPAsync
{
	namespace Detail
	{
		struct FWhenAllState
		{
			int32 TotalTasks = 0;
			int32 CurrentTasks = 0;
		};

		template<class TaskType>
		struct FWhenAllFunctionConstructionDataType
		{
			TaskType Task;
			TAsyncSharedPtr<FWhenAllState> WhenAllState;
		};

		template<class ConstructionDataType>
		auto CreateWhenAllFunction(ConstructionDataType InConstructionData)
		{
			return [ConstructionData = MoveTemp(InConstructionData)]()
			{
				++ConstructionData.WhenAllState->CurrentTasks;
				VSPCheckReturnCF(
					ConstructionData.WhenAllState->CurrentTasks <= ConstructionData.WhenAllState->TotalTasks,
					VSPAsyncLogInternal,
					TEXT("Internal error: too much invocations of WhenAllFunction"));

				if (ConstructionData.WhenAllState->CurrentTasks == ConstructionData.WhenAllState->TotalTasks)
				{
					// If you get an assert here about restarting a task,
					// then you have started the task returned from WhenAll() yourself.
					// This task is meant to be started by the algorithm.
					ConstructionData.Task->StartGraph();
				}
			};
		}
	}

	template<class FailureType, class ContainerType>
	TTaskPtr<void, void, FailureType> WhenAll(ContainerType& InputTasks)
	{
		using TaskTypePtr = TTaskPtr<void, void, FailureType>;

		TaskTypePtr Task = CreateEmptyTask<FailureType>();
		auto WhenAllState = AsyncMakeShared<Detail::FWhenAllState>();
		WhenAllState->TotalTasks = InputTasks.Num();

		auto WhenAllFunction = Detail::CreateWhenAllFunction(
			Detail::FWhenAllFunctionConstructionDataType<TaskTypePtr> { Task, WhenAllState });

		for (auto& InputTask : InputTasks)
			InputTask->NextAtStart(WhenAllFunction);

		return Task;
	}

	template<class ContainerType>
	TTaskPtr<> WhenAll(ContainerType& InputTasks)
	{
		return WhenAll<DefaultFailureType>(InputTasks);
	}

	template<class FailureType, class HeadTaskType, class... RestTasksType>
	TTaskPtr<void, void, FailureType> WhenAll(HeadTaskType& HeadTask, RestTasksType&&... RestTasks)
	{
		using TaskTypePtr = TTaskPtr<void, void, FailureType>;
		using ConstructionDataType = Detail::FWhenAllFunctionConstructionDataType<TaskTypePtr>;

		TaskTypePtr Task = CreateEmptyTask<FailureType>();
		auto WhenAllState = AsyncMakeShared<Detail::FWhenAllState>();
		WhenAllState->TotalTasks = sizeof...(RestTasks) + 1;
		ConstructionDataType ConstructionData { Task, MoveTemp(WhenAllState) };

		Detail::UnrollTasks(
			ConstructionData,
			Detail::CreateWhenAllFunction<ConstructionDataType>,
			HeadTask,
			Forward<RestTasksType>(RestTasks)...);

		return Task;
	}

	template<class HeadTaskType, class... RestTasksType>
	TTaskPtr<> WhenAll(HeadTaskType& HeadTask, RestTasksType&&... RestTasks)
	{
		return WhenAll<DefaultFailureType>(HeadTask, Forward<RestTasksType>(RestTasks)...);
	}
}
