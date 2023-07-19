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
#include "GitHighLevelWorkers.h"
// Engine headers
#include "SourceControlOperations.h"
// Module headers
#include "VSPGitModule.h"
#include "GitLowLevelCommands.h"
#include "Core/Base/VSPGitHelpers.h"

#define LOCTEXT_NAMESPACE "FVSPGitModule"

const FDateTime GitHighLevelWorkers::StartMeasureTheTime( const FString& InName )
{
	switch ( GitLowLevelCommands::GitLogMode )
	{
		case GitLowLevelCommands::Verbose:
			UE_LOG( VSPGitLog, Verbose, TEXT("--------------------\n--- %s executing..."), *InName );
			break;
		case GitLowLevelCommands::Full:
			UE_LOG( VSPGitLog, Log, TEXT("--------------------\n--- %s executing..."), *InName );
			break;
		default: ;
	}

	return FDateTime::Now();
}

void GitHighLevelWorkers::EndMeasureTheTime( const FString& InName, const FDateTime& InStartTimeStamp )
{
	const FTimespan ElapsedTime = FDateTime::Now() - InStartTimeStamp;
	switch ( GitLowLevelCommands::GitLogMode )
	{
		case GitLowLevelCommands::Verbose:
			UE_LOG(
				VSPGitLog,
				Verbose,
				TEXT("--- %s execution time is %02i:%02i:%02i.%03i (total %i ms).\n-------------"),
				*InName,
				ElapsedTime.GetHours(),
				ElapsedTime.GetMinutes(),
				ElapsedTime.GetSeconds(),
				(static_cast<int>(ElapsedTime.GetTotalMilliseconds()) % 1000),
				static_cast<int>(ElapsedTime.GetTotalMilliseconds()) );
			break;
		case GitLowLevelCommands::Full:
			UE_LOG(
				VSPGitLog,
				Log,
				TEXT("--- %s execution time is %02i:%02i:%02i.%03i (total %i ms).\n-------------"),
				*InName,
				ElapsedTime.GetHours(),
				ElapsedTime.GetMinutes(),
				ElapsedTime.GetSeconds(),
				(static_cast<int>(ElapsedTime.GetTotalMilliseconds()) % 1000),
				static_cast<int>(ElapsedTime.GetTotalMilliseconds()) );
			break;
		default: ;
	}
}

