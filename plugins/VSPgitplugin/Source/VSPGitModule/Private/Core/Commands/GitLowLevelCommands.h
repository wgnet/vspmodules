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

#include "CoreMinimal.h"

#include "ISourceControlOperation.h"
#include "ISourceControlState.h"
#include "Core/Base/VSPGitHelpers.h"

namespace GitLowLevelCommands
{
	static enum EGitLogMode
	{
		Silent,					// won't log anything
		Verbose,				// everything is in verbose mode
		Short,					// 1 command - 1 line and return code of the git
		Full,					// command and full git output
		ShortWithErrorOutput,	// 1 command - and full output if error
	};

	static FString GitBinPath = TEXT( "git" );
	static FString RepoRoot;
	static EGitLogMode GitLogMode = ShortWithErrorOutput;

	bool RunGitCommandLow(
		const FString& InCommand,
		const TArray< FString >& InParams,
		const TArray< FString >& InFiles,
		FSourceControlResultInfo& OutResults,
		TQueue< FString >& OutOutputQueue );

	bool RunGitCommand(
		const FString& InCommand,
		const TArray< FString >& InParams,
		const TArray< FString >& InFiles,
		FSourceControlResultInfo& OutResults,
		TQueue< FString >& OutOutputQueue );

	bool CheckGit(
		VSPGitHelpers::FGitVersion* OutVersion = nullptr );

	bool CheckLfs(
		FString& OutGitLfsVersion );

	bool FindRepoRoot(
		const FString& InPath,
		FString& OutRepoRoot );

	bool GetLocalBranchName(
		FString& OutLocalBranch );

	bool GetUpstreamBranchName(
		FString& OutUpstreamBranchName );

	bool GetRemoteUrl(
		const FString& RemoteName,
		FString& OutRemoteUrl );

	bool GetUserData(
		FString& OutUserName,
		FString& OutUserEmail );

	bool GetAllLocks(
		FSourceControlResultInfo& OutResults,
		TMap< FString, TSharedRef< VSPGitHelpers::FGitLfsLockInfo, ESPMode::ThreadSafe > >& OutLocks );

	bool UpdateStatus(
		const TArray< FString >& InLockableRules,
		TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > >& OutStates,
		FSourceControlResultInfo& OutResults );

	bool GetCommitInfo(
		FString& OutCommitId,
		FString& OutCommitSummary,
		FSourceControlResultInfo& OutResults );

	bool RunCommit(
		const FString& InMessageHead,
		const FString& InMessageContent,
		const TArray< FString >& InParameters,
		const TArray< FString >& InFiles,
		FSourceControlResultInfo& OutResults );

	bool GitFetch(
		const FString& RemoteName,
		FSourceControlResultInfo& OutResults,
		TQueue< FString >& OutQueue );

	bool RunDumpToFile(
		const FString& InParameter,
		const FString& InDumpFilename );

	bool GetDivergence(
		const FString& LeftBranch,
		const FString& RightBranch,
		int32& OutLeftCountCommit,
		int32& OutRightCountCommit );

	bool RunGetHistory(
		const FString& InFile,
		bool bMergeConflict,
		FSourceControlResultInfo& OutResults,
		TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > >& OutHistory );
};
