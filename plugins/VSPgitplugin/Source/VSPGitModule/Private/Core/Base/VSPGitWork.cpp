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
#include "VSPGitWork.h"
// Module headers
#include "Core/Base/IVSPGitWorker.h"

FVSPGitWork::FVSPGitWork(
	const TSharedRef< ISourceControlOperation, ESPMode::ThreadSafe >& InOperation,
	const TSharedRef< IVSPGitWorker, ESPMode::ThreadSafe >& InWorker,
	const TSharedRef< class FVSPGitSettings, ESPMode::ThreadSafe >& InSettings,
	const FSourceControlOperationComplete& InOperationCompleteDelegate )
	: Operation( InOperation )
	, Worker( InWorker )
	, Settings( InSettings )
	, OperationCompleteDelegate( InOperationCompleteDelegate )
	, bCommandSuccessful( false )
	, bExecuteProcessed( 0 )
	, bAutoDelete( true )
	, Concurrency()
{}

bool FVSPGitWork::DoWork()
{
	bCommandSuccessful = Worker->Execute( *this );
	FPlatformAtomics::InterlockedExchange( &bExecuteProcessed, 1 );

	return bCommandSuccessful;
}

void FVSPGitWork::Abandon()
{
	FPlatformAtomics::InterlockedExchange( &bExecuteProcessed, 1 );
}

void FVSPGitWork::DoThreadedWork()
{
	Concurrency = EConcurrency::Asynchronous;
	DoWork();
}

ECommandResult::Type FVSPGitWork::ReturnResults()
{
	FSourceControlResultInfo TextResult = Operation->GetResultInfo();

	const auto Result = bCommandSuccessful ? ECommandResult::Succeeded : ECommandResult::Failed;
	// ReSharper disable once CppExpressionWithoutSideEffects
	OperationCompleteDelegate.ExecuteIfBound( Operation, Result );
	return Result;
}
