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
#include "GitLowLevelCommands.h"

// Module includes
#include "Misc/DefaultValueHelper.h"
#include "VSPGitModule.h"
#include "Core/VSPGitRevision.h"
#include "Core/VSPGitState.h"
#include "Core/Base/VSPGitHelpers.h"

namespace GitLowLevelCommandsLocal
{
	void ParseStatusResult(
		const FSourceControlResultInfo& InResults,
		TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& OutStates );

	FString LogStatusToString( TCHAR InStatus );

	void ParseLogResults( const FSourceControlResultInfo& InResults, TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > >& OutHistory );

	bool ParseFileState(
		const FString& StatusResult,
		EGitState::Type& OutFileGitState,
		FString& OutFileName );

	void RunGetConflictStatus(
		const FString& InFile,
		TSharedRef< FVSPGitState, ESPMode::ThreadSafe >& FileState );
}

bool GitLowLevelCommands::RunGitCommandLow(
	const FString& InCommand,
	const TArray< FString >& InParams,
	const TArray< FString >& InFiles,
	FSourceControlResultInfo& OutResults,
	TQueue< FString >& OutOutputQueue )
{
	FDateTime StartTime = FDateTime::Now();

	FString ExecCommand = GitBinPath;
	FString ArgsLineCommand = TEXT( "" );

	if ( !RepoRoot.IsEmpty() )
	{
		ArgsLineCommand += FString::Printf( TEXT( "-C \"%s\" " ), *RepoRoot );
	}

	ArgsLineCommand += InCommand;

	for ( const auto& Parameter : InParams )
	{
		ArgsLineCommand += TEXT( " " );
		ArgsLineCommand += Parameter;
	}

	if ( InFiles.Num() > 0 )
	{
		ArgsLineCommand += TEXT( " -- " );
		for ( const auto& File : InFiles )
		{
			ArgsLineCommand += TEXT( " " );
			ArgsLineCommand += File;
		}
	}

	switch ( GitLogMode )
	{
		case Verbose:
			UE_LOG( VSPGitLog, Verbose, TEXT("--- 'git %s'"), *ArgsLineCommand );
			break;
		case Full:
			UE_LOG( VSPGitLog, Log, TEXT("--- 'git %s'"), *ArgsLineCommand );
			break;
	}

	TArray< FString > OutResultsStrings;
	int32 RetCode = 0;

	const bool bLaunchDetached = true;
	const bool bLaunchHidden = false;
	const bool bLaunchReallyHidden = bLaunchHidden;

	void* ReadPipe = nullptr;
	void* WritePipe = nullptr;

	verify( FPlatformProcess::CreatePipe(ReadPipe, WritePipe) );

	uint32 ProcessId;
	FProcHandle Handle = FPlatformProcess::CreateProc(
		*ExecCommand,
		*ArgsLineCommand,
		bLaunchDetached,
		bLaunchHidden,
		bLaunchReallyHidden,
		&ProcessId,
		0,
		nullptr,
		WritePipe );
	if ( !Handle.IsValid() || !FPlatformProcess::IsProcRunning( Handle ) || !FPlatformProcess::IsApplicationRunning( ProcessId ) )
	{
		UE_LOG( VSPGitLog, Warning, TEXT("Cant create proccess for '%s'."), *GitBinPath );
		FPlatformProcess::ClosePipe( ReadPipe, WritePipe );
		return false;
	}

	FString CurrentLine;
	FString FullOutput;
	FString LatestOutput;
	do
	{
		LatestOutput = FPlatformProcess::ReadPipe( ReadPipe );
		if ( !LatestOutput.IsEmpty() )
		{
			FullOutput += LatestOutput;
			CurrentLine += LatestOutput;
			VSPGitHelpers::ParseCumulativeStream( CurrentLine, OutOutputQueue );
		}
	}
	while ( FPlatformProcess::IsProcRunning( Handle ) );
	LatestOutput = FPlatformProcess::ReadPipe( ReadPipe );
	FullOutput += LatestOutput;
	CurrentLine += LatestOutput;
	VSPGitHelpers::ParseCumulativeStream( CurrentLine, OutOutputQueue );
	if ( !CurrentLine.IsEmpty() )
	{
		CurrentLine = CurrentLine.TrimEnd();
		OutOutputQueue.Enqueue( CurrentLine );
	}

	bool bGotReturnCode = FPlatformProcess::GetProcReturnCode( Handle, &RetCode );
	FPlatformProcess::CloseProc( Handle );
	FPlatformProcess::ClosePipe( ReadPipe, WritePipe );

	FullOutput.ParseIntoArrayLines( OutResultsStrings );

	for ( FString& ResultsString : OutResultsStrings )
		OutResults.InfoMessages.Add( FText::FromString( ResultsString ) );

	if ( RetCode == 0 )
		switch ( GitLogMode )
		{
			case Verbose:
				UE_LOG( VSPGitLog, Verbose, TEXT("-- Return code: %d; std::out result:\n%s"), RetCode, *FullOutput );
				break;
			case Full:
				UE_LOG( VSPGitLog, Log, TEXT("-- Return code: %d; std::out result:\n%s"), RetCode, *FullOutput );
				break;
		}
	else
	{
		for ( FString& ResultsString : OutResultsStrings )
			OutResults.ErrorMessages.Add( FText::FromString( ResultsString ) );

		switch ( GitLogMode )
		{
			case Verbose:
				UE_LOG( VSPGitLog, Warning, TEXT("-- Return code: %d; std::err result:\n%s"), RetCode, *FullOutput );
				break;
			case Full:
				UE_LOG( VSPGitLog, Warning, TEXT("-- Return code: %d; std::err result:\n%s"), RetCode, *FullOutput );
				break;
		}
	}

	const FTimespan ElapsedTime = FDateTime::Now() - StartTime;
	switch ( GitLogMode )
	{
		case Verbose:
			UE_LOG(
				VSPGitLog,
				Verbose,
				TEXT("--- Command 'git %s' execution time is %02i:%02i:%02i.%03i (total %i ms)."),
				*ArgsLineCommand,
				ElapsedTime.GetHours(),
				ElapsedTime.GetMinutes(),
				ElapsedTime.GetSeconds(),
				(static_cast<int>(ElapsedTime.GetTotalMilliseconds()) % 1000),
				static_cast<int>(ElapsedTime.GetTotalMilliseconds()) );
			break;
		case Short:
			if ( RetCode == 0 )
			{
				UE_LOG(
					VSPGitLog,
					Log,
					TEXT("--- 'git %s'; Return code: %d; execution time is %02i:%02i:%02i.%03i (total %i ms)."),
					*ArgsLineCommand,
					RetCode,
					ElapsedTime.GetHours(),
					ElapsedTime.GetMinutes(),
					ElapsedTime.GetSeconds(),
					(static_cast<int>(ElapsedTime.GetTotalMilliseconds()) % 1000),
					static_cast<int>(ElapsedTime.GetTotalMilliseconds()) );
			}
			else
			{
				UE_LOG(
					VSPGitLog,
					Warning,
					TEXT("--- 'git %s'; Return code: %d; execution time is %02i:%02i:%02i.%03i (total %i ms)."),
					*ArgsLineCommand,
					RetCode,
					ElapsedTime.GetHours(),
					ElapsedTime.GetMinutes(),
					ElapsedTime.GetSeconds(),
					(static_cast<int>(ElapsedTime.GetTotalMilliseconds()) % 1000),
					static_cast<int>(ElapsedTime.GetTotalMilliseconds()) );
			}
			break;
		case Full:
			UE_LOG(
				VSPGitLog,
				Log,
				TEXT("--- Command 'git %s' execution time is %02i:%02i:%02i.%03i (total %i ms)."),
				*ArgsLineCommand,
				ElapsedTime.GetHours(),
				ElapsedTime.GetMinutes(),
				ElapsedTime.GetSeconds(),
				(static_cast<int>(ElapsedTime.GetTotalMilliseconds()) % 1000),
				static_cast<int>(ElapsedTime.GetTotalMilliseconds()) );
			break;
		case ShortWithErrorOutput:
			if ( RetCode == 0 )
			{
				UE_LOG(
					VSPGitLog,
					Log,
					TEXT("--- 'git %s'; Return code: %d; execution time is %02i:%02i:%02i.%03i (total %i ms)."),
					*ArgsLineCommand,
					RetCode,
					ElapsedTime.GetHours(),
					ElapsedTime.GetMinutes(),
					ElapsedTime.GetSeconds(),
					(static_cast<int>(ElapsedTime.GetTotalMilliseconds()) % 1000),
					static_cast<int>(ElapsedTime.GetTotalMilliseconds()) );
			}
			else
			{
				UE_LOG(
					VSPGitLog,
					Warning,
					TEXT("--- 'git %s'; Return code: %d; execution time is %02i:%02i:%02i.%03i (total %i ms). Output:\n%s"),
					*ArgsLineCommand,
					RetCode,
					ElapsedTime.GetHours(),
					ElapsedTime.GetMinutes(),
					ElapsedTime.GetSeconds(),
					(static_cast<int>(ElapsedTime.GetTotalMilliseconds()) % 1000),
					static_cast<int>(ElapsedTime.GetTotalMilliseconds()),
					*FullOutput );
			}
			break;
		default: ;
	}


	return RetCode == 0;
}