//--------------------------------------------------------------
// Connect
bool GitHighLevelWorkers::FConnectWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FConnectWorker" );

	check( InWork.Operation->GetName() == GetName() );

	TSharedRef< FConnect, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FConnect >( InWork.Operation );

	if ( InWork.Settings->GetCurrentRepoSettings()->bUsingGitLfsLocking )
	{
		FSourceControlResultInfo LfsResult;
		const bool bLfsCommandSuccessful = GitLowLevelCommands::GetAllLocks( LfsResult, LockStatesFromGitLfs );
		// if (!bLfsCommandSuccessful)
		// {
		// 	UE_LOG(VSPGitLog, Error, TEXT("Command 'git lfs locks' failed. Check the availability of the remotes"));
		// }
	}
	InWork.bCommandSuccessful = GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );
	InWork.bCommandSuccessful &= GitLowLevelCommands::GetCommitInfo( InWork.CommitId, InWork.CommitSummary, Operation->ResultInfo );

	EndMeasureTheTime( "FConnectWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// UpdateStatus
bool GitHighLevelWorkers::FUpdateStatusWorker::Execute( FVSPGitWork& InWork )
{
	const GitLowLevelCommands::EGitLogMode LastMode = GitLowLevelCommands::GitLogMode;
	GitLowLevelCommands::GitLogMode = GitLowLevelCommands::EGitLogMode::Silent;

	const FDateTime StartTime = StartMeasureTheTime( "FUpdateStatusWorker" );

	check( InWork.Operation->GetName() == GetName() );

	const auto Operation = StaticCastSharedRef< FUpdateStatus >( InWork.Operation );

	InWork.bCommandSuccessful = GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );
	if ( Operation->ShouldUpdateHistory() )
	{
		TArray< FString > RelativeFilePaths = VSPGitHelpers::CheckAndMakeRelativeFilenames( InWork.Files, InWork.Settings->GetCurrentRepoSettings()->RepoRoot );
		TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > CachedStates;
		FVSPGitProvider::AccessProvider().GetState( InWork.Files, CachedStates, EStateCacheUsage::Use );
		for ( auto&& FilePath : RelativeFilePaths )
		{
			auto StatePredicate = [FilePath]( TSharedRef< ISourceControlState, ESPMode::ThreadSafe > State )
			{
				return State->GetFilename().Equals( FilePath );
			};
			TSharedRef< ISourceControlState, ESPMode::ThreadSafe >* SCState = StatesFromGit.FindByPredicate( StatePredicate );
			if ( SCState == nullptr )
				SCState = CachedStates.FindByPredicate( StatePredicate );

			if ( SCState == nullptr )
				continue;

			TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > > History;
			if ( ( *SCState )->IsConflicted() )
				// In case of a merge conflict, we first need to get the tip of the "remote branch" (MERGE_HEAD)
				GitLowLevelCommands::RunGetHistory( FilePath, true, Operation->ResultInfo, History );
			// Get the history of the file in the current branch
			InWork.bCommandSuccessful &= GitLowLevelCommands::RunGetHistory( FilePath, false, Operation->ResultInfo, History );
			Histories.Add( ( *SCState )->GetFilename(), History );
		}
	}

	EndMeasureTheTime( "FUpdateStatusWorker", StartTime );

	GitLowLevelCommands::GitLogMode = LastMode;

	return InWork.bCommandSuccessful;
}

bool GitHighLevelWorkers::FUpdateStatusWorker::UpdateModelStates() const
{
	FVSPGitProvider::AccessProvider().UpdateHistories( Histories );
	return FVSPGitProvider::AccessProvider().UpdateGitModelStates( StatesFromGit );
}

