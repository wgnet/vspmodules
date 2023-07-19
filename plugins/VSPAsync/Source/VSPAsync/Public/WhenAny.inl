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
		template<class TaskType>
		auto CreateWhenAnyFunction(TaskType& Task)
		{
			return [Task]()
			{
				if (!Task->IsGraphStarted())
				{
					// If you get an assert here about restarting a task,
					// then you have started the task returned from WhenAny() yourself.
					// This task is meant to be started by the algorithm.
					Task->StartGraph();
				}
			};
		}
	}

	template<class FailureType, class ContainerType>
	TTaskPtr<void, void, FailureType> WhenAny(ContainerType& InputTasks)
	{
		auto Task = CreateEmptyTask<FailureType>();

		auto WhenAnyFunction = Detail::CreateWhenAnyFunction(Task);
		for (auto& InputTask : InputTasks)
			InputTask->NextAtStart(WhenAnyFunction);

		return Task;
	}

	template<class ContainerType>
	TTaskPtr<> WhenAny(ContainerType& InputTasks)
	{
		return WhenAny<DefaultFailureType>(InputTasks);
	}

	template<class FailureType, class HeadTaskType, class... RestTasksType>
	TTaskPtr<void, void, FailureType> WhenAny(HeadTaskType& HeadTask, RestTasksType&&... RestTasks)
	{
		using TaskTypePtr = TTaskPtr<void, void, FailureType>;

		TaskTypePtr Task = CreateEmptyTask<FailureType>();
		Detail::UnrollTasks(
			Task,
			Detail::CreateWhenAnyFunction<TaskTypePtr>,
			HeadTask,
			Forward<RestTasksType>(RestTasks)...);

		return Task;
	}

	template<class HeadTaskType, class... RestTasksType>
	TTaskPtr<> WhenAny(HeadTaskType& HeadTask, RestTasksType&&... RestTasks)
	{
		return WhenAny<DefaultFailureType>(HeadTask, Forward<RestTasksType>(RestTasks)...);
	}
}