bool GitLowLevelCommands::RunGitCommand(
	const FString& InCommand,
	const TArray< FString >& InParams,
	const TArray< FString >& InFiles,
	FSourceControlResultInfo& OutResults,
	TQueue< FString >& OutOutputQueue )
{
	const bool bSuccessful = RunGitCommandLow( InCommand, InParams, InFiles, OutResults, OutOutputQueue );

	return bSuccessful;
}

bool GitLowLevelCommands::CheckGit( VSPGitHelpers::FGitVersion* OutVersion )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;

	const auto bResult = RunGitCommand( TEXT( "version" ), TArray< FString >(), TArray< FString >(), Results, Queue );

	if ( bResult && Results.InfoMessages.Num() > 0 )
		if ( Results.InfoMessages[ 0 ].ToString().Contains( TEXT( "git" ) ) )
		{
			if ( OutVersion )
			{
				ParseGitVersion( Results.InfoMessages[ 0 ].ToString(), OutVersion );
				FindGitLfsCapabilities( OutVersion );
			}
			return true;
		}

	return false;
}

bool GitLowLevelCommands::CheckLfs( FString& OutGitLfsVersion )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;

	const auto bResult = RunGitCommand( TEXT( "lfs version" ), TArray< FString >(), TArray< FString >(), Results, Queue );

	if ( bResult && Results.InfoMessages.Num() > 0 )
	{
		OutGitLfsVersion = Results.InfoMessages[ 0 ].ToString();
		return true;
	}

	return false;
}

