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

/*
#include "VSPTests.h"

static constexpr int AsyncTestsFlags =
	EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter;

class FBaseAsyncTaskTest : public VSPTests::FBaseTestFixture<AsyncTestsFlags>
{
public:
	static void AfterTearDown()
	{
		const FString TasksAlive = VSPAsync::FBaseTaskImpl::GetTasksAlive();
		VSPCheckCF(
			TasksAlive.Len() == 0,
			VSPAsyncLog,
			*FString::Printf(TEXT("Unexpected alive tasks found:\n%s"), *TasksAlive));
	}
};

struct FNotInt
{
	int32 Int = 0;
};
*/