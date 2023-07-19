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
#include "VSPGitProvider.h"
// Engine heads
#include "ScopedSourceControlProgress.h"
#include "SourceControlHelpers.h"
// Module headers
#include "VSPGitState.h"
#include "VSPGitModule.h"
#include "UI/Widgets/SVSPGitRepoSettings.h"
#include "Base/IVSPGitWorker.h"
#include "Commands/GitHighLevelWorkers.h"
#include "Commands/GitLowLevelCommands.h"
#include "Logging/MessageLog.h"
#include "UI/VSPGitSlateLayout.h"

static FName ProviderName( "VSPGit" );

#define LOCTEXT_NAMESPACE "FVSPGitModule"

FVSPGitProvider& FVSPGitProvider::Get()
{
	FVSPGitModule& VSPGitModule = FModuleManager::LoadModuleChecked< FVSPGitModule >( FName( "VSPGitModule" ) );
	return VSPGitModule.GetProvider();
}

FVSPGitProvider& FVSPGitProvider::AccessProvider()
{
	FVSPGitModule& VSPGitModule = FModuleManager::LoadModuleChecked< FVSPGitModule >( FName( "VSPGitModule" ) );
	return VSPGitModule.AccessProvider();
}

FVSPGitProvider::FVSPGitProvider()
	: bGitAvailable( false ), bRepoAvailable( false )
{
	Settings = MakeShared< class FVSPGitSettings, ESPMode::ThreadSafe >();

	Settings->LoadSettings();

	// TODO: Remove this section after added choice repositories logic
#if IS_PROGRAM
	TSharedPtr<FRepoSettings> GameRepoSettings = MakeShareable(new FRepoSettings());
	GameRepoSettings->RepoRoot = TEXT("e:\\Source\\game\\");
	Settings->SetCurrentRepoSettings(GameRepoSettings);
	TSharedPtr<FRepoSettings> RepoSettings = MakeShareable(new FRepoSettings());
	RepoSettings->RepoRoot = TEXT("e:\\Source\\gitpluginrun\\");
	Settings->SetCurrentRepoSettings(RepoSettings);
	Settings->SaveSettings();
#endif // IS_PROGRAM
}

FVSPGitProvider::~FVSPGitProvider()
{
	Settings = nullptr;
}

void FVSPGitProvider::Init( bool bForceConnection )
{
	if ( GitLowLevelCommands::GitLogMode == GitLowLevelCommands::Full )
	{
		UE_LOG( VSPGitLog, Log, TEXT("VSPGitProvider => Init") );
	}
	if ( !bGitAvailable )
		CheckGitAndRepository();
}

void FVSPGitProvider::Close()
{
	FVSPGitSlateLayout::Get().HideUiExtenders();

	bGitAvailable = false;

	Histories.Empty();
	StateMap.Empty();
	LocksMap.Empty();
	UE_LOG( VSPGitLog, Log, TEXT("VSPGitProvider => Close") );
}

const FName& FVSPGitProvider::GetName() const
{
	return ProviderName;
}

FText FVSPGitProvider::GetStatusText() const
{
	FFormatNamedArguments Args;
	TSharedPtr< FRepoSettings, ESPMode::ThreadSafe > RepoSettings = Settings->GetCurrentRepoSettings();
	Args.Add( TEXT( "RepositoryName" ), FText::FromString( RepoSettings->RepoRoot ) );
	Args.Add( TEXT( "RemoteUrl" ), FText::FromString( RepoSettings->RemoteUrl ) );
	Args.Add( TEXT( "UserName" ), FText::FromString( RepoSettings->UserName ) );
	Args.Add( TEXT( "UserEmail" ), FText::FromString( RepoSettings->UserEmail ) );
	Args.Add( TEXT( "BranchName" ), FText::FromString( RepoSettings->UpstreamBranch ) );
	Args.Add( TEXT( "CommitId" ), FText::FromString( CommitId.Left( 8 ) ) );
	Args.Add( TEXT( "CommitSummary" ), FText::FromString( CommitSummary ) );

	return FText::Format(
		NSLOCTEXT(
			"Status",
			"Provider: Git\nEnabledLabel",
			"Local repository: {RepositoryName}\nRemote origin: {RemoteUrl}\nUser: {UserName}\nE-mail: {UserEmail}\n[{BranchName}"
			/*" {CommitId}] {CommitSummary}"*/ ),
		Args );
}

