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

#include "Misc/AutomationTest.h"

VSPTESTS_API DECLARE_LOG_CATEGORY_EXTERN(VSPTestsLog, Log, All);

/**
	@brief Logs an error if the specified Condition value is not true
	@param What      - custom description text for the test
    @param Condition - condition to check
**/
#define VSP_EXPECT_TRUE_MSG(What, Condition) \
	do                                      \
	{                                       \
		const bool _bCondition = Condition; \
		TestTrue(What, _bCondition);        \
		if (!_bCondition)                   \
			PLATFORM_BREAK();               \
	} while (false)

/**
    @brief Logs an error if the specified Condition value is not true
    @param Condition - condition to check
**/
#define VSP_EXPECT_TRUE(Condition) VSP_EXPECT_TRUE_MSG(TEXT(#Condition), (Condition));

/**
    @brief Logs an error if the specified values value are not equal
    @param Left  - first value to check
    @param Right - second value to check
**/
#define VSP_EXPECT_EQ(Left, Right) VSP_EXPECT_TRUE_MSG(TEXT(#Left) TEXT(" == ") TEXT(#Right), Left == Right)

/**
    @brief Logs an error unconditionally
    @param What  - error message
**/
#define VSP_EXPECT_NO_ENTRY_MSG(What) VSP_EXPECT_TRUE_MSG(What, false);

/**
    @brief Logs an error unconditionally
**/
#define VSP_EXPECT_NO_ENTRY VSP_EXPECT_TRUE_MSG(TEXT("No entry"), false);