bool GitLowLevelCommands::FindRepoRoot(
	const FString& InPath,
	FString& OutRepoRoot )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	TArray< FString > Params;

	// git rev-parse --show-toplevel
	Params.Add( TEXT( "--show-toplevel" ) );

	const auto bResult = RunGitCommand( TEXT( "rev-parse" ), Params, TArray< FString >(), Results, Queue );

	if ( bResult && Results.InfoMessages.Num() > 0 )
	{
		OutRepoRoot = Results.InfoMessages[ 0 ].ToString();
	}

	return bResult;
}

bool GitLowLevelCommands::GetLocalBranchName(
	FString& OutLocalBranch )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	TArray< FString > Params;

	// git rev-parse --abbrev-ref --symbolic-full-name HEAD // local

	Params.Add( TEXT( "--symbolic-full-name" ) );
	Params.Add( TEXT( "--abbrev-ref" ) );
	Params.Add( TEXT( "HEAD" ) );

	bool bResult = RunGitCommand( TEXT( "rev-parse" ), Params, TArray< FString >(), Results, Queue );

	if ( bResult && Results.InfoMessages.Num() > 0 )
	{
		OutLocalBranch = Results.InfoMessages[ 0 ].ToString();
	}
	else
	{
		Params.Reset();
		Params.Add( TEXT( "-1" ) );
		Params.Add( TEXT( "--format=\"%h\"" ) );
		bResult = RunGitCommand( TEXT( "log" ), Params, TArray< FString >(), Results, Queue );

		if ( bResult && Results.InfoMessages.Num() > 0 )
			OutLocalBranch = TEXT( "HEAD detached at " ) + Results.InfoMessages[ 0 ].ToString();
		else
			bResult = false;
	}

	return bResult;
}

bool GitLowLevelCommands::GetUpstreamBranchName(
	FString& OutUpstreamBranchName )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	TArray< FString > Params;

	// git rev-parse --abbrev-ref --symbolic-full-name @{u} // remote

	Params.Add( TEXT( "--symbolic-full-name" ) );
	Params.Add( TEXT( "--abbrev-ref" ) );
	Params.Add( TEXT( "@{u}" ) );

	const bool bResult = RunGitCommand( TEXT( "rev-parse" ), Params, TArray< FString >(), Results, Queue );

	if ( bResult && Results.InfoMessages.Num() > 0 )
	{
		OutUpstreamBranchName = Results.InfoMessages[ 0 ].ToString();
	}

	return bResult;
}

bool GitLowLevelCommands::GetRemoteUrl(
	const FString& RemoteName,
	FString& OutRemoteUrl )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	TArray< FString > Params;

	Params.Add( TEXT( "get-url" ) );
	Params.Add( RemoteName );

	const bool bResult = RunGitCommand( TEXT( "remote" ), Params, TArray< FString >(), Results, Queue );

	if ( bResult && Results.InfoMessages.Num() > 0 )
	{
		OutRemoteUrl = Results.InfoMessages[ 0 ].ToString();
		return true;
	}

	return false;
}

bool GitLowLevelCommands::GetUserData(
	FString& OutUserName,
	FString& OutUserEmail )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	TArray< FString > Params;

	Params.Add( TEXT( "user.name" ) );
	bool bResult = RunGitCommand( TEXT( "config" ), Params, TArray< FString >(), Results, Queue );
	if ( bResult )
	{
		if ( Results.InfoMessages.Num() == 1 )
			OutUserName = Results.InfoMessages[ 0 ].ToString();
		// TODO: What if not configured?
	}
	else
	{
		bResult = false;
	}

	Params.Reset();
	Params.Add( TEXT( "user.email" ) );
	bResult &= RunGitCommand( TEXT( "config" ), Params, TArray< FString >(), Results, Queue );
	if ( bResult )
	{
		if ( Results.InfoMessages.Num() == 2 )
			OutUserEmail = Results.InfoMessages[ 1 ].ToString();
		// TODO: What if not configured?
	}
	else
	{
		bResult &= false;
	}

	return bResult;
}

