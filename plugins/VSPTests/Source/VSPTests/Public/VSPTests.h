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

#include "BaseTestFixture.h"
#include "VSPTestsDetails.h"

namespace VSPTests
{
	/**
        @brief  If fixture creation is way too complicated, RegisterFixtures() may be used to create it
                These fixtures will be added to all tests using this fixture type
        @see    VSPTestsTest.cpp for examples
        @tparam FixtureType - fixture type 
        @retval             - registered fixture instances
    **/
	template<class FixtureType>
	TArray<FixtureType> RegisterFixtures()
	{
		return {};
	}
}

/**
    @brief   Macro for simple stateless test creation
    @details Test name is VSP.GroupName.TestName
    @see     VSPTestsTest.cpp for examples
    @param   GroupName - test group name
    @param   TestName  - test personal name
    @param   Flags     - a combination of EAutomationTestFlags values,
                         used for specifying Automation Test requirements and behaviors
**/
#define VSP_TEST(GroupName, TestName, Flags)                                                          \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST(F##GroupName##TestName, "VSP." #GroupName "." #TestName, Flags); \
	bool F##GroupName##TestName::RunTest(const FString& Parameters)


/**
    @brief   Macro for stateful test creation
    @details Test name is VSP.GroupName.FixtureClass.TestName.
    @see     VSPTestsTest.cpp for examples
    @param   GroupName    - test group name
    @param   FixtureClass - behaviour and state storage class
    @param   TestName     - test personal name
    @param   ...          - test fixtures (see code examples)
**/
#define VSP_TEST_F(GroupName, FixtureClass, TestName, ...) _VSP_TEST_F(GroupName, FixtureClass, TestName, ##__VA_ARGS__)
