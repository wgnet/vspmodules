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

namespace VSPTests
{
	template<int FixtureFlagsArg>
	void FBaseTestFixture<FixtureFlagsArg>::SetUp()
	{
	}

	template<int FixtureFlagsArg>
	void FBaseTestFixture<FixtureFlagsArg>::TearDown()
	{
	}

	template<int FixtureFlagsArg>
	void FBaseTestFixture<FixtureFlagsArg>::AfterTearDown()
	{
	}

	template<int FixtureFlagsArg>
	void FBaseTestFixture<FixtureFlagsArg>::SetTimeout(FTimespan InTimeout)
	{
		Timeout = InTimeout;
	}

	template<int FixtureFlagsArg>
	template<class Callable>
	void FBaseTestFixture<FixtureFlagsArg>::SetUpdate(Callable callable)
	{
		UpdateFunction = MoveTemp(callable);
	}

	template<int FixtureFlagsArg>
	bool FBaseTestFixture<FixtureFlagsArg>::_DoRunTestImpl(
		FAutomationTestBase* InAutomationTestBase,
		const FString& Params)
	{
		AutomationTestBase = InAutomationTestBase;

		TimeoutTimePoint = FDateTime::Now() + Timeout;
		SetUp();
		_DoRunTest(Params);
		if (!UpdateFunction)
			TearDown();

		return true;
	}

	template<int FixtureFlagsArg>
	bool FBaseTestFixture<FixtureFlagsArg>::_IsUpdatable() const
	{
		return static_cast<bool>(UpdateFunction);
	}

	template<int FixtureFlagsArg>
	bool FBaseTestFixture<FixtureFlagsArg>::_Update()
	{
		if (!_IsUpdatable())
		{
			VSP_EXPECT_NO_ENTRY;
			return true;
		}

		if (TimeoutTimePoint < FDateTime::Now())
		{
			VSP_EXPECT_NO_ENTRY_MSG(TEXT("Test fixture timed out"));
			return true;
		}

		const bool bShouldEndUpdating = UpdateFunction();
		if (bShouldEndUpdating)
			TearDown();

		return bShouldEndUpdating;
	}

	template<int FixtureFlagsArg>
	void FBaseTestFixture<FixtureFlagsArg>::TestTrue(const TCHAR* What, bool Condition) const
	{
		AutomationTestBase->TestTrue(What, Condition);
	}

	template<int FixtureFlagsArg>
	template<class T>
	void FBaseTestFixture<FixtureFlagsArg>::TestEqual(const TCHAR* What, const T& Left, const T& Right) const
	{
		AutomationTestBase->TestEqual(What, Left, Right);
	}

	template<int FixtureFlagsArg>
	void FBaseTestFixture<FixtureFlagsArg>::_DoRunTest(const FString& Params)
	{
		VSP_EXPECT_NO_ENTRY;
	}
}