bool FVSPGitProvider::IsEnabled() const
{
	return bRepoAvailable && bGitAvailable;
}

bool FVSPGitProvider::IsAvailable() const
{
	return bRepoAvailable && bGitAvailable;
}

bool FVSPGitProvider::QueryStateBranchConfig( const FString& ConfigSrc, const FString& ConfigDest )
{
	// TODO: check how and implement
	UE_LOG( VSPGitLog, Error, TEXT("VSPGitProvider::QueryStateBranchConfig(const FString& ConfigSrc, const FString& ConfigDest) => not implemented yet") );
	return false;
}

void FVSPGitProvider::RegisterStateBranches( const TArray< FString >& BranchNames, const FString& ContentRoot )
{
	// TODO:
	UE_LOG(
		VSPGitLog,
		Error,
		TEXT("VSPGitProvider::RegisterStateBranches(const TArray<FString>& BranchNames, const FString& ContentRoot) => not implemented yet") );
}

int32 FVSPGitProvider::GetStateBranchIndex( const FString& BranchName ) const
{
	// TODO:
	UE_LOG( VSPGitLog, Error, TEXT("VSPGitProvider::GetStateBranchIndex(const FString& BranchName) => not implemented yet") );
	return INDEX_NONE;
}

ECommandResult::Type FVSPGitProvider::GetState(
	const TArray< FString >& InFiles,
	TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& OutState,
	EStateCacheUsage::Type InStateCacheUsage )
{
	TArray< FString > Files = VSPGitHelpers::CheckAndMakeRelativeFilenames( InFiles, Settings->GetCurrentRepoSettings()->RepoRoot );
	for ( auto&& FilePath : Files )
	{
		TSharedRef< ISourceControlState, ESPMode::ThreadSafe >* Found = StateMap.Find( FilePath );
		if ( Found != nullptr )
		{
			OutState.Add( *Found );
		}
		else
		{
			TSharedRef< FVSPGitState, ESPMode::ThreadSafe > FakeState = MakeShared< FVSPGitState, ESPMode::ThreadSafe >( FilePath, EGitState::Unchanged );
			TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > >* History = Histories.Find( FilePath );
			FakeState->bUsingGitLfsLocking = VSPGitHelpers::IsFileLockable( FilePath, Settings->GetCurrentRepoSettings()->LockableRules );
			if ( History != nullptr )
				FakeState->History = *History;
			OutState.Add( FakeState );
		}
	}
	return OutState.Num() ? ECommandResult::Succeeded : ECommandResult::Failed;
}

TArray< FSourceControlStateRef > FVSPGitProvider::GetCachedStateByPredicate(
	TFunctionRef< bool( const FSourceControlStateRef& ) > Predicate ) const
{
	TArray< FSourceControlStateRef > Result;

	for ( auto&& StatePair : StateMap )
		if ( Predicate( StatePair.Value ) )
			Result.Add( StatePair.Value );

	return Result;
}

FDelegateHandle FVSPGitProvider::RegisterSourceControlStateChanged_Handle(
	const FSourceControlStateChanged::FDelegate& SourceControlStateChanged )
{
	return OnSourceControlStateChanged.Add( SourceControlStateChanged );
}

void FVSPGitProvider::UnregisterSourceControlStateChanged_Handle( FDelegateHandle Handle )
{
	OnSourceControlStateChanged.Remove( Handle );
}

