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

// Module headers
#include "VSPGitHelpers.h"
#include "VSPGitWork.h"

class IVSPGitWorker
{
public:
	virtual ~IVSPGitWorker() = default;
	/**
	* Name describing the work that this worker does. Used for factory method hookup.
	*/
	virtual FName GetName() const = 0;

	/**
	* Function that actually does the work. Can be executed on another thread.
	*/
	virtual bool Execute( FVSPGitWork& InWork ) = 0;

	/**
	* Updates the state of any items after completion (if necessary). This is always executed on the main thread.
	* @returns true if states were updated
	*/
	virtual bool UpdateModelStates() const = 0;
};

class FVSPGitWorkerBase : public IVSPGitWorker
{
public:
	virtual ~FVSPGitWorkerBase() override = default;

	virtual FName GetName() const override = 0;
	virtual bool Execute( FVSPGitWork& InWork ) override = 0;
	virtual bool UpdateModelStates() const override;

protected:
	TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > StatesFromGit;
};

class FVSPGitLfsWorkerBase : public FVSPGitWorkerBase
{
public:
	virtual ~FVSPGitLfsWorkerBase() override = default;
	virtual FName GetName() const override = 0;
	virtual bool Execute( FVSPGitWork& InWork ) override = 0;
	virtual bool UpdateModelStates() const override;
protected:
	TMap< FString, TSharedRef< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe > > LockStatesFromGitLfs;
};
