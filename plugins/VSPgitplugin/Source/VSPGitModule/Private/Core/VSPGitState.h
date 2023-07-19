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
#include "CoreMinimal.h"
#include "ISourceControlState.h"
#include "ISourceControlRevision.h"

namespace EGitState
{
	enum Type
	{
		Unknown,
		Unchanged,
		Added,
		Deleted,
		Modified,
		Renamed,
		Copied,
		Missing,
		Conflicted,
		NotControlled,
		Ignored,
	};
}

class FVSPGitState : public ISourceControlState, public TSharedFromThis< FVSPGitState, ESPMode::ThreadSafe >
{
public:
	FVSPGitState( const FString InRelativePath, const EGitState::Type InState );

	virtual int32 GetHistorySize() const override;
	virtual TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > GetHistoryItem( int32 HistoryIndex ) const override;
	virtual TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > FindHistoryRevision( int32 RevisionNumber ) const override;
	virtual TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > FindHistoryRevision( const FString& InRevision ) const override;
	virtual TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > GetBaseRevForMerge() const override;
	FString GetIconNameInternal() const;
	virtual FName GetIconName() const override;
	virtual FName GetSmallIconName() const override;
	virtual FText GetDisplayName() const override;
	virtual FText GetDisplayTooltip() const override;
	virtual const FString& GetFilename() const override;
	virtual const FDateTime& GetTimeStamp() const override;
	virtual bool CanCheckIn() const override;
	virtual bool CanCheckout() const override;
	virtual bool IsCheckedOut() const override;
	virtual bool IsCheckedOutOther( FString* Who = nullptr ) const override;
	virtual bool IsCheckedOutInOtherBranch( const FString& CurrentBranch ) const override;
	virtual bool IsModifiedInOtherBranch( const FString& CurrentBranch ) const override;
	virtual bool IsCheckedOutOrModifiedInOtherBranch( const FString& CurrentBranch ) const override;
	virtual bool GetOtherBranchHeadModification( FString& HeadBranchOut, FString& ActionOut, int32& HeadChangeListOut ) const override;
	virtual TArray< FString > GetCheckedOutBranches() const override;
	virtual FString GetOtherUserBranchCheckedOuts() const override;
	virtual bool IsCurrent() const override;
	virtual bool IsSourceControlled() const override;
	virtual bool IsAdded() const override;
	virtual bool IsDeleted() const override;
	virtual bool IsIgnored() const override;
	virtual bool CanEdit() const override;
	virtual bool CanDelete() const override;
	virtual bool IsUnknown() const override;
	virtual bool IsModified() const override;
	virtual bool CanAdd() const override;
	virtual bool IsConflicted() const override;
	virtual bool CanRevert() const override;

public:
	/** Filename on disk */
	FString FileName;
	/** The timestamp of the last update */
	FDateTime TimeStamp;
	/** Current git state */
	EGitState::Type State;

	/** Has ReadOnly attribute (mark as lockable) */
	bool bUsingGitLfsLocking;
	/** Locked File Label */
	bool bIsLocked;
	/** Locked user name */
	FString LockUser;
	/** Locked Id */
	int32 LockId;
	TArray< TSharedRef< ISourceControlRevision, ESPMode::ThreadSafe > > History;

	/** Whether a newer version exists on the server */
	bool bNewerVersionOnServer;

	/** File Id with which our local revision diverged from the remote revision */
	FString PendingMergeBaseFileHash;
};