bool GitLowLevelCommands::GetAllLocks(
	FSourceControlResultInfo& OutResults,
	TMap< FString, TSharedRef< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe > >& OutLocks )
{
	TQueue< FString > Queue;
	const bool bResult = RunGitCommand( TEXT( "lfs locks" ), TArray< FString >(), TArray< FString >(), OutResults, Queue );
	for ( const FText& Result : OutResults.InfoMessages )
	{
		TSharedRef< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe > LockFile = MakeShared< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe >(
			Result.ToString() );
		if ( GitLogMode == Full )
		{
			UE_LOG( VSPGitLog, Log, TEXT("LockedFile ID:%d (%s, %s)"), LockFile->Id, *LockFile->LocalFilename, *LockFile->LockUser );
		}
		OutLocks.Add( LockFile->LocalFilename, LockFile );
	}

	return bResult;
}

bool GitLowLevelCommands::UpdateStatus(
	const TArray< FString >& InLockableRules,
	TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& OutStates,
	FSourceControlResultInfo& OutResults )
{
	TArray< FString > Params;
	Params.Add( TEXT( "--porcelain=v1" ) );
	Params.Add( TEXT( "--untracked-files=all" ) );
	FSourceControlResultInfo StatusResult;

	TQueue< FString > Queue;
	const bool bResult = RunGitCommand( TEXT( "status" ), Params, TArray< FString >(), StatusResult, Queue );
	if ( bResult )
	{
		GitLowLevelCommandsLocal::ParseStatusResult( StatusResult, OutStates );
		for ( TSharedRef< ISourceControlState, ESPMode::ThreadSafe >& State : OutStates )
		{
			TSharedRef< FVSPGitState, ESPMode::ThreadSafe > GitState = StaticCastSharedRef< FVSPGitState >( State );
			GitState->bUsingGitLfsLocking = VSPGitHelpers::IsFileLockable( GitState->FileName, InLockableRules );
		}
	}
	OutResults.Append( StatusResult );

	return bResult;
}

bool GitLowLevelCommands::GetCommitInfo(
	FString& OutCommitId,
	FString& OutCommitSummary,
	FSourceControlResultInfo& OutResults )
{
	TQueue< FString > Queue;
	TArray< FString > Parameters;
	Parameters.Add( TEXT( "-1" ) );
	Parameters.Add( TEXT( "--format=\"%H %s\"" ) );
	const bool bResults = RunGitCommand( TEXT( "log" ), Parameters, TArray< FString >(), OutResults, Queue );
	if ( bResults && OutResults.InfoMessages.Num() > 0 )
	{
		OutCommitId = OutResults.InfoMessages[ 0 ].ToString().Left( 40 );
		OutCommitSummary = OutResults.InfoMessages[ 0 ].ToString().RightChop( 41 );
	}

	return bResults;
}

bool GitLowLevelCommands::RunCommit(
	const FString& InMessageHead,
	const FString& InMessageContent,
	const TArray< FString >& InParameters,
	const TArray< FString >& InFiles,
	FSourceControlResultInfo& OutResults )
{
	if ( InMessageHead.IsEmpty() )
	{
		OutResults.ErrorMessages.Add( FText::FromString( TEXT( "Commit message is empty" ) ) );
		return false;
	}

	TArray< FString > Parameters;

	// Save commit message in temp file
	FString CommitMessage = InMessageHead;
	if ( !InMessageContent.IsEmpty() )
	{
		CommitMessage += FString::Printf( TEXT( "\n%s" ), *InMessageContent );
	}
	const VSPGitHelpers::FVSPGitScopedTempFile CommitMessageFile( FText::FromString( CommitMessage ) );
	// Create parameter for git command
	FString ParamCommitMessageFile = TEXT( "--file=\"" );
	ParamCommitMessageFile += FPaths::ConvertRelativePathToFull( CommitMessageFile.GetFilename() );
	ParamCommitMessageFile += TEXT( "\"" );
	Parameters.Add( ParamCommitMessageFile );

	// Save files list in temp file
	FString FilesList;
	for ( const FString& FilePath : InFiles )
	{
		FilesList += FString::Printf( TEXT( "%s\n" ), *FilePath );
	}
	const VSPGitHelpers::FVSPGitScopedTempFile FilesListFile( FText::FromString( FilesList ) );
	// Create parameter for git command
	FString ParamFilesListFile = TEXT( "--pathspec-from-file=\"" );
	ParamFilesListFile += FPaths::ConvertRelativePathToFull( FilesListFile.GetFilename() );
	ParamFilesListFile += TEXT( "\"" );
	Parameters.Add( ParamFilesListFile );

	// Adding additional params
	Parameters.Append( InParameters );

	// Run git commit command
	TQueue< FString > Queue;
	const bool bResult = RunGitCommand( TEXT( "commit" ), Parameters, TArray< FString >(), OutResults, Queue );

	return bResult;
}

