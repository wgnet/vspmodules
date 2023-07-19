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
#include "VSPGitRevision.h"
// Module headers
#include "Commands/GitLowLevelCommands.h"
#include "VSPGitModule.h"

bool FVSPGitRevision::Get( FString& InOutFilename ) const
{
	// if a filename for the temp file wasn't supplied generate a unique-ish one
	if ( InOutFilename.Len() == 0 )
	{
		// create the diff dir if we don't already have it (Git wont)
		IFileManager::Get().MakeDirectory( *FPaths::DiffDir(), true );
		// create a unique temp file name based on the unique commit Id
		const FString TempFileName = FString::Printf( TEXT( "%stemp-%s-%s" ), *FPaths::DiffDir(), *CommitId, *FPaths::GetCleanFilename( Filename ) );
		InOutFilename = FPaths::ConvertRelativePathToFull( TempFileName );
	}

	// Diff against the revision
	const FString Parameter = FString::Printf( TEXT( "%s:%s" ), *CommitId, *Filename );

	bool bCommandSuccessful;
	if ( FPaths::FileExists( InOutFilename ) )
		bCommandSuccessful = true; // if the temp file already exists, reuse it directly
	else
	{
		const FString PathToGitBinary = FVSPGitProvider::Get().GetSettings()->GetGitBinPath();
		const FString PathToRepositoryRoot = FVSPGitProvider::Get().GetSettings()->GetCurrentRepoSettings()->RepoRoot;
		bCommandSuccessful = GitLowLevelCommands::RunDumpToFile( Parameter, InOutFilename );
	}
	return bCommandSuccessful;
}

bool FVSPGitRevision::GetAnnotated( TArray< FAnnotationLine >& OutLines ) const
{
	return false;
}

bool FVSPGitRevision::GetAnnotated( FString& InOutFilename ) const
{
	return false;
}

const FString& FVSPGitRevision::GetFilename() const
{
	return Filename;
}

const FString& FVSPGitRevision::GetRevision() const
{
	return ShortCommitId;
}

int32 FVSPGitRevision::GetRevisionNumber() const
{
	return RevisionNumber;
}

const FString& FVSPGitRevision::GetDescription() const
{
	return Description;
}

const FString& FVSPGitRevision::GetUserName() const
{
	return UserName;
}

const FString& FVSPGitRevision::GetClientSpec() const
{
	static FString EmptyString( TEXT( "" ) );
	return EmptyString;
}

const FString& FVSPGitRevision::GetAction() const
{
	return Action;
}

TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > FVSPGitRevision::GetBranchSource() const
{
	// if this revision was copied/moved from some other revision
	return BranchSource;
}

const FDateTime& FVSPGitRevision::GetDate() const
{
	return Date;
}

int32 FVSPGitRevision::GetCheckInIdentifier() const
{
	return CommitIdNumber;
}

int32 FVSPGitRevision::GetFileSize() const
{
	return FileSize;
}
