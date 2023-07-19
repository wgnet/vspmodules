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

#include "SCheckingFileStatesTree.h"
#include "SlateBasics.h"
#include "Core/VSPGitProvider.h"

class SChangesTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SChangesTab )
	{}
	SLATE_END_ARGS()

	SChangesTab();
	~SChangesTab();

	void Construct( const FArguments& InArgs );

	FText GetCommitToText() const;
	FText GetCommitHintText() const;

	void PromptForCommit();
private:
	EVisibility GetStatusBusyIndicatorVisibility() const;
	EVisibility GetCommitBusyIndicatorVisibility() const;

	void SourceControlStateChanged_Handle();

	FReply CommitClicked();

	bool GetCommitButtonEnable() const;

private:
	TSharedPtr< SCheckingFileStatesTree > ChangesTreePtr;
	TSharedPtr< SEditableTextBox > CommitMessageEditableTextBoxPtr;
	TSharedPtr< SMultiLineEditableTextBox > CommitDescriptionMultiLineEditableTextBoxPtr;

	TArray< TSharedPtr< FCheckingTreeItemBase > > ChangesTreeData;

	bool bIsCommitBusy = false;
	FDelegateHandle SourceControlStateChanged;
};
