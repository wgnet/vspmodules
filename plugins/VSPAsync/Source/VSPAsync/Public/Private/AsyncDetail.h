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

namespace VSPAsync
{
	namespace Detail
	{
		template<class NextFunctionConstructionDataType, class NextFunctionCreatorType>
		void UnrollTasks(NextFunctionConstructionDataType, const NextFunctionCreatorType&)
		{
		}

		template<
			class NextFunctionConstructionDataType,
			class NextFunctionCreatorType,
			class HeadTaskType,
			class... RestTasksType>
		void UnrollTasks(
			NextFunctionConstructionDataType NextFunctionConstructionData,
			const NextFunctionCreatorType& NextFunctionCreator,
			HeadTaskType& HeadTask,
			RestTasksType&&... RestTasks)
		{
			HeadTask->NextAtStart(NextFunctionCreator(NextFunctionConstructionData));
			UnrollTasks(NextFunctionConstructionData, NextFunctionCreator, Forward<RestTasksType>(RestTasks)...);
		}
	}
}
