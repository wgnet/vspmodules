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
#include "ISourceControlRevision.h"

class FVSPGitRevision : public ISourceControlRevision, public TSharedFromThis< FVSPGitRevision, ESPMode::ThreadSafe >
{
	/* ISourceControlRevision interface */
public:
	FVSPGitRevision()
		: CommitIdNumber( 0 )
		, RevisionNumber( 0 )
		, FileSize( 0 )
	{}

	virtual bool Get( FString& InOutFilename ) const override;
	virtual bool GetAnnotated( TArray< FAnnotationLine >& OutLines ) const override;
	virtual bool GetAnnotated( FString& InOutFilename ) const override;
	virtual const FString& GetFilename() const override;
	virtual const FString& GetRevision() const override;
	virtual int32 GetRevisionNumber() const override;
	virtual const FString& GetDescription() const override;
	virtual const FString& GetUserName() const override;
	virtual const FString& GetClientSpec() const override;
	virtual const FString& GetAction() const override;
	virtual TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > GetBranchSource() const override;
	virtual const FDateTime& GetDate() const override;
	virtual int32 GetCheckInIdentifier() const override;
	virtual int32 GetFileSize() const override;

public:
	/** The filename this revision refers to */
	FString Filename;
	/** The full hexadecimal SHA1 id of the commit this revision refers to */
	FString CommitId;
	/** The short hexadecimal SHA1 id (8 first hex char out of 40) of the commit: the string to display */
	FString ShortCommitId;
	/** The numeric value of the short SHA1 (8 first hex char out of 40) */
	int32 CommitIdNumber;
	/** The index of the revision in the history (SBlueprintRevisionMenu assumes order for the "Depot" label) */
	int32 RevisionNumber;
	/** The SHA1 identifier of the file at this revision */
	FString FileHash;
	/** The description of this revision */
	FString Description;
	/** The user that made the change */
	FString UserName;
	/** The action (add, edit, branch etc.) performed at this revision */
	FString Action;
	/** Source of move ("branch" in Perforce term) if any */
	TSharedPtr< ISourceControlRevision, ESPMode::ThreadSafe > BranchSource;
	/** The date this revision was made */
	FDateTime Date;
	/** The size of the file at this revision */
	int32 FileSize;
};