ECommandResult::Type FVSPGitProvider::Execute(
	const TSharedRef< ISourceControlOperation, ESPMode::ThreadSafe >& InOperation,
	const TArray< FString >& InFiles,
	EConcurrency::Type InConcurrency,
	const FSourceControlOperationComplete& InOperationCompleteDelegate )
{
	if ( GitLowLevelCommands::GitLogMode == GitLowLevelCommands::Full )
	{
		UE_LOG(
			VSPGitLog,
			Log,
			TEXT("VSPGitProvider: Execute `%s` Concurrency=%hs"),
			*InOperation->GetName().ToString(),
			(InConcurrency == EConcurrency::Asynchronous? "Asynchronous" : "Synchronous") );
	}
	if ( !IsEnabled() && !( InOperation->GetName() == "Connect" ) )
	{
		// ReSharper disable once CppExpressionWithoutSideEffects
		InOperationCompleteDelegate.ExecuteIfBound( InOperation, ECommandResult::Failed );
		return ECommandResult::Failed;
	}

	// Sometimes we get paths to engine assets. They should be ignored.
	// "E:/Source/unreal_engine/VSP-4.25.4-22150bb6/Engine/Content/"
	//const TArray<FString> AbsFiles = SourceControlHelpers::AbsoluteFilenames(InFiles);
	const TArray< FString > AbsFiles = VSPGitHelpers::AbsoluteAndFilteringFilenames( InFiles, Settings->GetCurrentRepoSettings()->RepoRoot );

	const auto Worker = CreateWorker( InOperation->GetName() );
	if ( !Worker.IsValid() )
	{
		FFormatNamedArguments Args;
		Args.Add( TEXT( "OperationName" ), FText::FromName( InOperation->GetName() ) );
		Args.Add( TEXT( "ProviderName" ), FText::FromName( GetName() ) );
		const FText Message = FText::Format(
			FText::FromString( "Operation '{OperationName}' not supported by source control provider '{ProviderName}' " ),
			Args );

		FMessageLog( "SourceControl" ).Error( Message );
		InOperation->AddErrorMessge( Message );

		// ReSharper disable once CppExpressionWithoutSideEffects
		InOperationCompleteDelegate.ExecuteIfBound( InOperation, ECommandResult::Failed );
		return ECommandResult::Failed;
	}

	FVSPGitWork* Work = new FVSPGitWork( InOperation, Worker.ToSharedRef(), GetSettings() );
	Work->Files = AbsFiles;
	Work->OperationCompleteDelegate = InOperationCompleteDelegate;

	if ( InConcurrency == EConcurrency::Synchronous )
	{
		Work->bAutoDelete = false;
		return ExecuteSynchronousCommand( *Work, InOperation->GetInProgressString() );
	}
	Work->bAutoDelete = true;
	return IssueWork( *Work );
}

bool FVSPGitProvider::CanCancelOperation(
	const TSharedRef< ISourceControlOperation, ESPMode::ThreadSafe >& InOperation ) const
{
	return false;
}

void FVSPGitProvider::CancelOperation( const TSharedRef< ISourceControlOperation, ESPMode::ThreadSafe >& InOperation )
{
}

TArray< TSharedRef< ISourceControlLabel > > FVSPGitProvider::GetLabels( const FString& InMatchingSpec ) const
{
	// TODO: Is it need?
	TArray< TSharedRef< ISourceControlLabel > > Tags;
	return Tags;
}

bool FVSPGitProvider::UsesLocalReadOnlyState() const
{
	return Settings->GetCurrentRepoSettings()->bUsingGitLfsLocking;
}

bool FVSPGitProvider::UsesChangelists() const
{
	// TODO: Check, what it does
	return false;
}

bool FVSPGitProvider::UsesCheckout() const
{
	return Settings->GetCurrentRepoSettings()->bUsingGitLfsLocking;
}

