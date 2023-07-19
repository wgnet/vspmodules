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

#include "Algo/AnyOf.h"

#define VSP_STRINGIFY(str) #str

#define _VSP_TEST_F(GroupName, FixtureClass, TestName, ...)                                                            \
	IMPLEMENT_SIMPLE_AUTOMATION_TEST_PRIVATE(                                                                         \
		F##GroupName##FixtureClass##TestName,                                                                         \
		FAutomationTestBase,                                                                                          \
		"VSP." #GroupName "." #FixtureClass "." #TestName,                                                             \
		FixtureClass::_FixtureFlags,                                                                                  \
		__FILE__,                                                                                                     \
		__LINE__)                                                                                                     \
	inline bool F##GroupName##FixtureClass##TestName::RunTest(const FString&)                                         \
	{                                                                                                                 \
		return false;                                                                                                 \
	}                                                                                                                 \
                                                                                                                      \
	class F##GroupName##FixtureClass##TestName##FixtureImpl : public FixtureClass                                     \
	{                                                                                                                 \
	public:                                                                                                           \
		F##GroupName##FixtureClass##TestName##FixtureImpl() = default;                                                \
		F##GroupName##FixtureClass##TestName##FixtureImpl(FixtureClass InFixtureClass)                                \
			: FixtureClass(MoveTemp(InFixtureClass))                                                                  \
		{                                                                                                             \
		}                                                                                                             \
                                                                                                                      \
		virtual void _DoRunTest(const FString& Params) override;                                                      \
	};                                                                                                                \
                                                                                                                      \
	class F##GroupName##FixtureClass##TestName##Impl                                                                  \
		: public F##GroupName##FixtureClass##TestName                                                                 \
		, public IAutomationLatentCommand                                                                             \
	{                                                                                                                 \
	public:                                                                                                           \
		F##GroupName##FixtureClass##TestName##Impl(const FString& Name, TArray<FixtureClass> InTestFixtures)          \
			: F##GroupName##FixtureClass##TestName(Name)                                                              \
		{                                                                                                             \
			for (auto& InTestFixture : InTestFixtures)                                                                \
			{                                                                                                         \
				TestFixtures.Add(                                                                                     \
					MakeShared<F##GroupName##FixtureClass##TestName##FixtureImpl>(MoveTemp(InTestFixture)));          \
			}                                                                                                         \
                                                                                                                      \
			for (auto& RegisteredFixture : VSPTests::RegisterFixtures<FixtureClass>())                                 \
			{                                                                                                         \
				TestFixtures.Add(                                                                                     \
					MakeShared<F##GroupName##FixtureClass##TestName##FixtureImpl>(MoveTemp(RegisteredFixture)));      \
			}                                                                                                         \
                                                                                                                      \
			if (TestFixtures.Num() == 0)                                                                              \
				TestFixtures.Add(MakeShared<F##GroupName##FixtureClass##TestName##FixtureImpl>());                    \
		}                                                                                                             \
                                                                                                                      \
		virtual bool RunTest(const FString& Params) override                                                          \
		{                                                                                                             \
			for (const auto& TestFixture : TestFixtures)                                                              \
				TestFixture->_DoRunTestImpl(this, Params);                                                            \
                                                                                                                      \
			if (Algo::AnyOf(                                                                                          \
					TestFixtures,                                                                                     \
					[](const auto& TestFixture)                                                                       \
					{                                                                                                 \
						return TestFixture->_IsUpdatable();                                                           \
					}))                                                                                               \
			{                                                                                                         \
				FAutomationTestFramework::Get().EnqueueLatentCommand(AsShared());                                     \
			}                                                                                                         \
			else                                                                                                      \
			{                                                                                                         \
				TestFixtures.Empty();                                                                                 \
				FixtureClass::AfterTearDown();                                                                        \
			}                                                                                                         \
                                                                                                                      \
			return true;                                                                                              \
		}                                                                                                             \
                                                                                                                      \
		virtual bool Update() override                                                                                \
		{                                                                                                             \
			TestFixtures.RemoveAll(                                                                                   \
				[](auto& TestFixture)                                                                                 \
				{                                                                                                     \
					return !TestFixture->_IsUpdatable() || TestFixture->_Update();                                    \
				});                                                                                                   \
                                                                                                                      \
			if (TestFixtures.Num() == 0)                                                                              \
			{                                                                                                         \
				FixtureClass::AfterTearDown();                                                                        \
				return true;                                                                                          \
			}                                                                                                         \
			else                                                                                                      \
			{                                                                                                         \
				return false;                                                                                         \
			}                                                                                                         \
		}                                                                                                             \
                                                                                                                      \
	private:                                                                                                          \
		TArray<TSharedRef<F##GroupName##FixtureClass##TestName##FixtureImpl>> TestFixtures;                           \
	};                                                                                                                \
                                                                                                                      \
	namespace                                                                                                         \
	{                                                                                                                 \
		auto F##GroupName##FixtureClass##TestName##Instance = MakeShared<F##GroupName##FixtureClass##TestName##Impl>( \
			TEXT(VSP_STRINGIFY(F##GroupName##FixtureClass##TestName##Impl)),                                           \
			TArray<FixtureClass> { __VA_ARGS__ });                                                                    \
	}                                                                                                                 \
                                                                                                                      \
	inline void F##GroupName##FixtureClass##TestName##FixtureImpl::_DoRunTest(const FString& Params)