bool GitLowLevelCommands::GitFetch(
	const FString& RemoteName,
	FSourceControlResultInfo& OutResults,
	TQueue< FString >& OutQueue )
{
	TArray< FString > Params = TArray< FString >();

	Params.Add( FString( "-v" ) );
	Params.Add( FString( TEXT( "--progress \"" ) + RemoteName + TEXT( "\"" ) ) );

	TArray< FString > EmptyResults;
	TArray< FString > Results;

	const bool bResult = RunGitCommand( TEXT( "fetch" ), Params, TArray< FString >(), OutResults, OutQueue );

	return bResult;
}

/**
 * HACK:
 * Sometimes we receive trash line starting with "Downloading" string while getting answer from git using anonymous pipe.
 * Right now there're no ideas and time how to fix it proper way and what is the reason.
 * That's why we have to add this hack to unblock some editor functionality.
 * It deletes first line of array till EOL (including it), if it starts with "Downloading".
 */
void RemoveDownloadingString(TArray<uint8>& BinaryFileContent)
{
	const FString DownloadingString = "Downloading";
	bool StartsWithDownloading = true;

	if (DownloadingString.Len() > BinaryFileContent.Num())
	{
		StartsWithDownloading = false;
	}
	else
	{
		for (int i = 0; i < DownloadingString.Len(); ++i)
		{
			if (DownloadingString[i] != BinaryFileContent[i])
			{
				StartsWithDownloading = false;
				break;
			}
		}
	}

	if (StartsWithDownloading)
	{
		UE_LOG(VSPGitLog, Warning, TEXT("Found 'downloading' string in the first line of file while saving: removing it."));
		int EOLIndex = BinaryFileContent.Find(0x0A);

		if (EOLIndex != INDEX_NONE)
		{
			FMemory::Memmove(&BinaryFileContent[0], &BinaryFileContent[EOLIndex + 1], BinaryFileContent.Num() - EOLIndex);
			BinaryFileContent.SetNum(BinaryFileContent.Num() - EOLIndex);
		}
	}
}

bool GitLowLevelCommands::RunDumpToFile(
	const FString& InParameter,
	const FString& InDumpFilename )
{
	int32 ReturnCode = -1;
	FString FullCommand;

	if ( !RepoRoot.IsEmpty() )
	{
		// Specify the working copy (the root) of the git repository (before the command itself)
		FullCommand = TEXT( "-C \"" );
		FullCommand += RepoRoot;
		FullCommand += TEXT( "\" " );
	}

	FullCommand += TEXT( "cat-file --filters " );

	FullCommand += InParameter;

	const bool bLaunchDetached = false;
	const bool bLaunchHidden = true;
	const bool bLaunchReallyHidden = bLaunchHidden;

	void* PipeRead = nullptr;
	void* PipeWrite = nullptr;

	verify( FPlatformProcess::CreatePipe(PipeRead, PipeWrite) );

	UE_LOG( VSPGitLog, Log, TEXT("RunDumpToFile: 'git %s'"), *FullCommand );

	FProcHandle ProcessHandle = FPlatformProcess::CreateProc(
		*GitBinPath,
		*FullCommand,
		bLaunchDetached,
		bLaunchHidden,
		bLaunchReallyHidden,
		nullptr,
		0,
		*RepoRoot,
		PipeWrite );
	if ( ProcessHandle.IsValid() )
	{
		FPlatformProcess::Sleep( 0.01 );

		TArray< uint8 > BinaryFileContent;
		while ( FPlatformProcess::IsProcRunning( ProcessHandle ) )
		{
			TArray< uint8 > BinaryData;
			FPlatformProcess::ReadPipeToArray( PipeRead, BinaryData );
			if ( BinaryData.Num() > 0 )
				BinaryFileContent.Append( MoveTemp( BinaryData ) );
		}
		TArray< uint8 > BinaryData;
		FPlatformProcess::ReadPipeToArray( PipeRead, BinaryData );
		if ( BinaryData.Num() > 0 )
		{
			BinaryFileContent.Append( MoveTemp( BinaryData ) );
		}

		// HACK: check function's definition to get more details.
		RemoveDownloadingString(BinaryFileContent);

		FPlatformProcess::GetProcReturnCode( ProcessHandle, &ReturnCode );
		if ( ReturnCode == 0 )
		{
			// Save buffer into temp file
			if ( FFileHelper::SaveArrayToFile( BinaryFileContent, *InDumpFilename ) )
			{
				UE_LOG( VSPGitLog, Log, TEXT("Saved file '%s' (%do)"), *InDumpFilename, BinaryFileContent.Num() );
			}
			else
			{
				UE_LOG( VSPGitLog, Warning, TEXT("Could not write %s"), *InDumpFilename );
				ReturnCode = -1;
			}
		}
		else
		{
			UE_LOG( VSPGitLog, Warning, TEXT("DumpToFile: ReturnCode=%d"), ReturnCode );
		}

		FPlatformProcess::CloseProc( ProcessHandle );
	}
	else
	{
		UE_LOG( VSPGitLog, Warning, TEXT("Failed to launch 'git cat-file'") );
	}

	FPlatformProcess::ClosePipe( PipeRead, PipeWrite );

	return ( ReturnCode == 0 );
}