void FVSPGitProvider::Tick()
{
	auto bStatesUpdated = false;
	for ( auto i = 0; i < WorkQueue.Num(); ++i )
	{
		FVSPGitWork& Work = *WorkQueue[ i ];
		if ( Work.bExecuteProcessed )
		{
			// Check and update timer for background task
			UpdateTimersIfNeed( Work );

			// Remove work from the queue
			WorkQueue.RemoveAt( i );

			//// Update repository status on UpdateStatus operations
			UpdateRepositoryStatus( Work );

			//// let command update the states of any files
			bStatesUpdated |= Work.Worker->UpdateModelStates();
			//bStatesUpdated = UpdateFilesStatus(Work.Worker->States);

			//// dump any messages to output log
			//OutputCommandMessages(Command);

			// run the completion delegate callback if we have one bound
			Work.ReturnResults();

			// works that are left in the array during a tick need to be deleted
			if ( Work.bAutoDelete )
				// Only delete work that are not running 'synchronously'
				delete &Work;
			// only do one work per tick loop, as we dont want concurrent modification
			// of the work queue (which can happen in the completion delegate)
			break;
		}
	}

	if ( bStatesUpdated )
		OnSourceControlStateChanged.Broadcast();
	ExecuteBackgroundTasksIfNeed();
}

#if SOURCE_CONTROL_WITH_SLATE
TSharedRef< SWidget > FVSPGitProvider::MakeSettingsWidget() const
{
	return SNew( SVSPGitRepoSettings );
}
#endif // SOURCE_CONTROL_WITH_SLATE

bool FVSPGitProvider::UpdateGitModelStates( const TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& InStatesFromGit )
{
	const FDateTime Now = FDateTime::Now();
	StateMap.Empty();
	// Step 1: Fill map with modified states
	for ( TSharedRef< ISourceControlState, ESPMode::ThreadSafe > SourceControlState : InStatesFromGit )
	{
		TSharedRef< FVSPGitState, ESPMode::ThreadSafe > StatesFromGit = StaticCastSharedRef< FVSPGitState >( SourceControlState );
		//StatesFromGit->TimeStamp = Now;
		StateMap.Add( StatesFromGit->FileName, StatesFromGit );
	}

	// Step 2: Added locked or added marks for exist
	for ( auto&& LocksPair : LocksMap )
	{
		TSharedRef< ISourceControlState, ESPMode::ThreadSafe >* Founded = StateMap.Find( LocksPair.Key );
		if ( Founded != nullptr )
		{
			TSharedRef< FVSPGitState, ESPMode::ThreadSafe > StatesFromGit = StaticCastSharedRef< FVSPGitState >( *Founded );
			StatesFromGit->bIsLocked = true;
			StatesFromGit->LockUser = LocksPair.Value->LockUser;
			StatesFromGit->LockId = LocksPair.Value->Id;
		}
		else
		{
			TSharedRef< FVSPGitState, ESPMode::ThreadSafe > NewState = MakeShared< FVSPGitState, ESPMode::ThreadSafe >( LocksPair.Key, EGitState::Unchanged );
			NewState->bUsingGitLfsLocking = VSPGitHelpers::IsFileLockable( LocksPair.Key, Settings->GetCurrentRepoSettings()->LockableRules );
			NewState->bIsLocked = true;
			NewState->LockUser = LocksPair.Value->LockUser;
			NewState->LockId = LocksPair.Value->Id;
			NewState->TimeStamp = Now;
			StateMap.Add( LocksPair.Key, NewState );
		}
	}

	// Step 3: Update history from cash
	for ( auto&& StatePair : StateMap )
	{
		TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > >* History = Histories.Find( StatePair.Key );
		if ( History != nullptr )
		{
			TSharedRef< FVSPGitState, ESPMode::ThreadSafe > StatesFromGit = StaticCastSharedRef< FVSPGitState >( StatePair.Value );
			StatesFromGit->History = *History;
		}
	}

	return true;
}

bool FVSPGitProvider::UpdateGitLfsModelLockStates(
	const TMap< FString, TSharedRef< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe > >& InLockStatesFromGitLfs )
{
	LocksMap = InLockStatesFromGitLfs;
	return true;
}

void FVSPGitProvider::UpdateRepositoryStatus( const FVSPGitWork& InWork )
{
	// For all operations running UpdateStatus, get Commit information:
	if ( !InWork.CommitId.IsEmpty() )
	{
		CommitId = InWork.CommitId;
		CommitSummary = InWork.CommitSummary;
	}
}

