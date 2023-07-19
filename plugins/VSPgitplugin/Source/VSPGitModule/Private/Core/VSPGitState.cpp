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
#include "VSPGitState.h"
// Module headers
#include "VSPGitRevision.h"
#include "VSPGitModule.h"

FVSPGitState::FVSPGitState( const FString InRelativePath, const EGitState::Type InState )
	: FileName( InRelativePath )
	, TimeStamp( 0 )
	, State( InState )
	, bUsingGitLfsLocking( false )
	, bIsLocked( false )
	, LockId( -1 )
	, bNewerVersionOnServer( false )
{
}

int32 FVSPGitState::GetHistorySize() const
{
	return History.Num();
}

TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > FVSPGitState::GetHistoryItem( int32 HistoryIndex ) const
{
	check( History.IsValidIndex(HistoryIndex) );

	return History[ HistoryIndex ];
}

TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > FVSPGitState::FindHistoryRevision( int32 RevisionNumber ) const
{
	for ( const auto& Revision : History )
		if ( Revision->GetRevisionNumber() == RevisionNumber )
			return Revision;

	return nullptr;
}

TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > FVSPGitState::FindHistoryRevision( const FString& InRevision ) const
{
	for ( const auto& Revision : History )
		if ( Revision->GetRevision() == InRevision )
			return Revision;

	return nullptr;
}

TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > FVSPGitState::GetBaseRevForMerge() const
{
	for ( const TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe >& Revision : History )
	{
		// look for the the SHA1 id of the file, not the commit id (revision)
		TSharedRef< FVSPGitRevision, ESPMode::ThreadSafe > GitRevision = StaticCastSharedRef< FVSPGitRevision >( Revision );
		if ( GitRevision->FileHash == PendingMergeBaseFileHash )
			return Revision;
	}

	return nullptr;
}

FString FVSPGitState::GetIconNameInternal() const
{
	if ( bUsingGitLfsLocking )
	{
		if ( bIsLocked )
		{
			const bool LockByMe = VSPGitHelpers::IsLockByMe( LockUser );
			if ( LockByMe )
			{
				if ( IsModified() )
					return TEXT( "Subversion.CheckedOutByOtherUserOtherBranch" );
				return TEXT( "Subversion.CheckedOutByOtherUser" );
			}

			return TEXT( "Level.LockedIcon16x" );
		}

		if ( State == EGitState::Modified )
			return TEXT( "Subversion.NotInDepot" );
	}
	if ( !IsCurrent() )
		return TEXT( "Subversion.NotAtHeadRevision" );

	switch ( State )
	{
		case EGitState::NotControlled:
			return TEXT( "Subversion.NotInDepot" );
		case EGitState::Added:
			return TEXT( "Subversion.OpenForAdd" );
		case EGitState::Conflicted:
			return TEXT( "Subversion.ModifiedOtherBranch" );
		case EGitState::Modified:
			return TEXT( "Subversion.CheckedOut" );
		case EGitState::Renamed:
		case EGitState::Copied:
			return TEXT( "Subversion.Branched" );
		case EGitState::Deleted:
		case EGitState::Missing:
			return TEXT( "Subversion.MarkedForDelete" );
		default:
			return FString();
	}
}

FName FVSPGitState::GetIconName() const
{
	const FString Result = GetIconNameInternal();
	if ( Result.IsEmpty() )
		return NAME_None;
	return FName( *Result );
}

FName FVSPGitState::GetSmallIconName() const
{
	FString Result = GetIconNameInternal();
	if ( Result.IsEmpty() )
		return NAME_None;
	if ( Result.Contains( TEXT( "Private" ) ) )
		Result += ".Small";
	else
		Result += "_Small";
	return FName( *Result );
}

FText FVSPGitState::GetDisplayName() const
{
	if ( bUsingGitLfsLocking )
	{
		if ( bIsLocked )
		{
			const bool LockByMe = VSPGitHelpers::IsLockByMe( LockUser );
			if ( LockByMe )
				return FText::FromString( "Locked by current user" );

			return FText::Format( FText::FromString( "Locked by " ), FText::FromString( LockUser ) );
		}

		if ( State == EGitState::Modified )
			return FText::FromString( "Locked and Modified by current user" );
	}
	if ( !IsCurrent() )
		return FText::FromString( "Not current" );

	switch ( State )
	{
		case EGitState::Added:
			return FText::FromString( "Added" );
		case EGitState::Deleted: // PS. There is no file in WC
			return FText::FromString( "Deleted" );
		case EGitState::Missing:
			return FText::FromString( "Missing" );
		case EGitState::Modified:
			return FText::FromString( "Modified" );
		case EGitState::Renamed:
			return FText::FromString( "Renamed" );
		case EGitState::Copied:
			return FText::FromString( "Copied" );
		case EGitState::Conflicted:
			return FText::FromString( "Contents Conflict" );
		case EGitState::NotControlled:
			return FText::FromString( "Not Under Source Control" );
		case EGitState::Ignored:
			return FText::FromString( "Ignored" );
		case EGitState::Unchanged:
			return FText::FromString( "Unchanged" );
		case EGitState::Unknown:
			return FText::FromString( "Unknown" );
	}

	return FText();
}

