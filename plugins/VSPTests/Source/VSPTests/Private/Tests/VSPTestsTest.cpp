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
#include "VSPTests.h"

#include <ctime>

class FExampleFixture
	: public VSPTests::FBaseTestFixture<
		  EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter>
{
public:
	FExampleFixture() = default;
	FExampleFixture(int InNumber) : Number(InNumber)
	{
	}

private:
	// these methods overrides are optional
	virtual void SetUp() override
	{
		InitNumber = Number;
	}
	virtual void TearDown() override
	{
		VSP_EXPECT_EQ(Number, InitNumber);
	}

protected:
	int Number = 42;

private:
	int InitNumber = 0;
};

// default test fixture will be created with int Number = 42
VSP_TEST_F(VSPTests, FExampleFixture, Equal)
{
	VSP_EXPECT_EQ(Number, 42);
	// the test will end immediately after the exit
}

// default test fixture will be created with int Number = 42
VSP_TEST_F(VSPTests, FExampleFixture, NotEqual)
{
	srand(time(nullptr));

	// test won't end here
	// lambda (latent command) will be called every frame instead
	// as soon as the lambda returns true, the command will be considered completed
	SetUpdate(
		[this]()
		{
			if (rand() % 5 == 0)
			{
				VSP_EXPECT_EQ(Number, 42);
				return true;
			}

			return false;
		});
}

// test fixtures with Number = 41 and Number = 43 will be created
// default fixture won't be created
// test body will run for every created fixture
VSP_TEST_F(VSPTests, FExampleFixture, EqualCustomFixture, FExampleFixture(41), FExampleFixture(43))
{
	VSP_EXPECT_TRUE(Number == 41 || Number == 43);
	// the test will end immediately after the exit
}

class FDerivedExampleFixture : public FExampleFixture
{
public:
	using FExampleFixture::FExampleFixture;
};

// if fixture creation is way too complicated, RegisterFixtures() may be used to create it
// these fixtures will be added to all tests using this fixture type
template<>
TArray<FDerivedExampleFixture> VSPTests::RegisterFixtures()
{
	TArray<FDerivedExampleFixture> Fixtures;
	for (int32 i = 0; i < 10; ++i)
		Fixtures.Add(FDerivedExampleFixture(i));

	return Fixtures;
}

// 10 test fixtures will be created by RegisterFixtures<FDerivedExampleFixture>() instance
// test body will run for every created fixture
VSP_TEST_F(VSPTests, FDerivedExampleFixture, EqualCustomFixture)
{
	VSP_EXPECT_TRUE(Number >= 0 && Number <= 9);
	// the test will end immediately after the exit
}
