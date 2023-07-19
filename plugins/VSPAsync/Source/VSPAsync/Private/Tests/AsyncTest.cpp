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
/*
#define VSP_ASYNC_TESTING

#include "For.h"
#include "TasksTestFixture.h"
#include "Transform.h"
#include "WhenAll.h"
#include "WhenAny.h"

#include "Algo/AllOf.h"
#include "Algo/Count.h"

// ---------------------------------------------------------------------------------------------------------------------

class FWhenAnyTest : public FBaseAsyncTaskTest
{
protected:
	TArray<VSPAsync::TTaskPtr<void>> WhenAnyTasks;
	int32 NumWhenAnyCalls = 0;
	VSPAsync::TTaskPtr<void> WhenAnyNextTask;
};

VSP_TEST_F(AsyncFunctions, FWhenAnyTest, Main)
{
	// test WhenAny with container
	for (int32 i = 0; i < 10; ++i)
		WhenAnyTasks.Add(VSPAsync::CreateEmptyTask());

	WhenAnyNextTask = VSPAsync::WhenAny(WhenAnyTasks)
		->Next(
			[this]()
			{
				++NumWhenAnyCalls;

				const int32 NumFinished = Algo::CountIf(
					WhenAnyTasks,
					[](const auto& Task)
					{
						return Task->IsFinished();
					});

				VSP_EXPECT_TRUE(NumFinished > 0);
			});

	for (auto& Task : WhenAnyTasks)
		Task->StartGraph();

	SetUpdate(
		[this]()
		{
			if (WhenAnyNextTask->IsFinished())
			{
				VSP_EXPECT_EQ(NumWhenAnyCalls, 1);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FWhenAnyPackTest : public FBaseAsyncTaskTest
{
protected:
	int32 NumWhenAnyPackCalls = 0;
	VSPAsync::TTaskPtr<int> WhenAnyTask1;
	VSPAsync::TTaskPtr<FString, void, int> WhenAnyTask2;
	VSPAsync::TTaskPtr<void> WhenAnyTask3;
	VSPAsync::TTaskPtr<void> WhenAnyPackNextTask;
};

VSP_TEST_F(AsyncFunctions, FWhenAnyPackTest, Main)
{
	// test WhenAny with param pack
	WhenAnyTask1 = VSPAsync::CreateTask(
		[]()
		{
			return 1;
		});
	WhenAnyTask2 = VSPAsync::CreateTask<int>(
		[]()
		{
			return FString();
		});
	WhenAnyTask3 = VSPAsync::CreateTask(
		[]()
		{
		});

	WhenAnyPackNextTask = VSPAsync::WhenAny(WhenAnyTask1, WhenAnyTask2, WhenAnyTask3)
		->Next(
			[this]()
			{
				++NumWhenAnyPackCalls;
				VSP_EXPECT_TRUE(WhenAnyTask1->IsFinished() || WhenAnyTask2->IsFinished() || WhenAnyTask3->IsFinished());
			});

	WhenAnyTask1->StartGraph();
	WhenAnyTask2->StartGraph();
	WhenAnyTask3->StartGraph();

	SetUpdate(
		[this]()
		{
			if (WhenAnyPackNextTask->IsFinished())
			{
				VSP_EXPECT_EQ(NumWhenAnyPackCalls, 1);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FWhenAllTest : public FBaseAsyncTaskTest
{
protected:
	TArray<VSPAsync::TTaskPtr<void>> WhenAllTasks;
	int32 NumWhenAllCalls = 0;
	VSPAsync::TTaskPtr<void> WhenAllNextTask;
};

VSP_TEST_F(AsyncFunctions, FWhenAllTest, Main)
{
	// test WhenAll with container
	for (int32 i = 0; i < 10; ++i)
		WhenAllTasks.Add(VSPAsync::CreateEmptyTask());

	WhenAllNextTask = VSPAsync::WhenAll(WhenAllTasks)
		->Next(
			[this]()
			{
				++NumWhenAllCalls;

				const int32 NumFinished = Algo::CountIf(
					WhenAllTasks,
					[](const auto& Task)
					{
						return Task->IsFinished();
					});

				VSP_EXPECT_EQ(NumFinished, WhenAllTasks.Num());
			});

	for (auto& Task : WhenAllTasks)
		Task->StartGraph();

	SetUpdate(
		[this]()
		{
			if (WhenAllNextTask->IsFinished())
			{
				VSP_EXPECT_EQ(NumWhenAllCalls, 1);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FWhenAllPackTest : public FBaseAsyncTaskTest
{
protected:
	int32 NumWhenAllPackCalls = 0;
	VSPAsync::TTaskPtr<int> WhenAllTask1;
	VSPAsync::TTaskPtr<FString, void, int> WhenAllTask2;
	VSPAsync::TTaskPtr<void> WhenAllTask3;
	VSPAsync::TTaskPtr<void> WhenAllPackNextTask;
};

VSP_TEST_F(AsyncFunctions, FWhenAllPackTest, Main)
{
	// test WhenAll with param pack
	WhenAllTask1 = VSPAsync::CreateTask(
		[]()
		{
			return 1;
		});
	WhenAllTask2 = VSPAsync::CreateTask<int>(
		[]()
		{
			return FString();
		});
	WhenAllTask3 = VSPAsync::CreateTask(
		[]()
		{
		});

	WhenAllPackNextTask = VSPAsync::WhenAll(WhenAllTask1, WhenAllTask2, WhenAllTask3)
		->Next(
			[this]()
			{
				++NumWhenAllPackCalls;
				VSP_EXPECT_TRUE(WhenAllTask1->IsFinished() && WhenAllTask2->IsFinished() && WhenAllTask3->IsFinished());
			});

	WhenAllTask1->StartGraph();
	WhenAllTask2->StartGraph();
	WhenAllTask3->StartGraph();

	SetUpdate(
		[this]()
		{
			if (WhenAllPackNextTask->IsFinished())
			{
				VSP_EXPECT_EQ(NumWhenAllPackCalls, 1);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FForTest : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<> WhenAll;
	int ForResult = 0;
};

VSP_TEST_F(AsyncFunctions, FForTest, Main)
{
	// test For
	WhenAll = VSPAsync::For(
		0,
		10,
		2,
		[this](int Index)
		{
			ForResult += Index;
		});

	WhenAll->Next(
		[this]()
		{
			VSP_EXPECT_EQ(ForResult, 20);
		});

	SetUpdate(
		[this]()
		{
			return WhenAll->IsFinished();
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FForNoTaskTest : public FBaseAsyncTaskTest
{
protected:
	int ForResult = 0;
};

VSP_TEST_F(AsyncFunctions, FForNoTaskTest, Main)
{
	// test ForNoTask
	VSPAsync::ForNoTask(
		0,
		10,
		2,
		[this](int Index)
		{
			ForResult += Index;
		});

	SetUpdate(
		[this]()
		{
			if (ForResult == 20)
				return true;

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FForEachNoTaskTest : public FBaseAsyncTaskTest
{
protected:
	int ForEachResult = 0;
};

VSP_TEST_F(AsyncFunctions, FForEachNoTaskTest, Main)
{
	// test ForEachNoTask
	TArray<FNotInt> ForEachNumbers;
	for (int32 Index = 0; Index < 5; ++Index)
		ForEachNumbers.Add(FNotInt { Index * 3 });

	VSPAsync::ForEachNoTask(
		ForEachNumbers,
		[this](const FNotInt& NotInt)
		{
			ForEachResult += NotInt.Int;
		});

	SetUpdate(
		[this]()
		{
			if (ForEachResult == 30)
				return true;

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FForEachTest : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<> WhenAll;
	int ForEachResult = 0;
};

VSP_TEST_F(AsyncFunctions, FForEachTest, Main)
{
	// test ForEach
	TArray<FNotInt> ForEachNumbers;
	for (int32 Index = 0; Index < 5; ++Index)
		ForEachNumbers.Add(FNotInt { Index * 3 });

	WhenAll = VSPAsync::ForEach(
		ForEachNumbers,
		[this](const FNotInt& NotInt)
		{
			ForEachResult += NotInt.Int;
		});

	WhenAll->Next(
		[this]()
		{
			VSP_EXPECT_EQ(ForEachResult, 30);
		});

	SetUpdate(
		[this]()
		{
			return WhenAll->IsFinished();
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FForEachNoCopyNoTaskTest : public FBaseAsyncTaskTest
{
protected:
	int ForEachNoCopyResult = 0;
	TArray<FNotInt> ForEachNoCopyNumbers;
};

VSP_TEST_F(AsyncFunctions, FForEachNoCopyNoTaskTest, Main)
{
	// test ForEachNoCopy
	for (int32 Index = 0; Index < 5; ++Index)
		ForEachNoCopyNumbers.Add(FNotInt { Index * 3 });

	VSPAsync::ForEachNoCopyNoTask(
		ForEachNoCopyNumbers,
		[this](FNotInt& NotInt)
		{
			NotInt.Int = -1;
		});

	SetUpdate(
		[this]()
		{
			if (Algo::AllOf(
					ForEachNoCopyNumbers,
					[](const FNotInt& NotInt)
					{
						return NotInt.Int == -1;
					}))
			{
				return true;
			}

			return false;
		});
}


// ---------------------------------------------------------------------------------------------------------------------

class FForEachNoCopyTest : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<> WhenAll;
	int ForEachNoCopyResult = 0;
	TArray<FNotInt> ForEachNoCopyNumbers;
};

VSP_TEST_F(AsyncFunctions, FForEachNoCopyTest, Main)
{
	// test ForEachNoCopy
	for (int32 Index = 0; Index < 5; ++Index)
		ForEachNoCopyNumbers.Add(FNotInt { Index * 3 });

	WhenAll = VSPAsync::ForEachNoCopy(
		ForEachNoCopyNumbers,
		[this](FNotInt& NotInt)
		{
			NotInt.Int = -1;
		});

	WhenAll->Next(
		[this]()
		{
			VSP_EXPECT_TRUE(Algo::AllOf(
				ForEachNoCopyNumbers,
				[](const FNotInt& NotInt)
				{
					return NotInt.Int == -1;
				}));
		});

	SetUpdate(
		[this]()
		{
			return WhenAll->IsFinished();
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FTransformIfNoTaskTest : public FBaseAsyncTaskTest
{
protected:
	TArray<int32> TransformIfOutputNumbers;
};

VSP_TEST_F(AsyncFunctions, FTransformIfNoTaskTest, Main)
{
	// test TransformIf
	TArray<FNotInt> TransformIfInputNumbers;
	for (int32 Index = 0; Index < 10; ++Index)
		TransformIfInputNumbers.Add(FNotInt { Index });

	VSPAsync::TransformIfNoTask(
		TransformIfInputNumbers,
		TransformIfOutputNumbers,
		[](const FNotInt& NotInt)
		{
			return NotInt.Int % 2 == 0;
		},
		[](const FNotInt& NotInt)
		{
			return NotInt.Int;
		});

	SetUpdate(
		[this]()
		{
			if (TransformIfOutputNumbers.Num() == 5)
			{
				VSP_EXPECT_EQ(TransformIfOutputNumbers[0], 0);
				VSP_EXPECT_EQ(TransformIfOutputNumbers[1], 2);
				VSP_EXPECT_EQ(TransformIfOutputNumbers[2], 4);
				VSP_EXPECT_EQ(TransformIfOutputNumbers[3], 6);
				VSP_EXPECT_EQ(TransformIfOutputNumbers[4], 8);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FTransformIfTest : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<> WhenAll;
	TArray<int32> TransformIfOutputNumbers;
};

VSP_TEST_F(AsyncFunctions, FTransformIfTest, Main)
{
	// test TransformIf
	TArray<FNotInt> TransformIfInputNumbers;
	for (int32 Index = 0; Index < 10; ++Index)
		TransformIfInputNumbers.Add(FNotInt { Index });

	WhenAll = VSPAsync::TransformIf(
		TransformIfInputNumbers,
		TransformIfOutputNumbers,
		[](const FNotInt& NotInt)
		{
			return NotInt.Int % 2 == 0;
		},
		[](const FNotInt& NotInt)
		{
			return NotInt.Int;
		});

	WhenAll->Next(
		[this]()
		{
			VSP_EXPECT_EQ(TransformIfOutputNumbers[0], 0);
			VSP_EXPECT_EQ(TransformIfOutputNumbers[1], 2);
			VSP_EXPECT_EQ(TransformIfOutputNumbers[2], 4);
			VSP_EXPECT_EQ(TransformIfOutputNumbers[3], 6);
			VSP_EXPECT_EQ(TransformIfOutputNumbers[4], 8);
		});

	SetUpdate(
		[this]()
		{
			return WhenAll->IsFinished();
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FTransformIfNoCopyTest : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<> WhenAll;
	TArray<FNotInt> TransformIfNoCopyInputNumbers;
	TArray<int32> TransformIfNoCopyOutputNumbers;
};

VSP_TEST_F(AsyncFunctions, FTransformIfNoCopyTest, Main)
{
	// test TransformIfNoCopy
	for (int32 Index = 0; Index < 10; ++Index)
		TransformIfNoCopyInputNumbers.Add(FNotInt { Index });

	WhenAll = VSPAsync::TransformIfNoCopy(
		TransformIfNoCopyInputNumbers,
		TransformIfNoCopyOutputNumbers,
		[](const FNotInt& NotInt)
		{
			return NotInt.Int % 2 == 0;
		},
		[](const FNotInt& NotInt)
		{
			return NotInt.Int;
		});

	WhenAll->Next(
		[this]()
		{
			VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[0], 0);
			VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[1], 2);
			VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[2], 4);
			VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[3], 6);
			VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[4], 8);
		});

	SetUpdate(
		[this]()
		{
			return WhenAll->IsFinished();
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FTransformIfNoCopyNoTaskTest : public FBaseAsyncTaskTest
{
protected:
	TArray<FNotInt> TransformIfNoCopyInputNumbers;
	TArray<int32> TransformIfNoCopyOutputNumbers;
};

VSP_TEST_F(AsyncFunctions, FTransformIfNoCopyNoTaskTest, Main)
{
	// test TransformIfNoCopy
	for (int32 Index = 0; Index < 10; ++Index)
		TransformIfNoCopyInputNumbers.Add(FNotInt { Index });

	VSPAsync::TransformIfNoCopyNoTask(
		TransformIfNoCopyInputNumbers,
		TransformIfNoCopyOutputNumbers,
		[](const FNotInt& NotInt)
		{
			return NotInt.Int % 2 == 0;
		},
		[](const FNotInt& NotInt)
		{
			return NotInt.Int;
		});

	SetUpdate(
		[this]()
		{
			if (TransformIfNoCopyOutputNumbers.Num() == 5)
			{
				VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[0], 0);
				VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[1], 2);
				VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[2], 4);
				VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[3], 6);
				VSP_EXPECT_EQ(TransformIfNoCopyOutputNumbers[4], 8);
				return true;
			}

			return false;
		});
}
*/