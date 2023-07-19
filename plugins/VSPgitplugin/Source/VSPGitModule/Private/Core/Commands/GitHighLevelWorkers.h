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
#include "Core.h"
#include "SourceControlOperations.h"
// Module headers
#include "Core/VSPGitRevision.h"
#include "Core/Base/VSPGitWork.h"
#include "Core/Base/IVSPGitWorker.h"

namespace GitHighLevelWorkers
{
	static const FDateTime StartMeasureTheTime( const FString& InName );
	static void EndMeasureTheTime( const FString& InName, const FDateTime& InStartTimeStamp );

	// Connect
	class FConnectWorker : public FVSPGitLfsWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Connect" ); }
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// UpdateStatus
	class FUpdateStatusWorker : public FVSPGitWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "UpdateStatus" ); }
		virtual bool Execute( FVSPGitWork& InWork ) override;
		virtual bool UpdateModelStates() const override;
	public:
		/** Map of filenames to history */
		TMap< FString, TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > > > Histories;
	};

	// MarkForAdd
	class FMarkForAddWorker : public FVSPGitWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "MarkForAdd" ); }
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Delete
	class FDeleteWorker : public FVSPGitWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Delete" ); }
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Revert
	class FRevertWorker : public FVSPGitWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Revert" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Commit
	class FCommit : public FCheckIn
	{
	public:
		virtual FName GetName() const override { return FName( "Commit" ); };
		virtual FText GetInProgressString() const override;

		FText GetHeaderText();
		void SetHeaderText( FText InHeader );

	protected:
		FText HeaderText;
	};

	class FCommitWorker : public FVSPGitWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Commit" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Fetch
	class FFetch : public FSourceControlOperationBase
	{
	public:
		virtual FName GetName() const override { return FName( "Fetch" ); };
		virtual FText GetInProgressString() const override;
	};

	class FFetchWorker : public FVSPGitWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Fetch" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Push
	class FPush : public FSourceControlOperationBase
	{
	public:
		virtual FName GetName() const override { return FName( "Push" ); };
		virtual FText GetInProgressString() const override;

		const TArray< FString >& GetArgs() const { return Args; };
		void AddArg( const FString& InArg ) { Args.Add( InArg ); };
		void AppendArgs( const TArray< FString >& InArgs ) { Args.Append( InArgs ); };
	protected:
		TArray< FString > Args;
	};

	class FPushWorker : public FVSPGitWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Push" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Pull
	class FPull : public FSourceControlOperationBase
	{
	public:
		virtual FName GetName() const override { return FName( "Pull" ); };
		virtual FText GetInProgressString() const override;

		const TArray< FString >& GetArgs() const { return Args; };
		void AddArg( const FString& InArg ) { Args.Add( InArg ); };
		void AppendArgs( const TArray< FString >& InArgs ) { Args.Append( InArgs ); };
	protected:
		TArray< FString > Args;
	};

	class FPullWorker : public FVSPGitWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Pull" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// LfsLocks
	class FLfsLocks : public FSourceControlOperationBase
	{
	public:
		virtual FName GetName() const override { return FName( "LfsLocks" ); };
		virtual FText GetInProgressString() const override;
	};

	class FLfsLocksWorker : public FVSPGitLfsWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "LfsLocks" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Lock
	class FLock : public FSourceControlOperationBase
	{
	public:
		virtual FName GetName() const override { return FName( "Lock" ); };
		virtual FText GetInProgressString() const override;
	};

	class FLockWorker : public FVSPGitLfsWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Lock" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Unlock
	class FUnlock : public FSourceControlOperationBase
	{
	public:
		virtual FName GetName() const override { return FName( "Unlock" ); };
		virtual FText GetInProgressString() const override;

		bool IsForce() const { return bForce; };
		void SetForceArg(bool bInForce) { bForce = bInForce; }
	private:
		bool bForce = false;
	};

	class FUnlockWorker : public FVSPGitLfsWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Unlock" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};

	// Background heavy task
	class FBackgroundTask : public FSourceControlOperationBase
	{
	public:
		virtual FName GetName() const override { return FName( "Background" ); };
		virtual FText GetInProgressString() const override;
	};

	class FBackgroundWorker : public FVSPGitLfsWorkerBase
	{
	public:
		virtual FName GetName() const override { return FName( "Background" ); };
		virtual bool Execute( FVSPGitWork& InWork ) override;
	};
}
