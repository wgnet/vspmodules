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
#include "VSPGitHelpers.h"
// Module headers
#include "SourceControlHelpers.h"
#include "VSPGitModule.h"
#include "Core/VSPGitState.h"
#include "Core/Commands/GitLowLevelCommands.h"

#define LOCTEXT_NAMESPACE "FVSPGitModule"

namespace VSPGitHelpersLocal
{
	FString FilenameFromGitStatus( const FString& InResult );
}

VSPGitHelpers::FGitLfsLockInfo::FGitLfsLockInfo( const FString& InStatus )
	: Id( -1 )
{
	TArray< FString > Information;
	InStatus.ParseIntoArray( Information, TEXT( "\t" ), true );
	if ( Information.Num() >= 3 )
	{
		Information[ 0 ].TrimEndInline(); // Trim whitespace from the end of the filename
		Information[ 1 ].TrimEndInline(); // Trim whitespace from the end of the username
		Information[ 2 ].TrimEndInline(); // Trim whitespace from the end of the ID:
		Information[ 2 ].RemoveFromStart( TEXT( "ID:" ) );
		LocalFilename = MoveTemp( Information[ 0 ] );
		LockUser = MoveTemp( Information[ 1 ] );
		Id = FCString::Atoi( *Information[ 2 ] );
	}
}

VSPGitHelpers::FVSPGitScopedTempFile::FVSPGitScopedTempFile( const FText& InText )
{
	Filename = FPaths::CreateTempFilename( *FPaths::ProjectLogDir(), TEXT( "Git-Temp" ), TEXT( ".txt" ) );
	if ( !FFileHelper::SaveStringToFile( InText.ToString(), *Filename, FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM ) )
	{
		UE_LOG( VSPGitLog, Error, TEXT("Failed to write to temp file: %s"), *Filename );
	}
}

VSPGitHelpers::FVSPGitScopedTempFile::~FVSPGitScopedTempFile()
{
	if ( FPaths::FileExists( Filename ) )
		if ( !FPlatformFileManager::Get().GetPlatformFile().DeleteFile( *Filename ) )
		{
			UE_LOG( VSPGitLog, Error, TEXT("Failed to delete temp file: %s"), *Filename );
		}
}

const FString& VSPGitHelpers::FVSPGitScopedTempFile::GetFilename() const
{
	return Filename;
}

bool VSPGitHelpers::FVSPGitStatusFileMatcher::operator()( const FString& InResult ) const
{
	return AbsoluteFilename.Contains( VSPGitHelpersLocal::FilenameFromGitStatus( InResult ) );
}

void VSPGitHelpers::ParseGitVersion( const FString& InVersionString, FGitVersion* OutGitVersion )
{
	// Parse "git version 2.11.0.windows.3" into the string tokens "git", "version", "2.11.0.windows.3"
	TArray< FString > TokenizedString;
	InVersionString.ParseIntoArrayWS( TokenizedString );

	// Select the string token containing the version "2.11.0.windows.3"
	const FString* TokenVersionStringPtr = TokenizedString.FindByPredicate( []( FString& s ) { return TChar< TCHAR >::IsDigit( s[ 0 ] ); } );
	if ( TokenVersionStringPtr )
	{
		// Parse the version into its numerical components
		TArray< FString > ParsedVersionString;
		TokenVersionStringPtr->ParseIntoArray( ParsedVersionString, TEXT( "." ) );
		if ( ParsedVersionString.Num() >= 3 )
			if ( ParsedVersionString[ 0 ].IsNumeric() && ParsedVersionString[ 1 ].IsNumeric() && ParsedVersionString[ 2 ].IsNumeric() )
			{
				OutGitVersion->Major = FCString::Atoi( *ParsedVersionString[ 0 ] );
				OutGitVersion->Minor = FCString::Atoi( *ParsedVersionString[ 1 ] );
				OutGitVersion->Patch = FCString::Atoi( *ParsedVersionString[ 2 ] );
				if ( ParsedVersionString.Num() >= 5 )
					if ( ( ParsedVersionString[ 3 ] == TEXT( "windows" ) ) && ParsedVersionString[ 4 ].IsNumeric() )
						OutGitVersion->Windows = FCString::Atoi( *ParsedVersionString[ 4 ] );
				UE_LOG(
					VSPGitLog,
					Log,
					TEXT("Git version: v%d.%d.%d.windows.%d"),
					OutGitVersion->Major,
					OutGitVersion->Minor,
					OutGitVersion->Patch,
					OutGitVersion->Windows );
			}
	}
}