//--------------------------------------------------------------
// Mark for Add
bool GitHighLevelWorkers::FMarkForAddWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FMarkForAddWorker" );

	check( InWork.Operation->GetName() == GetName() );

	FSourceControlResultInfo ResultInfo;
	InWork.bCommandSuccessful = GitLowLevelCommands::RunGitCommand( TEXT( "add" ), TArray< FString >(), InWork.Files, ResultInfo, InWork.OutputQueue );
	// now update the status of our files
	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus( InWork.Settings->GetCurrentRepoSettings()->LockableRules, StatesFromGit, ResultInfo );
	InWork.Operation->AppendResultInfo( ResultInfo );

	EndMeasureTheTime( "FMarkForAddWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Delete
bool GitHighLevelWorkers::FDeleteWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FDeleteWorker" );

	check( InWork.Operation->GetName() == GetName() );

	FSourceControlResultInfo ResultInfo;
	InWork.bCommandSuccessful = GitLowLevelCommands::RunGitCommand( TEXT( "rm" ), TArray< FString >(), InWork.Files, ResultInfo, InWork.OutputQueue );
	// now update the status of our files
	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus( InWork.Settings->GetCurrentRepoSettings()->LockableRules, StatesFromGit, ResultInfo );
	InWork.Operation->AppendResultInfo( ResultInfo );

	EndMeasureTheTime( "FDeleteWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Revert
bool GitHighLevelWorkers::FRevertWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FRevertWorker" );

	check( InWork.Operation->GetName() == GetName() );

	// Filter files by status to use the right "revert" commands on them
	TArray< FString > MissingFiles;
	TArray< FString > AllExistingFiles;
	TArray< FString > OtherThanAddedExistingFiles;
	VSPGitHelpers::GetMissingVsExistingFiles( InWork.Files, MissingFiles, AllExistingFiles, OtherThanAddedExistingFiles );

	FSourceControlResultInfo ResultInfo;
	InWork.bCommandSuccessful = true;
	if ( MissingFiles.Num() > 0 )
		// "Added" files that have been deleted needs to be removed from source control
		InWork.bCommandSuccessful &= GitLowLevelCommands::RunGitCommand( TEXT( "rm" ), TArray< FString >(), MissingFiles, ResultInfo, InWork.OutputQueue );
	if ( AllExistingFiles.Num() > 0 )
		// reset any changes already added to the index
		InWork.bCommandSuccessful &= GitLowLevelCommands::RunGitCommand(
			TEXT( "reset" ),
			TArray< FString >(),
			AllExistingFiles,
			ResultInfo,
			InWork.OutputQueue );
	if ( OtherThanAddedExistingFiles.Num() > 0 )
		// revert any changes in working copy (this would fails if the asset was in "Added" state, since after "reset" it is now "untracked")
		InWork.bCommandSuccessful &= GitLowLevelCommands::RunGitCommand(
			TEXT( "checkout" ),
			TArray< FString >(),
			OtherThanAddedExistingFiles,
			ResultInfo,
			InWork.OutputQueue );
	// now update the status of our files
	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus( InWork.Settings->GetCurrentRepoSettings()->LockableRules, StatesFromGit, ResultInfo );
	InWork.Operation->AppendResultInfo( ResultInfo );

	EndMeasureTheTime( "FRevertWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Commit
FText GitHighLevelWorkers::FCommit::GetInProgressString() const
{
	return LOCTEXT( "SourceControl_Commit", "Commiting file(s) into Git repository..." );
}

FText GitHighLevelWorkers::FCommit::GetHeaderText()
{
	return HeaderText;
}

void GitHighLevelWorkers::FCommit::SetHeaderText( FText InHeader )
{
	HeaderText = InHeader;
}

bool GitHighLevelWorkers::FCommitWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FCommitWorker" );

	//check(InWork.Operation->GetName() == GetName());

	TSharedRef< FCheckIn, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FCheckIn >( InWork.Operation );

	FString Header = Operation->GetDescription().ToString();
	FString Description;
	if ( Operation->GetName().IsEqual( FName( "Commit" ) ) )
	{
		TSharedRef< FCommit, ESPMode::ThreadSafe > CommitOperation = StaticCastSharedRef< FCommit >( InWork.Operation );
		Header = CommitOperation->GetHeaderText().ToString();
		Description = CommitOperation->GetDescription().ToString();
	}
	InWork.bCommandSuccessful = GitLowLevelCommands::RunCommit( Header, Description, TArray< FString >(), InWork.Files, Operation->ResultInfo );
	if ( InWork.bCommandSuccessful )
	{
		Operation->SetSuccessMessage( VSPGitHelpers::ParseCommitResults( Operation->ResultInfo.InfoMessages ) );
		const FString Message = ( Operation->ResultInfo.InfoMessages.Num() > 0 ) ? Operation->ResultInfo.InfoMessages[ 0 ].ToString() : TEXT( "" );
		UE_LOG( VSPGitLog, Log, TEXT("commit successful: %s"), *Message );
	}

	FString LocalBranch;
	FString UpstreamBranch;
	VSPGitHelpers::FDivergence Div;
	GitLowLevelCommands::GetLocalBranchName( LocalBranch );
	GitLowLevelCommands::GetUpstreamBranchName( UpstreamBranch );
	GitLowLevelCommands::GetDivergence( UpstreamBranch, LocalBranch, Div.Behind, Div.Ahead );

	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->LocalBranch = LocalBranch;
	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->UpstreamBranch = UpstreamBranch.IsEmpty()
		? TEXT( "There is no upstream branch" )
		: UpstreamBranch;
	FVSPGitProvider::AccessProvider().Divergence = Div;

	// now update the status of our files
	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );
	GitLowLevelCommands::GetCommitInfo( InWork.CommitId, InWork.CommitSummary, Operation->ResultInfo );

	EndMeasureTheTime( "FCommitWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Fetch
FText GitHighLevelWorkers::FFetch::GetInProgressString() const
{
	return LOCTEXT( "SourceControl_Fetch", "Fetch branches and tags from remote origin..." );
}

bool GitHighLevelWorkers::FFetchWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FFetchWorker" );

	check( InWork.Operation->GetName() == GetName() )

	TSharedRef< FFetch, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FFetch >( InWork.Operation );

	InWork.bCommandSuccessful = GitLowLevelCommands::GitFetch(
		InWork.Settings->GetCurrentRepoSettings()->RemoteName,
		Operation->ResultInfo,
		InWork.OutputQueue );

	FString LocalBranch;
	FString UpstreamBranch;
	VSPGitHelpers::FDivergence Div;
	GitLowLevelCommands::GetLocalBranchName( LocalBranch );
	GitLowLevelCommands::GetUpstreamBranchName( UpstreamBranch );
	GitLowLevelCommands::GetDivergence( UpstreamBranch, LocalBranch, Div.Behind, Div.Ahead );

	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->LocalBranch = LocalBranch;
	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->UpstreamBranch = UpstreamBranch.IsEmpty()
		? TEXT( "There is no upstream branch" )
		: UpstreamBranch;
	FVSPGitProvider::AccessProvider().Divergence = Div;

	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );
	GitLowLevelCommands::GetCommitInfo( InWork.CommitId, InWork.CommitSummary, Operation->ResultInfo );

	EndMeasureTheTime( "FFetchWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Push
FText GitHighLevelWorkers::FPush::GetInProgressString() const
{
	return LOCTEXT( "SourceControl_Push", "Pushing local commits to remote origin..." );
}

bool GitHighLevelWorkers::FPushWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FPushWorker" );

	check( InWork.Operation->GetName() == GetName() )

	TSharedRef< FPush, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FPush >( InWork.Operation );

	// push the branch to its default remote
	// (works only if the default remote "origin" is set and does not require authentication)
	TArray< FString > Parameters;
	Parameters.Add( TEXT( "--set-upstream" ) );
	Parameters.Add( TEXT( "origin" ) );
	Parameters.Add( TEXT( "HEAD" ) );

	Operation->AppendArgs( Parameters );

	InWork.bCommandSuccessful = GitLowLevelCommands::RunGitCommand(
		TEXT( "push" ),
		Operation->GetArgs(),
		TArray< FString >(),
		Operation->ResultInfo,
		InWork.OutputQueue );

	FString LocalBranch;
	FString UpstreamBranch;
	VSPGitHelpers::FDivergence Div;
	GitLowLevelCommands::GetLocalBranchName( LocalBranch );
	GitLowLevelCommands::GetUpstreamBranchName( UpstreamBranch );
	GitLowLevelCommands::GetDivergence( UpstreamBranch, LocalBranch, Div.Behind, Div.Ahead );

	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->LocalBranch = LocalBranch;
	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->UpstreamBranch = UpstreamBranch.IsEmpty()
		? TEXT( "There is no upstream branch" )
		: UpstreamBranch;
	FVSPGitProvider::AccessProvider().Divergence = Div;

	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );
	GitLowLevelCommands::GetCommitInfo( InWork.CommitId, InWork.CommitSummary, Operation->ResultInfo );

	EndMeasureTheTime( "FPushWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Pull
FText GitHighLevelWorkers::FPull::GetInProgressString() const
{
	return LOCTEXT( "SourceControl_Pull", "Pulling new changes from remote server..." );
}

bool GitHighLevelWorkers::FPullWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FPullWorker" );

	check( InWork.Operation->GetName() == GetName() )

	TSharedRef< FPull, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FPull >( InWork.Operation );

	//Operation->AddArg(TEXT("--progress"));
	InWork.bCommandSuccessful = GitLowLevelCommands::RunGitCommand(
		TEXT( "pull" ),
		Operation->GetArgs(),
		TArray< FString >(),
		Operation->ResultInfo,
		InWork.OutputQueue );

	FString LocalBranch;
	FString UpstreamBranch;
	VSPGitHelpers::FDivergence Div;
	GitLowLevelCommands::GetLocalBranchName( LocalBranch );
	GitLowLevelCommands::GetUpstreamBranchName( UpstreamBranch );
	GitLowLevelCommands::GetDivergence( UpstreamBranch, LocalBranch, Div.Behind, Div.Ahead );

	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->LocalBranch = LocalBranch;
	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->UpstreamBranch = UpstreamBranch.IsEmpty()
		? TEXT( "There is no upstream branch" )
		: UpstreamBranch;
	FVSPGitProvider::AccessProvider().Divergence = Div;

	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );
	GitLowLevelCommands::GetCommitInfo( InWork.CommitId, InWork.CommitSummary, Operation->ResultInfo );

	EndMeasureTheTime( "FPullWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// LfsLocks
FText GitHighLevelWorkers::FLfsLocks::GetInProgressString() const
{
	return LOCTEXT( "SourceControl_LfsLocks", "Getting information about locked files..." );
}

bool GitHighLevelWorkers::FLfsLocksWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FLfsLocksWorker" );

	check( InWork.Operation->GetName() == GetName() )

	TSharedRef< FLfsLocks, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FLfsLocks >( InWork.Operation );

	InWork.bCommandSuccessful = GitLowLevelCommands::GetAllLocks( Operation->ResultInfo, LockStatesFromGitLfs );
	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );

	EndMeasureTheTime( "FLfsLocksWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Lock
FText GitHighLevelWorkers::FLock::GetInProgressString() const
{
	return LOCTEXT( "SourceControl_Lock", "Locking a file(s) in progress..." );
}

bool GitHighLevelWorkers::FLockWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FLockWorker" );

	// Note: Because the "Unlock" operation comes from the plugin, but the "Check-out" operation comes from the engine
	// check(InWork.Operation->GetName() == GetName())
	// TSharedRef<FLock, ESPMode::ThreadSafe> Operation = StaticCastSharedRef<FLock>(InWork.Operation);
	TSharedRef< FSourceControlOperationBase, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FSourceControlOperationBase >( InWork.Operation );

	InWork.bCommandSuccessful = true;
	TQueue< FString > Queue;
	TArray< FString > RelativeFilePaths = VSPGitHelpers::CheckAndMakeRelativeFilenames(
		InWork.Files,
		InWork.Settings->GetCurrentRepoSettings()->RepoRoot );
	for ( auto&& FilePath : RelativeFilePaths )
	{
		if ( !VSPGitHelpers::IsFileLockable( FilePath, InWork.Settings->GetCurrentRepoSettings()->LockableRules ) )
			continue;

		TArray< FString > OneFileArg;
		OneFileArg.Add( FilePath );
		InWork.bCommandSuccessful &= GitLowLevelCommands::RunGitCommand( TEXT( "lfs lock" ), OneFileArg, TArray< FString >(), Operation->ResultInfo, Queue );
	}

	InWork.bCommandSuccessful &= GitLowLevelCommands::GetAllLocks( Operation->ResultInfo, LockStatesFromGitLfs );
	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );

	EndMeasureTheTime( "FLockWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Unlock
FText GitHighLevelWorkers::FUnlock::GetInProgressString() const
{
	return LOCTEXT( "SourceControl_Unlock", "Unlocking a file(s) in progress..." );
}

bool GitHighLevelWorkers::FUnlockWorker::Execute( FVSPGitWork& InWork )
{
	const FDateTime StartTime = StartMeasureTheTime( "FUnlockWorker" );

	check( InWork.Operation->GetName() == GetName() )

	TSharedRef< FUnlock, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FUnlock >( InWork.Operation );

	InWork.bCommandSuccessful = true;
	TQueue< FString > Queue;
	TArray< FString > RelativeFilePaths = VSPGitHelpers::CheckAndMakeRelativeFilenames(
		InWork.Files,
		InWork.Settings->GetCurrentRepoSettings()->RepoRoot );
	for ( auto&& FilePath : RelativeFilePaths )
	{
		TArray< FString > OneFileArg;
		OneFileArg.Add( FilePath );
		if ( Operation->IsForce() )
			OneFileArg.Add( TEXT("--force") );
		InWork.bCommandSuccessful &= GitLowLevelCommands::RunGitCommand( TEXT( "lfs unlock" ), OneFileArg, TArray< FString >(), Operation->ResultInfo, Queue );
	}

	InWork.bCommandSuccessful &= GitLowLevelCommands::GetAllLocks( Operation->ResultInfo, LockStatesFromGitLfs );
	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );

	EndMeasureTheTime( "FUnlockWorker", StartTime );

	return InWork.bCommandSuccessful;
}

//--------------------------------------------------------------
// Background heavy task
FText GitHighLevelWorkers::FBackgroundTask::GetInProgressString() const
{
	return LOCTEXT( "SourceControl_Background", "Background fetch task in progress..." );
}

bool GitHighLevelWorkers::FBackgroundWorker::Execute( FVSPGitWork& InWork )
{
	const GitLowLevelCommands::EGitLogMode LastMode = GitLowLevelCommands::GitLogMode;
	GitLowLevelCommands::GitLogMode = GitLowLevelCommands::EGitLogMode::Silent;

	const FDateTime StartTime = StartMeasureTheTime( "FBackgroundWorker" );

	check( InWork.Operation->GetName() == GetName() )

	TSharedRef< FBackgroundTask, ESPMode::ThreadSafe > Operation = StaticCastSharedRef< FBackgroundTask >( InWork.Operation );

	// TODO: May be need add some more command in here
	//GitLowLevelCommands::GitFetch(InWork.Settings->GetGitBinPath(), InWork.Settings->GetCurrentRepoSettings()->RepoRoot, InWork.Settings->GetCurrentRepoSettings()->RemoteName, InWork.InfoMessages, InWork.ErrorMessages, InWork.OutputQueue);

	InWork.bCommandSuccessful = true;

	FString LocalBranch;
	FString UpstreamBranch;
	VSPGitHelpers::FDivergence Div;
	GitLowLevelCommands::GetLocalBranchName( LocalBranch );
	GitLowLevelCommands::GetUpstreamBranchName( UpstreamBranch );
	GitLowLevelCommands::GetDivergence( UpstreamBranch, LocalBranch, Div.Behind, Div.Ahead );

	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->LocalBranch = LocalBranch;
	FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->UpstreamBranch = UpstreamBranch.IsEmpty()
		? TEXT( "There is no upstream branch" )
		: UpstreamBranch;
	FVSPGitProvider::AccessProvider().Divergence = Div;

	FSourceControlResultInfo LfsResult;
	InWork.bCommandSuccessful &= GitLowLevelCommands::GetAllLocks( LfsResult, LockStatesFromGitLfs );
	InWork.bCommandSuccessful &= GitLowLevelCommands::UpdateStatus(
		InWork.Settings->GetCurrentRepoSettings()->LockableRules,
		StatesFromGit,
		Operation->ResultInfo );

	if ( InWork.bCommandSuccessful )
		switch ( GitLowLevelCommands::GitLogMode )
		{
			case GitLowLevelCommands::Full:
				UE_LOG( VSPGitLog, Log, TEXT("The background task has completed successfully.") );
				break;
			default: ;
		}
	else
		switch ( GitLowLevelCommands::GitLogMode )
		{
			case GitLowLevelCommands::Full:
				UE_LOG( VSPGitLog, Warning, TEXT("The background task has failed.") );
				break;
			default: ;
		}

	EndMeasureTheTime( "FBackgroundWorker", StartTime );

	GitLowLevelCommands::GitLogMode = LastMode;

	return InWork.bCommandSuccessful;
}

#undef LOCTEXT_NAMESPACE