TArray< FString > FVSPGitProvider::GetCachedFilesList()
{
	TArray< FString > Files;
	for ( const auto& FilesStatePair : StateMap )
		Files.Add( FilesStatePair.Key );
	return Files;
}

TSharedRef< FVSPGitSettings, ESPMode::ThreadSafe > FVSPGitProvider::GetSettings() const
{
	return Settings.ToSharedRef();
}

void FVSPGitProvider::SaveSettings() const
{
	Settings->SaveSettings();
}

void FVSPGitProvider::CheckGitAndRepository()
{
	const FString GitBinPath = Settings->GetGitBinPath();

	if ( !GitBinPath.IsEmpty() )
	{
		UE_LOG( VSPGitLog, Log, TEXT("Using git from: '%s'"), *GitBinPath );
		GitLowLevelCommands::GitBinPath = GitBinPath;
		bGitAvailable = GitLowLevelCommands::CheckGit( &Settings->GitVersion );
		if ( bGitAvailable )
		{
			FVSPGitSlateLayout::Get().ShowUiExtenders();
			Settings->bGitLfsAvailable = GitLowLevelCommands::CheckLfs( Settings->GitLfsVersion );

			CheckRepository();
		}
	}
}

void FVSPGitProvider::CheckRepository()
{
	const auto ProjectDir = FPaths::ConvertRelativePathToFull( FPaths::ProjectDir() );
	GitLowLevelCommands::RepoRoot = ProjectDir;
	bRepoAvailable = GitLowLevelCommands::FindRepoRoot( ProjectDir, Settings->GetCurrentRepoSettings()->RepoRoot );
	if ( bRepoAvailable )
	{
		bRepoAvailable = GitLowLevelCommands::GetLocalBranchName( Settings->GetCurrentRepoSettings()->LocalBranch );
		if ( bRepoAvailable )
		{
			GitLowLevelCommands::GetUpstreamBranchName( Settings->GetCurrentRepoSettings()->UpstreamBranch );
			GitLowLevelCommands::GetRemoteUrl( Settings->GetCurrentRepoSettings()->RemoteName, Settings->GetCurrentRepoSettings()->RemoteUrl );

			if ( Settings->bGitLfsAvailable )
				Settings->GetCurrentRepoSettings()->bUsingGitLfsLocking = VSPGitHelpers::GetLockableRuleList(
					Settings->GetCurrentRepoSettings()->RepoRoot,
					Settings->GetCurrentRepoSettings()->LockableRules );
		}
		else
		{
			UE_LOG( VSPGitLog, Error, TEXT("'%s' invalide git repository state"), *Settings->GetCurrentRepoSettings()->RepoRoot );
		}
	}
	else
	{
		UE_LOG( VSPGitLog, Warning, TEXT("'%s is not part of git repository"), *ProjectDir );
	}
	GitLowLevelCommands::GetUserData( Settings->GetCurrentRepoSettings()->UserName, Settings->GetCurrentRepoSettings()->UserEmail );
}

void FVSPGitProvider::AddWorker( const FName& InName, const FGetVSPGitWorker& InDelegate )
{
	WorkersMap.Add( InName, InDelegate );
}

void FVSPGitProvider::UpdateHistories(
	const TMap< FString, TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > > >& InHistories )
{
	for ( auto&& History : InHistories )
	{
		TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > >* FileHistory = Histories.Find( History.Key );
		if ( FileHistory != nullptr )
			Histories.Remove( History.Key );
		Histories.Add( History.Key, History.Value );
	}
}

VSPGitHelpers::FDivergence FVSPGitProvider::GetDivergence()
{
	return Divergence;
}

TArray< FVSPGitWork* > FVSPGitProvider::GetWorksQueue() const
{
	return WorkQueue;
}

const TMap< FString, TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& FVSPGitProvider::GetFilesStates()
{
	return StateMap;
}