bool GitLowLevelCommands::GetDivergence(
	const FString& LeftBranch,
	const FString& RightBranch,
	int32& OutLeftCountCommit,
	int32& OutRightCountCommit )
{
	TArray< FStringFormatArg > args;
	args.Add( FStringFormatArg( LeftBranch ) );
	args.Add( FStringFormatArg( RightBranch ) );

	TArray< FString > Params;
	Params.Add( TEXT( "--left-right" ) );
	Params.Add( TEXT( "--count" ) );
	Params.Add( FString::Format( TEXT( "{0}...{1}" ), args ) );

	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	const bool bResult = RunGitCommand( TEXT( "rev-list" ), Params, TArray< FString >(), Results, Queue );

	OutLeftCountCommit = OutRightCountCommit = 0;
	for ( FText& ResultLine : Results.InfoMessages )
	{
		FString LeftS;
		FString RightS;
		if ( ResultLine.ToString().Split( "\t", &LeftS, &RightS ) )
		{
			FDefaultValueHelper::ParseInt( LeftS, OutLeftCountCommit );
			FDefaultValueHelper::ParseInt( RightS, OutRightCountCommit );
		}
	}

	return bResult;
}

/**
* Translate file actions from the given Git log --name-status command to keywords used by the Editor UI.
*
* @see https://www.kernel.org/pub/software/scm/git/docs/git-log.html
* ' ' = unmodified
* 'M' = modified
* 'A' = added
* 'D' = deleted
* 'R' = renamed
* 'C' = copied
* 'T' = type changed
* 'U' = updated but unmerged
* 'X' = unknown
* 'B' = broken pairing
*
* @see SHistoryRevisionListRowContent::GenerateWidgetForColumn(): "add", "edit", "delete", "branch" and "integrate" (everything else is taken like "edit")
*/
FString GitLowLevelCommandsLocal::LogStatusToString( TCHAR InStatus )
{
	switch ( InStatus )
	{
		case TEXT( ' ' ):
			return FString( "unmodified" );
		case TEXT( 'M' ):
			return FString( "modified" );
		case TEXT( 'A' ): // added: keyword "add" to display a specific icon instead of the default "edit" action one
			return FString( "add" );
		case TEXT( 'D' ): // deleted: keyword "delete" to display a specific icon instead of the default "edit" action one
			return FString( "delete" );
		case TEXT( 'R' ): // renamed keyword "branch" to display a specific icon instead of the default "edit" action one
			return FString( "branch" );
		case TEXT( 'C' ): // copied keyword "branch" to display a specific icon instead of the default "edit" action one
			return FString( "branch" );
		case TEXT( 'T' ):
			return FString( "type changed" );
		case TEXT( 'U' ):
			return FString( "unmerged" );
		case TEXT( 'X' ):
			return FString( "unknown" );
		case TEXT( 'B' ):
			return FString( "broken pairing" );
		default: ;
	}

	return FString();
}