void VSPGitHelpers::FindGitCapabilities( FGitVersion* OutGitVersion )
{
	// TODO: What is that? Is it need?
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	GitLowLevelCommands::RunGitCommand( TEXT( "cat-file -h" ), TArray< FString >(), TArray< FString >(), Results, Queue );
	if ( Results.InfoMessages[ 0 ].ToString().Contains( "--filters" ) )
	{
		//OutGitVersion->bHasCatFileWithFilters = true;
	}
}

void VSPGitHelpers::FindGitLfsCapabilities( FGitVersion* OutGitVersion )
{
	FSourceControlResultInfo Results;
	TQueue< FString > Queue;
	const bool bResult = GitLowLevelCommands::RunGitCommand( TEXT( "lfs version" ), TArray< FString >(), TArray< FString >(), Results, Queue );
	if ( bResult && Results.InfoMessages.Num() > 0 )
	{
		OutGitVersion->bHasGitLfs = true;

		if ( Results.InfoMessages[ 0 ].ToString().Compare( TEXT( "git-lfs/2.0.0" ) ) >= 0 )
			OutGitVersion->bHasGitLfsLocking = true; // Git LFS File Locking workflow introduced in "git-lfs/2.0.0"
		UE_LOG( VSPGitLog, Log, TEXT("Git LFS version: %s"), *(Results.InfoMessages[0].ToString()) );
	}
}

void VSPGitHelpers::ParseCumulativeStream( FString& CurrentCumulativeString, TQueue< FString >& OutQueueLines )
{
	TArray< FString > Lines;
	CurrentCumulativeString.ParseIntoArrayLines( Lines );
	if ( Lines.Num() > 1 )
	{
		CurrentCumulativeString = Lines.Last();
		Lines.RemoveAt( Lines.Num() - 1 );
		for ( FString Line : Lines )
			if ( !Line.IsEmpty() )
				OutQueueLines.Enqueue( Line );
	}
}

bool VSPGitHelpers::GetLockableRuleList( const FString& InRepoRoot, TArray< FString >& OutRules )
{
	const FString GitAttributesPath = FPaths::Combine( InRepoRoot, TEXT( ".gitattributes" ) );

	if ( !FPaths::FileExists( GitAttributesPath ) )
		return false;

	TArray< FString > FileLines;
	FFileHelper::LoadFileToStringArray( FileLines, *GitAttributesPath );

	OutRules.Empty();
	for ( auto&& FileLine : FileLines )
		if ( FileLine.Contains( TEXT( "lockable" ) ) )
		{
			TArray< FString > Words;
			FileLine.ParseIntoArray( Words, TEXT( " " ) );

			if ( Words.Num() > 0 )
				OutRules.Add( Words[ 0 ] );
		}

	return true;
}

bool VSPGitHelpers::IsFileLockable( const FString& InFileName, const TArray< FString >& InLockableRules )
{
	const FString FileExtension = FPaths::GetExtension( InFileName );
	for ( auto&& LockableRule : InLockableRules )
		if ( LockableRule.Contains( FileExtension ) )
			return true;
	return false;
}

bool VSPGitHelpers::IsLockByMe( const FString& InLockUser )
{
	TSharedRef< FVSPGitSettings, ESPMode::ThreadSafe > Settings = FVSPGitProvider::Get().GetSettings();
	const TSharedPtr< FRepoSettings, ESPMode::ThreadSafe > RepoSettings = Settings->GetCurrentRepoSettings();
	return InLockUser.Contains( RepoSettings->UserName ) || InLockUser.Contains( RepoSettings->UserEmail );
}

FString VSPGitHelpersLocal::FilenameFromGitStatus( const FString& InResult )
{
	int32 RenameIndex;
	if ( InResult.FindLastChar( '>', RenameIndex ) )
		// Extract only the second part of a rename "from -> to"
		return InResult.RightChop( RenameIndex + 2 );
	// Extract the relative filename from the Git status result (after the 2 letters status and 1 space)
	return InResult.RightChop( 3 );
}

