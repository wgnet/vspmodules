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

#include "CreateTask.h"

namespace VSPAsync
{
	/**
        @brief   Create a task that will be executed when all of the input tasks is finished
        @warning You must not start this task yourself
        @tparam  FailureType   - custom type to use as error marker in FailureFunctionType callable
        @tparam  ContainerType - "Container" type. must support begin() and end()
        @param   InputTasks    - container with tasks
        @retval                - a task that will be executed when all of the input tasks are finished
    **/
	template<class FailureType, class ContainerType>
	UE_NODISCARD TTaskPtr<void, void, FailureType> WhenAll(ContainerType& InputTasks);

	/**
	    @brief   Create a task that will be executed when all of the input tasks is finished
        @warning You must not start this task yourself
	    @tparam  ContainerType - "Container" type. must support begin() and end()
	    @param   InputTasks    - container with tasks
	    @retval                - a task that will be executed when all of the input tasks are finished
	**/
	template<class ContainerType>
	UE_NODISCARD TTaskPtr<> WhenAll(ContainerType& InputTasks);

	/**
        @brief   Create a task that will be executed when all of the input tasks is finished
        @warning You must not start this task yourself
        @tparam  FailureType   - custom type to use as error marker in FailureFunctionType callable
        @tparam  HeadTaskType  - first task type. all tasks may be of different types
        @tparam  RestTasksType - rest tasks type. all tasks may be of different types
        @param   HeadTask      - first task object
        @param   RestTasks     - rest tasks objects pack
        @retval                - a task that will be executed when all of the input tasks are finished
    **/
	template<class FailureType, class HeadTaskType, class... RestTasksType>
	UE_NODISCARD TTaskPtr<void, void, FailureType> WhenAll(HeadTaskType& HeadTask, RestTasksType&&... RestTasks);

	/**
        @brief   Create a task that will be executed when all of the input tasks is finished
        @warning You must not start this task yourself
        @tparam  HeadTaskType  - first task type. all tasks may be of different types
        @tparam  RestTasksType - rest tasks type. all tasks may be of different types
        @param   HeadTask      - first task object
        @param   RestTasks     - rest tasks objects pack
        @retval                - a task that will be executed when all of the input tasks are finished
    **/
	template<class HeadTaskType, class... RestTasksType>
	UE_NODISCARD TTaskPtr<> WhenAll(HeadTaskType& HeadTask, RestTasksType&&... RestTasks);
}

#include "WhenAll.inl"