FText FVSPGitState::GetDisplayTooltip() const
{
	if ( bUsingGitLfsLocking )
	{
		if ( bIsLocked )
		{
			const bool LockByMe = VSPGitHelpers::IsLockByMe( LockUser );
			if ( LockByMe )
				return FText::FromString( "Locked for editing by current user" );

			return FText::Format( FText::FromString( "Locked for editing by: {0}" ), FText::FromString( LockUser ) );
		}

		if ( State == EGitState::Modified )
			return FText::FromString( "Locked and Modified by current user" );
	}
	if ( !IsCurrent() )
		return FText::FromString( "The file(s) are not at the head revision" );

	switch ( State )
	{
		case EGitState::Unknown:
			return FText::FromString( "Unknown source control state" );
		case EGitState::Unchanged:
			return FText::FromString( "There are no modifications" );
		case EGitState::Added:
			return FText::FromString( "Item is scheduled for addition" );
		case EGitState::Deleted:
			return FText::FromString( "Item is scheduled for deletion" );
		case EGitState::Modified:
			return FText::FromString( "Item has been modified" );
		case EGitState::Renamed:
			return FText::FromString( "Item has been renamed" );
		case EGitState::Copied:
			return FText::FromString( "Item has been copied" );
		case EGitState::Conflicted:
			return FText::FromString( "The contents of the item conflict with updates received from the repository." );
		case EGitState::Ignored:
			return FText::FromString( "Item is being ignored." );
		case EGitState::NotControlled:
			return FText::FromString( "Item is not under version control." );
		case EGitState::Missing:
			return FText::FromString(
				"Item is missing (e.g., you moved or deleted it without using Git). This also indicates that a directory is incomplete (a checkout or update was interrupted)." );
	}

	return FText();
}

const FString& FVSPGitState::GetFilename() const
{
	return FileName;
}

const FDateTime& FVSPGitState::GetTimeStamp() const
{
	return TimeStamp;
}

bool FVSPGitState::CanCheckIn() const
{
	if ( bUsingGitLfsLocking )
	{
		const bool bLockByMe = VSPGitHelpers::IsLockByMe( LockUser );
		return ( ( bIsLocked && bLockByMe && IsModified() && !IsConflicted() ) || ( State == EGitState::Added ) ) && IsCurrent();
	}

	return ( State == EGitState::Added
		|| State == EGitState::Deleted
		|| State == EGitState::Missing
		|| State == EGitState::Modified
		|| State == EGitState::Renamed ) && IsCurrent();
}

bool FVSPGitState::CanCheckout() const
{
	if ( bUsingGitLfsLocking )
		return ( State == EGitState::Unchanged || State == EGitState::Modified ) && !bIsLocked && IsCurrent();

	return false; // With Git all tracked files in the working copy are always already checked-out (as opposed to Perforce)
}

bool FVSPGitState::IsCheckedOut() const
{
	if ( bUsingGitLfsLocking )
	{
		const bool bLockByMe = VSPGitHelpers::IsLockByMe( LockUser );

		return bIsLocked && bLockByMe;
	}

	return IsSourceControlled();
}

bool FVSPGitState::IsCheckedOutOther( FString* Who ) const
{
	if ( bUsingGitLfsLocking )
	{
		if ( Who != nullptr )
			*Who = LockUser;
		const bool bLockByMe = VSPGitHelpers::IsLockByMe( LockUser );
		return bIsLocked && !bLockByMe;
	}

	return false;
}

bool FVSPGitState::IsCheckedOutInOtherBranch( const FString& CurrentBranch ) const
{
	return false;
}

bool FVSPGitState::IsModifiedInOtherBranch( const FString& CurrentBranch ) const
{
	return false;
}

bool FVSPGitState::IsCheckedOutOrModifiedInOtherBranch( const FString& CurrentBranch ) const
{
	return IsCheckedOutInOtherBranch( CurrentBranch ) || IsModifiedInOtherBranch( CurrentBranch );
}

bool FVSPGitState::GetOtherBranchHeadModification( FString& HeadBranchOut, FString& ActionOut, int32& HeadChangeListOut ) const
{
	return false;
}

TArray< FString > FVSPGitState::GetCheckedOutBranches() const
{
	return TArray< FString >();
}

FString FVSPGitState::GetOtherUserBranchCheckedOuts() const
{
	return FString();
}

bool FVSPGitState::IsCurrent() const
{
	return !bNewerVersionOnServer;
}

bool FVSPGitState::IsSourceControlled() const
{
	return State != EGitState::NotControlled && State != EGitState::Ignored && State != EGitState::Unknown;
}

bool FVSPGitState::IsAdded() const
{
	return State == EGitState::Added;
}

bool FVSPGitState::IsDeleted() const
{
	return State == EGitState::Deleted || State == EGitState::Missing;
}

bool FVSPGitState::IsIgnored() const
{
	return State == EGitState::Ignored;
}

bool FVSPGitState::CanEdit() const
{
	return !IsCheckedOutOther() && IsCurrent();
}

bool FVSPGitState::CanDelete() const
{
	return !IsCheckedOutOther() && IsSourceControlled() && IsCurrent();
}

bool FVSPGitState::IsUnknown() const
{
	return State == EGitState::Unknown;
}

bool FVSPGitState::IsModified() const
{
	//  - Unknown
	//  - Unchanged
	//  - NotControlled
	//  - Ignored
	return State == EGitState::Added
		|| State == EGitState::Deleted
		|| State == EGitState::Modified
		|| State == EGitState::Renamed
		|| State == EGitState::Copied
		|| State == EGitState::Missing
		|| State == EGitState::Conflicted;
}

bool FVSPGitState::CanAdd() const
{
	return State == EGitState::NotControlled;
}

bool FVSPGitState::IsConflicted() const
{
	return State == EGitState::Conflicted;
}

bool FVSPGitState::CanRevert() const
{
	return IsModified();
}