bool FVSPGitProvider::IsFileLocked( const FString& InContentPath, FString& OutUserName, int32& OutLockId )
{
	TSharedRef< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe >* LockInfo = LocksMap.Find( InContentPath );
	if ( LockInfo == nullptr )
		return false;

	OutUserName = LockInfo->Get().LockUser;
	OutLockId = LockInfo->Get().Id;
	return true;
}

TSharedPtr< IVSPGitWorker, ESPMode::ThreadSafe > FVSPGitProvider::CreateWorker( const FName& InOperationName ) const
{
	const FGetVSPGitWorker* Operation = WorkersMap.Find( InOperationName );
	if ( Operation != nullptr )
		return Operation->Execute();
	return nullptr;
}

ECommandResult::Type FVSPGitProvider::ExecuteSynchronousCommand( FVSPGitWork& InWork, const FText& Task )
{
	auto Res = ECommandResult::Failed;

	{
		FScopedSlowTask Progress(0, Task);
		Progress.MakeDialog();

		IssueWork( InWork );

		while ( !InWork.bExecuteProcessed )
		{
			Tick();
			FPlatformProcess::Sleep( 0.01f );
		}
		Tick(); // safety tick

		if ( InWork.bCommandSuccessful )
			Res = ECommandResult::Succeeded;
	}

	check( !InWork.bAutoDelete );

	if ( WorkQueue.Contains( &InWork ) )
		WorkQueue.Remove( &InWork );
	delete &InWork;

	return Res;
}

ECommandResult::Type FVSPGitProvider::IssueWork( FVSPGitWork& InWork )
{
	if ( GThreadPool != nullptr )
	{
		GThreadPool->AddQueuedWork( &InWork );
		WorkQueue.Add( &InWork );
		return ECommandResult::Succeeded;
	}

	const FText Msg = FText::FromString( "There are no threads available to process the source control operation." );

	FMessageLog( "SourceControl" ).Error( Msg );
	InWork.bCommandSuccessful = false;
	InWork.Operation->AddErrorMessge( Msg );

	return InWork.ReturnResults();
}

void FVSPGitProvider::UpdateTimersIfNeed( const FVSPGitWork& Work )
{
	const FName WorkerName = Work.Worker->GetName();

	if ( WorkerName.IsEqual( FName( "LfsLocks" ) ) ||
		WorkerName.IsEqual( FName( "Lock" ) ) ||
		WorkerName.IsEqual( FName( "Unlock" ) ) ||
		WorkerName.IsEqual( FName( "CheckOut" ) ) )
		BackgroundTimeStamp = FDateTime::Now();

	if ( WorkerName.IsEqual( FName( "Fetch" ) ) )
		AutoFetchTimeStamp = FDateTime::Now();
}

void FVSPGitProvider::ExecuteBackgroundTasksIfNeed()
{
	const FTimespan ElapsedTime = FDateTime::Now() - BackgroundTimeStamp;
	if ( ElapsedTime.GetTotalMilliseconds() >= Settings->BackgroundUpdateTimeMs )
	{
		BackgroundTimeStamp = FDateTime::Now();
		TSharedRef< GitHighLevelWorkers::FBackgroundTask, ESPMode::ThreadSafe > BackgroundTask =
			GitHighLevelWorkers::FBackgroundTask::Create< GitHighLevelWorkers::FBackgroundTask >();
		Execute( BackgroundTask, TArray< FString >(), EConcurrency::Asynchronous );
	}

	const FTimespan FetchElapsedTime = FDateTime::Now() - AutoFetchTimeStamp;
	if ( FetchElapsedTime.GetTotalMilliseconds() >= Settings->AutoFetchUpdateTimeMs )
	{
		AutoFetchTimeStamp = FDateTime::Now();
		TSharedRef< GitHighLevelWorkers::FFetch, ESPMode::ThreadSafe > FetchTask =
			GitHighLevelWorkers::FFetch::Create< GitHighLevelWorkers::FFetch >();
		Execute( FetchTask, TArray< FString >(), EConcurrency::Asynchronous );
	}
}

#undef LOCTEXT_NAMESPACE