void VSPGitHelpers::GetMissingVsExistingFiles(
	const TArray< FString >& InFiles,
	TArray< FString >& OutMissingFiles,
	TArray< FString >& OutAllExistingFiles,
	TArray< FString >& OutOtherThanAddedExistingFiles )
{
	auto& Provider = FVSPGitProvider::Get();
	const TArray< FString > Files = ( InFiles.Num() > 0 ) ? ( InFiles ) : ( Provider.GetCachedFilesList() );

	TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > LocalStates;
	Provider.GetState( Files, LocalStates, EStateCacheUsage::Use );
	for ( const auto& State : LocalStates )
		if ( FPaths::FileExists( RelativeByRepoPathToFull( State->GetFilename() ) ) )
		{
			if ( State->IsAdded() )
				OutAllExistingFiles.Add( State->GetFilename() );
			else if ( State->IsModified() )
			{
				OutOtherThanAddedExistingFiles.Add( State->GetFilename() );
				OutAllExistingFiles.Add( State->GetFilename() );
			}
			else if ( State->CanRevert() ) // for locked but unmodified files
				OutOtherThanAddedExistingFiles.Add( State->GetFilename() );
		}
		else
		{
			if ( State->IsSourceControlled() )
				OutMissingFiles.Add( State->GetFilename() );
		}
}

FText VSPGitHelpers::ParseCommitResults( const TArray< FText >& InResults )
{
	if ( InResults.Num() >= 2 )
		return FText::Format( LOCTEXT( "CommitMessage", "Commited {0}." ), InResults[ InResults.Num() - 2 ] );
	return LOCTEXT( "CommitMessageUnknown", "Submitted revision." );
}

FString VSPGitHelpers::RelativeByRepoPathToFull( const FString& InRelativePath )
{
	FString TrueRelativePath = InRelativePath;
	if ( ( TrueRelativePath.StartsWith( TEXT( ".." ) ) ) || ( TrueRelativePath.Contains( TEXT( "Binaries/Win64" ) ) ) )
	{
		int32 Index = TrueRelativePath.Find( TEXT( "Content" ) );
		TrueRelativePath = TrueRelativePath.Right( TrueRelativePath.Len() - Index );
	}
	FString FullPath = FPaths::Combine( FVSPGitProvider::Get().GetSettings()->GetCurrentRepoSettings()->RepoRoot, TrueRelativePath );
	//check(FPaths::FileExists(FullPath));
	return FullPath;
}

TArray< FString > VSPGitHelpers::RelativeByRepoPathToFull( const TArray< FString >& InRelativePaths )
{
	TArray< FString > AbsPaths;
	for ( auto&& RelativePath : InRelativePaths )
		AbsPaths.Add( RelativeByRepoPathToFull( RelativePath ) );
	return AbsPaths;
}

TArray< FString > VSPGitHelpers::AbsoluteAndFilteringFilenames( const TArray< FString >& InFileNames, const FString& InRelativeTo )
{
	TArray< FString > AbsFiles;
	for ( FString FileName : InFileNames ) // string copy to be able to convert it inplace
	{
		FString AbsPath = FileName;
		if ( FPaths::IsRelative( FileName ) )
		{
			AbsPath = FPaths::ConvertRelativePathToFull( FileName );
			if ( !FPaths::IsUnderDirectory( AbsPath, InRelativeTo ) )
				AbsPath = FPaths::Combine( InRelativeTo, FileName );
		}

		if ( FPaths::IsUnderDirectory( AbsPath, InRelativeTo ) )
			AbsFiles.Add( AbsPath );
	}

	return AbsFiles;
}

TArray< FString > VSPGitHelpers::CheckAndMakeRelativeFilenames(
	const TArray< FString >& InFileNames,
	const FString& InRelativeTo )
{
	TArray< FString > RelativeFiles;
	FString RelativeTo = InRelativeTo;

	// Ensure that the path ends w/ '/'
	if ( ( RelativeTo.Len() > 0 )
		&& ( RelativeTo.EndsWith( TEXT( "/" ), ESearchCase::CaseSensitive ) == false )
		&& ( RelativeTo.EndsWith( TEXT( "\\" ), ESearchCase::CaseSensitive ) == false ) )
		RelativeTo += TEXT( "/" );

	for ( FString FileName : InFileNames ) // string copy to be able to convert it in place
	{
		FString AbsPath = FileName;
		if ( FPaths::IsRelative( FileName ) )
		{
			AbsPath = FPaths::ConvertRelativePathToFull( FileName );
			if ( !FPaths::IsUnderDirectory( AbsPath, InRelativeTo ) )
				AbsPath = FPaths::Combine( InRelativeTo, FileName );
			else
			{
				check( FPaths::IsUnderDirectory(AbsPath, InRelativeTo) );
			}
		}
		if ( FPaths::MakePathRelativeTo( AbsPath, *RelativeTo ) )
			RelativeFiles.Add( AbsPath );
	}

	return RelativeFiles;
}

#undef LOCTEXT_NAMESPACE
