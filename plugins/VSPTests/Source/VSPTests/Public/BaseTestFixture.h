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

#include "VSPTestsAsserts.h"

namespace VSPTests
{
	/**
        @brief   Base test fixture class. Use it for all your derived test fixtures
        @tparam  FixtureFlagsArg - a combination of EAutomationTestFlags values,
                                   used for specifying Automation Test requirements and behaviors
    **/
	template<int FixtureFlagsArg>
	class FBaseTestFixture
	{
	public:
		using TUpdateFunction = TFunction<bool()>;

		// -------------------------------- to override -------------------------------

		virtual ~FBaseTestFixture() = default;

		/**
            @brief Set up test. Called before test running.
        **/
		virtual void SetUp();

		/**
            @brief Tear down test. Called after test completion (test body or update function)
        **/
		virtual void TearDown();

		/**
            @brief Method is called when all test fixtures are destroyed.
                   May be used to check if all objects were destroyed correctly.
        **/
		static void AfterTearDown();

		// -------------------------------- public api --------------------------------

		/**
            @brief   Set test timeout
            @details If test is being timed out, it fails
            @param   InTimeout - timeout value
        **/
		void SetTimeout(FTimespan InTimeout);

		/**
			@brief  Set update function
			@tparam Callable - callable type (TFunction, lambda, ect)
			@param  callable - callable object
		**/
		template<class Callable>
		void SetUpdate(Callable callable);

		// -------------------------------- private api -------------------------------

		static constexpr int _FixtureFlags = FixtureFlagsArg;

		bool _DoRunTestImpl(FAutomationTestBase* InAutomationTestBase, const FString& Params);

		bool _IsUpdatable() const;

		bool _Update();

	protected:
		// this function is supposed to be called from macro. do not call it manually, use macros instead.
		void TestTrue(const TCHAR* What, bool Condition) const;

		// this function is supposed to be called from macro. do not call it manually, use macros instead.
		template<class T>
		void TestEqual(const TCHAR* What, const T& Left, const T& Right) const;

	private:
		virtual void _DoRunTest(const FString& Params);

	private:
		FAutomationTestBase* AutomationTestBase = nullptr;
		TUpdateFunction UpdateFunction;
		FTimespan Timeout = FTimespan::FromSeconds(5);
		FDateTime TimeoutTimePoint = FDateTime(0);
	};
}

#include "BaseTestFixture.inl"