// ReSharper disable CommentTypo
/**
 * Parse the array of strings results of a 'git log' command
 *
 * Example git log results:
commit 97a4e7626681895e073aaefd68b8ac087db81b0b
Author: Sébastien Rombauts <sebastien.rombauts@gmail.com>
Date:   2014-2015-05-15 21:32:27 +0200

    Another commit used to test History

     - with many lines
     - some <xml>
     - and strange characteres $*+

M	Content/Blueprints/Blueprint_CeilingLight.uasset
R100	Content/Textures/T_Concrete_Poured_D.uasset Content/Textures/T_Concrete_Poured_D2.uasset

commit 355f0df26ebd3888adbb558fd42bb8bd3e565000
Author: Sébastien Rombauts <sebastien.rombauts@gmail.com>
Date:   2014-2015-05-12 11:28:14 +0200

    Testing git status, edit, and revert

A	Content/Blueprints/Blueprint_CeilingLight.uasset
C099	Content/Textures/T_Concrete_Poured_N.uasset Content/Textures/T_Concrete_Poured_N2.uasset
*/
// ReSharper restore CommentTypo
void GitLowLevelCommandsLocal::ParseLogResults(
	const FSourceControlResultInfo& InResults,
	TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > >& OutHistory )
{
	TSharedRef< FVSPGitRevision, ESPMode::ThreadSafe > VSPGitRevision = MakeShareable( new FVSPGitRevision() );
	for ( const FText& Result : InResults.InfoMessages )
		if ( Result.ToString().StartsWith( TEXT( "commit " ) ) ) // Start of a new commit
		{
			// End of the previous commit
			if ( VSPGitRevision->RevisionNumber != 0 )
			{
				OutHistory.Add( MoveTemp( VSPGitRevision ) );

				VSPGitRevision = MakeShareable( new FVSPGitRevision );
			}
			VSPGitRevision->CommitId = Result.ToString().RightChop( 7 ); // Full commit SHA1 hexadecimal string
			VSPGitRevision->ShortCommitId = VSPGitRevision->CommitId.Left( 8 ); // Short revision ; first 8 hex characters (max that can hold a 32 bit integer)
			VSPGitRevision->CommitIdNumber = FParse::HexNumber( *VSPGitRevision->ShortCommitId );
			VSPGitRevision->RevisionNumber = -1; // RevisionNumber will be set at the end, based off the index in the History
		}
		else if ( Result.ToString().StartsWith( TEXT( "Author: " ) ) ) // Author name & email
		{
			// Remove the 'email' part of the UserName
			FString UserNameEmail = Result.ToString().RightChop( 8 );
			int32 EmailIndex = 0;
			if ( UserNameEmail.FindLastChar( '<', EmailIndex ) )
				VSPGitRevision->UserName = UserNameEmail.Left( EmailIndex - 1 );
		}
		else if ( Result.ToString().StartsWith( TEXT( "Date:   " ) ) ) // Commit date
		{
			FString Date = Result.ToString().RightChop( 8 );
			VSPGitRevision->Date = FDateTime::FromUnixTimestamp( FCString::Atoi( *Date ) );
		}
			//	else if(Result.IsEmpty()) // empty line before/after commit message has already been taken care by FString::ParseIntoArray()
		else if ( Result.ToString().StartsWith( TEXT( "    " ) ) ) // Multi-lines commit message
		{
			VSPGitRevision->Description += Result.ToString().RightChop( 4 );
			VSPGitRevision->Description += TEXT( "\n" );
		}
		else // Name of the file, starting with an uppercase status letter ("A"/"M"...)
		{
			const TCHAR Status = Result.ToString()[ 0 ];
			VSPGitRevision->Action = LogStatusToString( Status ); // Readable action string ("Added", Modified"...) instead of "A"/"M"...
			// Take care of special case for Renamed/Copied file: extract the second filename after second tabulation
			int32 IdxTab;
			if ( Result.ToString().FindLastChar( '\t', IdxTab ) )
				VSPGitRevision->Filename = Result.ToString().RightChop( IdxTab + 1 ); // relative filename
		}
	// End of the last commit
	if ( VSPGitRevision->RevisionNumber != 0 )
		OutHistory.Add( MoveTemp( VSPGitRevision ) );

	// Then set the revision number of each Revision based on its index (reverse order since the log starts with the most recent change)
	for ( int32 RevisionIndex = 0; RevisionIndex < OutHistory.Num(); RevisionIndex++ )
	{
		TSharedRef< FVSPGitRevision, ESPMode::ThreadSafe > SourceControlRevisionItem =
			StaticCastSharedRef< FVSPGitRevision >( OutHistory[ RevisionIndex ] );
		SourceControlRevisionItem->RevisionNumber = OutHistory.Num() - RevisionIndex;

		// Special case of a move ("branch" in Perforce term): point to the previous change (so the next one in the order of the log)
		if ( ( SourceControlRevisionItem->Action == "branch" ) && ( RevisionIndex < OutHistory.Num() - 1 ) )
			SourceControlRevisionItem->BranchSource = OutHistory[ RevisionIndex + 1 ];
	}
}

bool GitLowLevelCommands::RunGetHistory(
	const FString& InFile,
	bool bMergeConflict,
	FSourceControlResultInfo& OutResults,
	TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > >& OutHistory )
{
	TArray< FString > Parameters;
	TQueue< FString > Queue;
	Parameters.Add( TEXT( "--follow" ) ); // follow file renames
	Parameters.Add( TEXT( "--date=raw" ) );
	Parameters.Add( TEXT( "--name-status" ) ); // relative filename at this revision, preceded by a status character
	Parameters.Add( TEXT( "--pretty=medium" ) ); // make sure format matches expected in ParseLogResults
	if ( bMergeConflict )
	{
		// In case of a merge conflict, we also need to get the tip of the "remote branch" (MERGE_HEAD) before the log of the "current branch" (HEAD)
		// @todo does not work for a cherry-pick! Test for a rebase.
		Parameters.Add( TEXT( "MERGE_HEAD" ) );
		Parameters.Add( TEXT( "--max-count 1" ) );
	}
	TArray< FString > Files;
	Files.Add( *InFile );
	bool bResults = RunGitCommand( TEXT( "log" ), Parameters, Files, OutResults, Queue );
	if ( bResults )
		GitLowLevelCommandsLocal::ParseLogResults( OutResults, OutHistory );

	for ( TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe >& Revision : OutHistory )
	{
		TSharedRef< FVSPGitRevision, ESPMode::ThreadSafe > GitRevision = StaticCastSharedRef< FVSPGitRevision >( Revision );
		// Get file (blob) sha1 id and size
		FSourceControlResultInfo LsResults;
		TArray< FString > LsParameters;
		LsParameters.Add( TEXT( "--long" ) ); // Show object size of blob (file) entries.
		LsParameters.Add( GitRevision->GetRevision() );
		TArray< FString > LsFiles;
		LsFiles.Add( *GitRevision->GetFilename() );
		bResults &= RunGitCommand( TEXT( "ls-tree" ), LsParameters, LsFiles, LsResults, Queue );
		if ( bResults && LsResults.InfoMessages.Num() )
		{
			const VSPGitHelpers::FGitLsTreeParser LsTree( LsResults.InfoMessages );
			GitRevision->FileHash = LsTree.FileHash;
			GitRevision->FileSize = LsTree.FileSize;
		}
	}

	return bResults;
}

