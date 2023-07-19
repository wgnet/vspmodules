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
#include "Slate.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "ISourceControlOperation.h"
#include "ISourceControlProvider.h"

class SVSPGitRepoSettings : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SVSPGitRepoSettings )
	{}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

private:
	// git binaries
	FString GetBinaryPathString() const;
	void OnBinaryPathPicked( const FString& PickedPath ) const;
	FText GetGitVersion() const;
	FText GetGitLfsVersion() const;

	// repo data
	FText GetRepoRoot() const;
	FText GetBranchData() const;
	FText GetServerData() const;

	// user data
	FText GetUserData() const;

	// Repo not connected and need init repo
	// TODO: Is it Need?
	/*
	 EVisibility MustInitializeGitRepository() const;
	bool CanInitializeGitRepository() const;
	bool CanInitializeGitLfs() const;
	bool CanUseGitLfsLocking() const;

	void LaunchMarkForAddOperation(const TArray<FString>& InFiles);
	void LaunchCheckInOperation();
	*/

	/** Asynchronous operation progress notifications */
	TWeakPtr< SNotificationItem > OperationInProgressNotification;

	void OnSourceControlOperationComplete( const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult );
	void DisplayInProgressNotification( const FSourceControlOperationRef& InOperation );
	void RemoveInProgressNotification();
	void DisplaySuccessNotification( const FSourceControlOperationRef& InOperation ) const;
	void DisplayFailureNotification( const FSourceControlOperationRef& InOperation ) const;

	/*
	FReply OnClickedInitializeGitRepository();

	void OnLfsUserNameCommitted(const FText& InText, ETextCommit::Type InCommitType) const;
	FText GetLfsUserName() const;

	void OnCheckedCreateGitIgnore(ECheckBoxState NewCheckedState);
	bool bAutoCreateGitIgnore = true;

	void OnCheckedCreateReadme(ECheckBoxState NewCheckedState);
	bool GetAutoCreateReadme() const;
	bool bAutoCreateReadme = true;

	void OnReadmeContentCommitted(const FText& InText, ETextCommit::Type InCommitType);
	FText GetReadmeContent() const;
	FText ReadmeContent;

	void OnCheckedCreateGitAttributes(ECheckBoxState NewCheckedState);
	bool bAutoCreateGitAttributes = false;

	void OnCheckedInitialCommit(ECheckBoxState NewCheckedState);
	bool GetAutoInitialCommit() const;
	bool bAutoInitialCommit = true;

	void OnInitialCommitMessageCommitted(const FText& InText, ETextCommit::Type InCommitType);
	FText GetInitialCommitMessage() const;
	FText InitialCommitMessage;

	void OnRemoteUrlCommitted(const FText& InText, ETextCommit::Type InCommitType);
	FText GetRemoteUrl() const;
	FText RemoteUrl;
	*/
};
