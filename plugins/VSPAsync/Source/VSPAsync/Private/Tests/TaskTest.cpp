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

#include "Run.h"
#include "TasksTestFixture.h"

#include <chrono>
#include <string>
#include <thread>

// ---------------------------------------------------------------------------------------------------------------------

class FSuccessfulSimpleChain : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<int, int> SuccessfulSimpleChainTask;
};

VSP_TEST_F(AsyncTask, FSuccessfulSimpleChain, Main)
{
	// check successful result
	VSPAsync::TTaskPtr<int, void, FString> StartTask = VSPAsync::CreateTask(
		[]()
		{
			return 4;
		});

	SuccessfulSimpleChainTask = StartTask
		->Next(
			[](size_t answer)
			{
				return std::to_string(answer) + "0";
			})
		->Next(
			[](const std::string& answer)
			{
				return std::stoi(answer) + 5;
			})
		->Next(
			[](int answer)
			{
				return answer - 3;
			});

	SuccessfulSimpleChainTask->StartGraph();

	SetUpdate(
		[this]()
		{
			if (SuccessfulSimpleChainTask->IsSuccessful())
			{
				const auto TaskResult = SuccessfulSimpleChainTask->GetSuccess();
				VSP_EXPECT_TRUE(TaskResult && *TaskResult == 42);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FUnsuccessfulSimpleChain : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<std::string, int> UnsuccessfulSimpleChainTask;
};

VSP_TEST_F(AsyncTask, FUnsuccessfulSimpleChain, Main)
{
	// check unsuccessful result
	auto StartTask = VSPAsync::CreateTask(
		[]()
		{
			return 4;
		});
	UnsuccessfulSimpleChainTask = StartTask->Next(
		[](int)
		{
			return VSPAsync::TTask<std::string, int>::MakeFailure(TEXT("Task failed"));
		},
		[](const FString&, const FString&)
		{
		}); // do not assert

	UnsuccessfulSimpleChainTask->StartGraph();

	SetUpdate(
		[this]()
		{
			if (UnsuccessfulSimpleChainTask->IsFailed())
			{
				const auto TaskResult = UnsuccessfulSimpleChainTask->GetFailure();
				VSP_EXPECT_TRUE(TaskResult && *TaskResult == TEXT("Task failed"));
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FCustomFailureTask : public FBaseAsyncTaskTest
{
protected:
	enum class EFailureType
	{
		Bad,
		ReallyBad,
		Unknown
	};

	VSPAsync::TTaskPtr<std::string, int, EFailureType> CustomFailureTask;
};

VSP_TEST_F(AsyncTask, FCustomFailureTask, Main)
{
	// check custom failure type
	auto StartTask = VSPAsync::CreateTask<EFailureType>(
		[]()
		{
			return 4;
		});

	CustomFailureTask = StartTask->Next(
		[](int)
		{
			return VSPAsync::TTask<std::string, int, EFailureType>::MakeFailure(EFailureType::ReallyBad);
		},
		[](const FString&, EFailureType)
		{
		}); // do not assert

	CustomFailureTask->StartGraph();

	SetUpdate(
		[this]()
		{
			if (CustomFailureTask->IsFailed())
			{
				const auto TaskResult = CustomFailureTask->GetFailure();
				VSP_EXPECT_TRUE(TaskResult && *TaskResult == EFailureType::ReallyBad);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FNextIsTaskTask : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<int, int> NextIsTaskTask;
	int NextIsTaskTaskCounter = 0;
};

VSP_TEST_F(AsyncTask, FNextIsTaskTask, Main)
{
	// check empty function args as next
	auto VoidAfterIntContinuation = [this]()
	{
		VSP_EXPECT_EQ(NextIsTaskTaskCounter, 3);
	};

	auto StartTask = VSPAsync::CreateTask(
		[this]()
		{
			++NextIsTaskTaskCounter;
		});
	auto Task2 = StartTask->Next(
		[this]()
		{
			++NextIsTaskTaskCounter;
		});
	auto Task3 = Task2->Next(
		[this]()
		{
			++NextIsTaskTaskCounter;
			return 4;
		});
	auto Task4 = Task3->Next(VoidAfterIntContinuation);

	// check NextTask
	auto Task5 = VSPAsync::CreateTask(
		[]()
		{
			return 4;
		});

	NextIsTaskTask = StartTask->NextTask(MoveTemp(Task5))
		->Next(
			[](size_t answer)
			{
				return std::to_string(answer) + "0";
			})
		->Next(
			[](const std::string& answer)
			{
				return std::stoi(answer) + 5;
			})
		->Next(
			[](int answer)
			{
				return answer - 3;
			});

	NextIsTaskTask->StartGraph();

	SetUpdate(
		[this]()
		{
			if (NextIsTaskTask->IsSuccessful())
			{
				const auto TaskResult = NextIsTaskTask->GetSuccess();
				VSP_EXPECT_TRUE(TaskResult && *TaskResult == 42);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FCancelledTask : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<int> StartTask;
	VSPAsync::TTaskPtr<int, int> CancelledTask;
};

VSP_TEST_F(AsyncTask, FCancelledTask, Main)
{
	// check cancellation
	StartTask = VSPAsync::CreateTask(
		[]()
		{
			return 42;
		});
	CancelledTask = StartTask->Next(
		[](int Result)
		{
			return Result;
		});
	CancelledTask->StartGraph();

	VSP_EXPECT_TRUE(!CancelledTask->IsStarted());
	VSP_EXPECT_TRUE(!CancelledTask->IsCancelled());
	VSP_EXPECT_TRUE(!CancelledTask->IsInProgress());
	VSP_EXPECT_TRUE(!CancelledTask->IsFinished());
	VSP_EXPECT_TRUE(!CancelledTask->IsSuccessful());
	VSP_EXPECT_TRUE(!CancelledTask->IsFailed());
	VSP_EXPECT_TRUE(CancelledTask->IsGraphStarted());
	VSP_EXPECT_TRUE(!CancelledTask->IsGraphCompleted());

	VSP_EXPECT_EQ(CancelledTask->Cancel(), EAsyncCancellationResult::Success);

	VSP_EXPECT_TRUE(!CancelledTask->IsStarted());
	VSP_EXPECT_TRUE(CancelledTask->IsCancelled());
	VSP_EXPECT_TRUE(!CancelledTask->IsInProgress());
	VSP_EXPECT_TRUE(!CancelledTask->IsFinished());
	VSP_EXPECT_TRUE(!CancelledTask->IsSuccessful());
	VSP_EXPECT_TRUE(!CancelledTask->IsFailed());
	VSP_EXPECT_TRUE(CancelledTask->IsGraphStarted());
	VSP_EXPECT_TRUE(!CancelledTask->IsGraphCompleted());

	SetUpdate(
		[this]()
		{
			if (StartTask->IsSuccessful())
			{
				const auto StartTaskResult = StartTask->GetSuccess();
				VSP_EXPECT_EQ(StartTaskResult && *StartTaskResult, 42);

				VSP_EXPECT_TRUE(!CancelledTask->IsStarted());
				VSP_EXPECT_TRUE(CancelledTask->IsCancelled());
				VSP_EXPECT_TRUE(!CancelledTask->IsInProgress());
				VSP_EXPECT_TRUE(!CancelledTask->IsFinished());
				VSP_EXPECT_TRUE(!CancelledTask->IsSuccessful());
				VSP_EXPECT_TRUE(!CancelledTask->IsFailed());
				VSP_EXPECT_TRUE(CancelledTask->IsGraphStarted());
				VSP_EXPECT_TRUE(CancelledTask->IsGraphCompleted());
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FTaskLifetime : public FBaseAsyncTaskTest
{
protected:
	TOptional<int> TaskLifetimeResult1;
	TOptional<int> TaskLifetimeResult2;
};

VSP_TEST_F(AsyncTask, FTaskLifetime, Main)
{
	// check task lifetime (no task object)
	VSPAsync::CreateTask(
		[this]()
		{
			TaskLifetimeResult1 = 1;
			return 2;
		})
		->Next(
			[this](int Result)
			{
				TaskLifetimeResult2 = Result;
			})
		->StartGraph();

	SetUpdate(
		[this]()
		{
			if (TaskLifetimeResult1.IsSet() && TaskLifetimeResult2.IsSet())
			{
				VSP_EXPECT_EQ(*TaskLifetimeResult1, 1);
				VSP_EXPECT_EQ(*TaskLifetimeResult2, 2);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FReturnAwaitingTask : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<int> StartTask;
};

VSP_TEST_F(AsyncTask, FReturnAwaitingTask, Success)
{
	StartTask = VSPAsync::Run(
		[]()
		{
			return VSPAsync::Run(
				[]()
				{
					return 42;
				});
		});

	SetUpdate(
		[this]()
		{
			if (StartTask->IsSuccessful())
			{
				const auto TaskResult = StartTask->GetSuccess();
				VSP_EXPECT_TRUE(TaskResult && *TaskResult == 42);
				return true;
			}

			return false;
		});
}

VSP_TEST_F(AsyncTask, FReturnAwaitingTask, Failure)
{
	StartTask = VSPAsync::CreateTask(
		[]()
		{
			return VSPAsync::Run(
				[]()
				{
					return VSPAsync::TTask<int>::MakeFailure(TEXT("Task failed"));
				},
				[](const FString&, const FString&)
				{
				});
		},
		[](const FString&, const FString&)
		{
		});

	StartTask->StartGraph();

	SetUpdate(
		[this]()
		{
			if (StartTask->IsFailed())
			{
				const auto TaskResult = StartTask->GetFailure();
				VSP_EXPECT_TRUE(TaskResult && *TaskResult == TEXT("Task failed"));
				return true;
			}

			return false;
		});
}

VSP_TEST_F(AsyncTask, FReturnAwaitingTask, Wait)
{
	StartTask = VSPAsync::Run(
		[]()
		{
			return VSPAsync::Run(
				[]()
				{
					return 42;
				},
				EAsyncExecution::ThreadPool);
		},
		EAsyncExecution::ThreadPool);

	StartTask->Wait();

	const auto TaskResult = StartTask->GetSuccess();
	VSP_EXPECT_TRUE(TaskResult && *TaskResult == 42);
}

// ---------------------------------------------------------------------------------------------------------------------

VSP_TEST(AsyncTask, Pipeline, AsyncTestsFlags)
{
	// check pipeline and statuses
	auto ParentTask = VSPAsync::CreateTask(
		[]()
		{
			return 4;
		},
		EAsyncExecution::ThreadPool);

	VSP_EXPECT_TRUE(!ParentTask->IsStarted());
	VSP_EXPECT_TRUE(!ParentTask->IsCancelled());
	VSP_EXPECT_TRUE(!ParentTask->IsInProgress());
	VSP_EXPECT_TRUE(!ParentTask->IsFinished());
	VSP_EXPECT_TRUE(!ParentTask->IsSuccessful());
	VSP_EXPECT_TRUE(!ParentTask->IsFailed());
	VSP_EXPECT_TRUE(!ParentTask->IsGraphStarted());
	VSP_EXPECT_TRUE(!ParentTask->IsGraphCompleted());

	auto ChildTask = ParentTask->Next(
		[](int32 Num)
		{
			return Num + 38;
		},
		EAsyncExecution::ThreadPool);

	VSP_EXPECT_TRUE(!ParentTask->IsStarted());
	VSP_EXPECT_TRUE(!ParentTask->IsCancelled());
	VSP_EXPECT_TRUE(!ParentTask->IsInProgress());
	VSP_EXPECT_TRUE(!ParentTask->IsFinished());
	VSP_EXPECT_TRUE(!ParentTask->IsSuccessful());
	VSP_EXPECT_TRUE(!ParentTask->IsFailed());
	VSP_EXPECT_TRUE(!ParentTask->IsGraphStarted());
	VSP_EXPECT_TRUE(!ParentTask->IsGraphCompleted());

	VSP_EXPECT_TRUE(!ChildTask->IsStarted());
	VSP_EXPECT_TRUE(!ChildTask->IsCancelled());
	VSP_EXPECT_TRUE(!ChildTask->IsInProgress());
	VSP_EXPECT_TRUE(!ChildTask->IsFinished());
	VSP_EXPECT_TRUE(!ChildTask->IsSuccessful());
	VSP_EXPECT_TRUE(!ChildTask->IsFailed());
	VSP_EXPECT_TRUE(!ChildTask->IsGraphStarted());
	VSP_EXPECT_TRUE(!ChildTask->IsGraphCompleted());

	ChildTask->StartGraph();
	VSP_EXPECT_TRUE(ParentTask->IsStarted());

	ParentTask->Wait();
	VSP_EXPECT_TRUE(ParentTask->IsSuccessful());
	const auto ParentTaskResult = ParentTask->GetSuccess();
	VSP_EXPECT_TRUE(ParentTaskResult && *ParentTaskResult == 4);

	VSP_EXPECT_TRUE(ParentTask->IsStarted());
	VSP_EXPECT_TRUE(!ParentTask->IsInProgress());
	VSP_EXPECT_TRUE(ParentTask->IsFinished());
	VSP_EXPECT_TRUE(ParentTask->IsGraphStarted());

	ChildTask->Wait();
	VSP_EXPECT_TRUE(ChildTask->IsSuccessful());
	const auto ChildTaskResult = ChildTask->GetSuccess();
	VSP_EXPECT_TRUE(ChildTaskResult && *ChildTaskResult == 42);

	VSP_EXPECT_TRUE(ChildTask->IsStarted());
	VSP_EXPECT_TRUE(!ChildTask->IsCancelled());
	VSP_EXPECT_TRUE(!ChildTask->IsInProgress());
	VSP_EXPECT_TRUE(ChildTask->IsFinished());
	VSP_EXPECT_TRUE(ChildTask->IsSuccessful());
	VSP_EXPECT_TRUE(!ChildTask->IsFailed());
	VSP_EXPECT_TRUE(ChildTask->IsGraphStarted());
	VSP_EXPECT_TRUE(ChildTask->IsGraphCompleted());

	return true;
}

// ---------------------------------------------------------------------------------------------------------------------

class FAddContinuationToFinishedTask : public FBaseAsyncTaskTest
{
	virtual void SetUp() override
	{
		BaseTask = VSPAsync::CreateTask(
			[]()
			{
				return 42;
			},
			EAsyncExecution::ThreadPool);
	}

protected:
	VSPAsync::TTaskPtr<int> BaseTask;
	VSPAsync::TTaskPtr<FNotInt, int> NextTask;
	VSPAsync::TTaskPtr<float, FNotInt> NextNextTask;
};

VSP_TEST_F(AsyncTask, FAddContinuationToFinishedTask, Success)
{
	NextTask = BaseTask->Next(
		[](int Number)
		{
			return FNotInt { Number + 8 };
		},
		EAsyncExecution::ThreadPool);

	NextTask->StartGraph();
	NextTask->Wait();

	NextNextTask = NextTask->Next(
		[](const FNotInt& NotInt)
		{
			return static_cast<float>(NotInt.Int);
		},
		EAsyncExecution::ThreadPool);

	NextNextTask->Wait();

	const auto Success = NextNextTask->GetSuccess();
	VSP_EXPECT_TRUE(Success);
	if (Success)
		VSP_EXPECT_EQ(*Success, 50.f);
}

VSP_TEST_F(AsyncTask, FAddContinuationToFinishedTask, Failure)
{
	NextTask = BaseTask->Next(
		[](int Number)
		{
			if (Number == 42)
				return VSPAsync::TTask<FNotInt, int>::MakeFailure(TEXT("Failure"));
			else
				return VSPAsync::TTask<FNotInt, int>::MakeSuccess(FNotInt { Number + 8 });
		},
		[](const FString&, const FString&) // do not assert
		{
		},
		EAsyncExecution::ThreadPool);

	NextTask->StartGraph();
	NextTask->Wait();

	NextNextTask = NextTask->Next(
		[](const FNotInt& NotInt)
		{
			return static_cast<float>(NotInt.Int);
		},
		EAsyncExecution::ThreadPool);

	VSP_EXPECT_TRUE(NextNextTask->IsCancelled());
}

// ---------------------------------------------------------------------------------------------------------------------

class FReturnType : public FBaseAsyncTaskTest
{
public:
	enum class ETestType
	{
		Success,
		Failure,
		AwaitSuccess,
		AwaitFailure,
	};

public:
	FReturnType() = default;
	FReturnType(ETestType InTestType) : TestType(InTestType)
	{
	}

protected:
	ETestType TestType = ETestType::Success;
	VSPAsync::TTaskPtr<int> AwaitTask;
	VSPAsync::TTaskPtr<int, int> AwaitTaskSuccessChild;
	VSPAsync::TTaskPtr<int, int> AwaitTaskFailureChild;
};

template<>
TArray<FReturnType> VSPTests::RegisterFixtures()
{
	TArray<FReturnType> Fixtures;

	for (int i = 0; i < 1000; ++i)
		Fixtures.Add(FReturnType(FReturnType::ETestType::Success));

	for (int i = 0; i < 1000; ++i)
		Fixtures.Add(FReturnType(FReturnType::ETestType::Failure));

	for (int i = 0; i < 1000; ++i)
		Fixtures.Add(FReturnType(FReturnType::ETestType::AwaitSuccess));

	for (int i = 0; i < 1000; ++i)
		Fixtures.Add(FReturnType(FReturnType::ETestType::AwaitFailure));

	return Fixtures;
}

VSP_TEST_F(AsyncTask, FReturnType, Main)
{
	if (TestType == ETestType::AwaitSuccess || TestType == ETestType::AwaitFailure)
	{
		AwaitTask = VSPAsync::Run(
			[]()
			{
				return 30;
			},
			EAsyncExecution::ThreadPool);

		if (TestType == ETestType::AwaitSuccess)
		{
			AwaitTaskSuccessChild = AwaitTask->Next(
				[](int Number)
				{
					return Number + 12;
				},
				EAsyncExecution::ThreadPool);
		}
		else
		{
			AwaitTaskFailureChild = AwaitTask->Next(
				[](int Number)
				{
					return VSPAsync::TTask<int, int>::MakeFailure(TEXT("Failure"));
				},
				[](const FString&, const FString&) // do not assert
				{
				},
				EAsyncExecution::ThreadPool);
		}
	}

	auto StartTask = VSPAsync::CreateTask(
		[this]()
		{
			switch (TestType)
			{
			case ETestType::Success:
				return VSPAsync::TTask<int>::MakeSuccess(42);

			case ETestType::Failure:
				return VSPAsync::TTask<int>::MakeFailure(TEXT("Failure"));

			case ETestType::AwaitSuccess:
				return VSPAsync::TTask<int>::MakeAwait(AwaitTaskSuccessChild);

			default:
				return VSPAsync::TTask<int>::MakeAwait(AwaitTaskFailureChild);
			}
		},
		[](const FString&, const FString&) // do not assert
		{
		},
		EAsyncExecution::ThreadPool);

	StartTask->StartGraph();
	StartTask->Wait();

	if (TestType == ETestType::Failure || TestType == ETestType::AwaitFailure)
	{
		const auto Failure = StartTask->GetFailure();
		VSP_EXPECT_TRUE(Failure);
		if (Failure)
			VSP_EXPECT_EQ(*Failure, TEXT("Failure"));
	}
	else
	{
		const auto Success = StartTask->GetSuccess();
		VSP_EXPECT_TRUE(Success);
		if (Success)
			VSP_EXPECT_EQ(*Success, 42);
	}
}

// ---------------------------------------------------------------------------------------------------------------------

class FParentLock : public FBaseAsyncTaskTest
{
protected:
	int Result = 0;
};

template<>
TArray<FParentLock> VSPTests::RegisterFixtures()
{
	TArray<FParentLock> Fixtures;

	for (int i = 0; i < 100; ++i)
		Fixtures.Add(FParentLock());

	return Fixtures;
}

VSP_TEST_F(AsyncTask, FParentLock, Main)
{
	auto CreateResult = [this]()
	{
		auto RootTask = VSPAsync::CreateTask(
			[]()
			{
				return 30;
			});

		RootTask->StartGraph();

		RootTask
			->Next(
				[this](int Number)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					return Number + 12;
				})
			->Next(
				[this](int Number)
				{
					std::this_thread::sleep_for(std::chrono::milliseconds(10));
					Result = Number;
				});
	};

	CreateResult();

	SetUpdate(
		[this]()
		{
			if (Result != 0)
			{
				VSP_EXPECT_EQ(Result, 42);
				return true;
			}

			return false;
		});
}

// ---------------------------------------------------------------------------------------------------------------------

class FAwait : public FBaseAsyncTaskTest
{
protected:
	VSPAsync::TTaskPtr<void> Foo1()
	{
		return VSPAsync::Run(
			[this]()
			{
				Foo1Value = Generate();
			});
	}

	VSPAsync::TTaskPtr<void> Foo2()
	{
		return VSPAsync::CreateTask(
			[this]()
			{
				Foo2Value = Generate();
			});
	}

	VSPAsync::TTaskPtr<void> Foo3()
	{
		return VSPAsync::CreateTask(
			[this]()
			{
				Foo3Value = Generate();
			});
	}

	VSPAsync::TTaskPtr<void> Foo4()
	{
		return VSPAsync::CreateTask(
			[this]()
			{
				Foo4Value = Generate();
			});
	}

	int Generate()
	{
		return ++Generator;
	}

	TUpdateFunction GetUpdateFunction() const
	{
		return [this]()
		{
			if (EndTask->IsFinished())
			{
				VSP_EXPECT_EQ(Foo1Value, 1);
				VSP_EXPECT_EQ(Foo2Value, 2);
				VSP_EXPECT_EQ(Foo3Value, 3);
				VSP_EXPECT_EQ(Foo4Value, 4);
				return true;
			}
			return false;
		};
	}

protected:
	VSPAsync::TTaskPtr<void> EndTask;
	int Generator = 0;
	int Foo1Value = -1;
	int Foo2Value = -1;
	int Foo3Value = -1;
	int Foo4Value = -1;
};

VSP_TEST_F(AsyncTask, FAwait, Await)
{
	EndTask = Foo1()->Await(Foo2())->Await(Foo3())->Await(Foo4());
	SetUpdate(GetUpdateFunction());
}

VSP_TEST_F(AsyncTask, FAwait, AwaitWithNext)
{
	EndTask = Foo1()
		->Next(
			[this]()
			{
				return VSPAsync::TTask<void>::MakeAwait(Foo2(), true);
			})
		->Next(
			[this]()
			{
				return VSPAsync::TTask<void>::MakeAwait(Foo3(), true);
			})
		->Next(
			[this]()
			{
				return VSPAsync::TTask<void>::MakeAwait(Foo4(), true);
			});

	return SetUpdate(GetUpdateFunction());
}
*/