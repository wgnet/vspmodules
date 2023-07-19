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
#include "ISourceControlState.h"
// Module headers
#include "VSPGitSettings.h"
#include "Base/VSPGitHelpers.h"
#include "Base/VSPGitWork.h"

typedef TSharedRef< IVSPGitWorker, ESPMode::ThreadSafe > IVSPGitWorkerRef;

DECLARE_DELEGATE_RetVal( IVSPGitWorkerRef, FGetVSPGitWorker );

class FVSPGitProvider : public ISourceControlProvider
{
public:
	static FVSPGitProvider& Get();
	static FVSPGitProvider& AccessProvider();

	FVSPGitProvider();
	~FVSPGitProvider();

	/* ISourceControlProvider implementation */
public:
	virtual void Init( bool bForceConnection = true ) override;
	virtual void Close() override;
	virtual const FName& GetName() const override;
	virtual FText GetStatusText() const override;
	virtual bool IsEnabled() const override;
	virtual bool IsAvailable() const override;
	virtual bool QueryStateBranchConfig( const FString& ConfigSrc, const FString& ConfigDest ) override;
	virtual void RegisterStateBranches( const TArray< FString >& BranchNames, const FString& ContentRoot ) override;
	virtual int32 GetStateBranchIndex( const FString& BranchName ) const override;
	virtual ECommandResult::Type GetState(
		const TArray< FString >& InFiles,
		TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& OutState,
		EStateCacheUsage::Type InStateCacheUsage ) override;
	virtual TArray< FSourceControlStateRef > GetCachedStateByPredicate(
		TFunctionRef< bool( const FSourceControlStateRef& ) > Predicate ) const override;
	virtual FDelegateHandle RegisterSourceControlStateChanged_Handle(
		const FSourceControlStateChanged::FDelegate& SourceControlStateChanged ) override;
	virtual void UnregisterSourceControlStateChanged_Handle( FDelegateHandle Handle ) override;
	virtual ECommandResult::Type Execute(
		const TSharedRef< ISourceControlOperation, ESPMode::ThreadSafe >& InOperation,
		const TArray< FString >& InFiles,
		EConcurrency::Type InConcurrency = EConcurrency::Synchronous,
		const FSourceControlOperationComplete& InOperationCompleteDelegate = FSourceControlOperationComplete() ) override;
	virtual bool CanCancelOperation( const TSharedRef< ISourceControlOperation, ESPMode::ThreadSafe >& InOperation ) const override;
	virtual void CancelOperation( const TSharedRef< ISourceControlOperation, ESPMode::ThreadSafe >& InOperation ) override;
	virtual TArray< TSharedRef< ISourceControlLabel > > GetLabels( const FString& InMatchingSpec ) const override;
	virtual bool UsesLocalReadOnlyState() const override;
	virtual bool UsesChangelists() const override;
	virtual bool UsesCheckout() const override;
	virtual void Tick() override;

#if SOURCE_CONTROL_WITH_SLATE
	virtual TSharedRef< SWidget > MakeSettingsWidget() const override;
#endif // SOURCE_CONTROL_WITH_SLATE

public:
	/* Implementation for updating source control states */
	bool UpdateGitModelStates( const TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& InStatesFromGit );
	bool UpdateGitLfsModelLockStates( const TMap< FString, TSharedRef< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe > >& InLockStatesFromGitLfs );

	void UpdateRepositoryStatus( const FVSPGitWork& InWork );
	TArray< FString > GetCachedFilesList();

	TSharedRef< class FVSPGitSettings, ESPMode::ThreadSafe > GetSettings() const;
	void SaveSettings() const;

	/* Check git binaries and repositories */
	void CheckGitAndRepository();
	void CheckRepository();

	void AddWorker( const FName& InName, const FGetVSPGitWorker& InDelegate );
	void UpdateHistories( const TMap< FString, TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > > >& InHistories );
	VSPGitHelpers::FDivergence GetDivergence();
	TArray< FVSPGitWork* > GetWorksQueue() const;
	const TMap< FString, TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& GetFilesStates();

	bool IsFileLocked( const FString& InContentPath, FString& OutUserName, int32& OutLockId );
private:
	TSharedPtr< IVSPGitWorker, ESPMode::ThreadSafe > CreateWorker( const FName& InOperationName ) const;
	ECommandResult::Type ExecuteSynchronousCommand( FVSPGitWork& InWork, const FText& Task );
	ECommandResult::Type IssueWork( FVSPGitWork& InWork );

	void UpdateTimersIfNeed( const FVSPGitWork& Work );
	void ExecuteBackgroundTasksIfNeed();

private:
	bool bGitAvailable;
	bool bRepoAvailable;

	/** Current Commit full SHA1 */
	FString CommitId;
	/** Current Commit description's Summary */
	FString CommitSummary;

	/* GitPlugin settings */
	TSharedPtr< FVSPGitSettings, ESPMode::ThreadSafe > Settings;

	/* Event delegate from ISourceControl from engine */
	FSourceControlStateChanged OnSourceControlStateChanged;

	TMap< FString, TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > StateMap;
	TMap< FString, TSharedRef< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe > > LocksMap;
	TMap< FString, TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > > > Histories;

	TArray< FVSPGitWork* > WorkQueue;
	TMap< FName, FGetVSPGitWorker > WorkersMap;

	FDateTime BackgroundTimeStamp;
	FDateTime AutoFetchTimeStamp;
	// TODO: Remove this ugly code after demo
public:
	VSPGitHelpers::FDivergence Divergence;
};
