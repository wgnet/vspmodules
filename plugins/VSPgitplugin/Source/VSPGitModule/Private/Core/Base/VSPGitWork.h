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

// Engine headers
#include "ISourceControlProvider.h"

/**
* Used to execute Git commands multi-threaded.
*/
class FVSPGitWork : public IQueuedWork
{
public:
	FVSPGitWork(
		const TSharedRef< class ISourceControlOperation, ESPMode::ThreadSafe >& InOperation,
		const TSharedRef< class IVSPGitWorker, ESPMode::ThreadSafe >& InWorker,
		const TSharedRef< class FVSPGitSettings, ESPMode::ThreadSafe >& InSettings,
		const FSourceControlOperationComplete& InOperationCompleteDelegate = FSourceControlOperationComplete() );

	/**
	* This is where the real thread work is done. All work that is done for
	* this queued object should be done from within the call to this function.
	*/
	bool DoWork();

	/**
	* Tells the queued work that it is being abandoned so that it can do
	* per object clean up as needed. This will only be called if it is being
	* abandoned before completion. NOTE: This requires the object to delete
	* itself using whatever heap it was allocated in.
	*/
	virtual void Abandon() override;

	/**
	* This method is also used to tell the object to cleanup but not before
	* the object has finished it's work.
	*/
	virtual void DoThreadedWork() override;

	/** Save any results and call any registered callbacks. */
	ECommandResult::Type ReturnResults();

public:
	/** Files to perform this operation on */
	TArray< FString > Files;

	TQueue< FString > OutputQueue;

	/** Operation we want to perform - contains outward-facing parameters & results */
	TSharedRef< class ISourceControlOperation, ESPMode::ThreadSafe > Operation;
	/** The object that will actually do the work */
	TSharedRef< class IVSPGitWorker, ESPMode::ThreadSafe > Worker;
	/** Ref of settings */
	TSharedRef< class FVSPGitSettings, ESPMode::ThreadSafe > Settings;
	/** Delegate to notify when this operation completes */
	FSourceControlOperationComplete OperationCompleteDelegate;

	FString CommitId;
	FString CommitSummary;

	/** If true, the source control command succeeded */
	bool bCommandSuccessful;

	/**If true, this command has been processed by the source control thread*/
	volatile int32 bExecuteProcessed;
	/** If true, this command will be automatically cleaned up in Tick() */
	bool bAutoDelete;
	/** Whether we are running multi-treaded or not*/
	EConcurrency::Type Concurrency;
};