/** Execute a command to get the details of a conflict */
void GitLowLevelCommandsLocal::RunGetConflictStatus(
	const FString& InFile,
	TSharedRef< FVSPGitState, ESPMode::ThreadSafe >& FileState )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	TArray< FString > Files;
	Files.Add( InFile );
	TArray< FString > Parameters;
	Parameters.Add( TEXT( "--unmerged" ) );
	const bool bResult = GitLowLevelCommands::RunGitCommand( TEXT( "ls-files" ), Parameters, Files, Results, Queue );
	if ( bResult && Results.InfoMessages.Num() == 3 )
	{
		// Parse the unmerge status: extract the base revision (or the other branch?)
		const FString& FirstResult = Results.InfoMessages[ 0 ].ToString(); // 1: The common ancestor of merged branches
		FileState->PendingMergeBaseFileHash = FirstResult.Mid( 7, 40 ); ///< SHA1 Id of the file (warning: not the commit Id)
	}
}

// Parse one row from of Porcelain Format Version 1
bool GitLowLevelCommandsLocal::ParseFileState(
	const FString& StatusResult,
	EGitState::Type& OutFileGitState,
	FString& OutFileName )
{
	OutFileGitState = EGitState::Unknown;
	OutFileName = TEXT( "" );

	if ( StatusResult.IsEmpty() )
		return false;

	// parse git status
	const TCHAR IndexState = StatusResult[ 0 ];
	const TCHAR WorkingCopyState = StatusResult[ 1 ];

	if ( ( IndexState == 'U' || WorkingCopyState == 'U' ) ||
		( IndexState == 'A' && WorkingCopyState == 'A' ) ||
		( IndexState == 'D' && WorkingCopyState == 'D' ) )
		OutFileGitState = EGitState::Conflicted;
	else if ( IndexState == 'A' )
		OutFileGitState = EGitState::Added;
	else if ( IndexState == 'D' )
		OutFileGitState = EGitState::Deleted;
	else if ( WorkingCopyState == 'D' )
		OutFileGitState = EGitState::Missing;
	else if ( IndexState == 'M' || WorkingCopyState == 'M' )
		OutFileGitState = EGitState::Modified;
	else if ( IndexState == 'R' )
		OutFileGitState = EGitState::Renamed;
	else if ( IndexState == 'C' )
		OutFileGitState = EGitState::Copied;
	else if ( IndexState == '?' || WorkingCopyState == '?' )
		OutFileGitState = EGitState::NotControlled;
	else if ( IndexState == '!' || WorkingCopyState == '!' )
		OutFileGitState = EGitState::Ignored;
	else // UNKNOWN State
		return false;

	// get file name
	auto Index = StatusResult.Find( " -> " );
	if ( Index == INDEX_NONE )
		Index = 3;
	else
		Index += 4; // Len of " -> "
	OutFileName = StatusResult.RightChop( Index );

	if ( OutFileName.IsEmpty() )
		return false; // unknown row format

	return true;
}

void GitLowLevelCommandsLocal::ParseStatusResult(
	const FSourceControlResultInfo& InResults,
	TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& OutStates )
{
	for ( const FText& ResultLine : InResults.InfoMessages )
	{
		FString RelativeFilename;
		EGitState::Type GitState;
		if ( ParseFileState( ResultLine.ToString(), GitState, RelativeFilename ) )
		{
			if ( FPaths::DirectoryExists( RelativeFilename ) )
				continue;

			TSharedRef< FVSPGitState, ESPMode::ThreadSafe > NewState = MakeShared< FVSPGitState, ESPMode::ThreadSafe >( RelativeFilename, GitState );
			NewState->TimeStamp = FDateTime::Now();
			if ( NewState->IsConflicted() )
				// In case of a conflict (unmerged file) get the base revision to merge
				RunGetConflictStatus( RelativeFilename, NewState );
			OutStates.Add( NewState );
		}
	}
}
