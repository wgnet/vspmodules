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
#include "WeakFunctionTest.h"

#include "VSPTests.h"
#include "WeakFunction.h"

constexpr auto WeakFunctionTestsFlags = EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter;

class FWeakFunctionTests : public VSPTests::FBaseTestFixture<WeakFunctionTestsFlags>
{
protected:
	template<class T>
	void CheckVoid(T* Anchor, bool ShouldExecute);

	template<class T>
	void CheckWithReturnValue(T* Anchor, bool ShouldExecute);
};

template<class T>
void FWeakFunctionTests::CheckVoid(T* Anchor, bool ShouldExecute)
{
	bool Executed = false;
	auto Lambda = [&Executed]()
	{
		Executed = true;
	};

	auto WeakLambda = MakeWeakFunction(Anchor, Lambda, false);
	WeakLambda();

	VSP_EXPECT_EQ(Executed, ShouldExecute);
}

template<class T>
void FWeakFunctionTests::CheckWithReturnValue(T* Anchor, bool ShouldExecute)
{
	bool Executed = false;
	auto Lambda = [&Executed]()
	{
		Executed = true;
		return 1;
	};

	auto WeakLambda = MakeWeakFunction(Anchor, Lambda, 0, false);
	const auto Result = WeakLambda();

	VSP_EXPECT_EQ(Executed, ShouldExecute);
	VSP_EXPECT_TRUE(Executed && Result == 1 || !Executed && Result == 0);
}

VSP_TEST_F(WeakFunctionTest, FWeakFunctionTests, UObject)
{
	CheckVoid(NewObject<UTestObject>(), true);
	CheckVoid(static_cast<UTestObject*>(nullptr), false);
}

VSP_TEST_F(WeakFunctionTest, FWeakFunctionTests, UObjectWithReturn)
{
	CheckWithReturnValue(NewObject<UTestObject>(), true);
	CheckWithReturnValue(static_cast<UTestObject*>(nullptr), false);
}

VSP_TEST_F(WeakFunctionTest, FWeakFunctionTests, SharedFromThis)
{
	CheckVoid(&MakeShared<FSharedFromThisObject>().Get(), true);
	CheckVoid(static_cast<FSharedFromThisObject*>(nullptr), false);
}

VSP_TEST_F(WeakFunctionTest, FWeakFunctionTests, SharedFromThisWithReturn)
{
	CheckWithReturnValue(&MakeShared<FSharedFromThisObject>().Get(), true);
	CheckWithReturnValue(static_cast<FSharedFromThisObject*>(nullptr), false);
}
