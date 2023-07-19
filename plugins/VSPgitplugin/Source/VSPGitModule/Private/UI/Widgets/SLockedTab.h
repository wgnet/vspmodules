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

#include "ISourceControlProvider.h"
#include "SCheckingFileStatesTree.h"
#include "SlateBasics.h"
#include "UI/CheckingFileStateItem.h"

class SLockedTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SLockedTab )
	{}
	SLATE_END_ARGS()

	SLockedTab();
	~SLockedTab();

	void Construct( const FArguments& InArgs );

	void SourceControlStateChanged_Handle();

	FText GetUnlockButtonText() const;
	bool GetUnlockButtonEnable() const;
	EVisibility GetUnlockBusyIndicatorVisibility() const;
	EVisibility GetLockedStateBusyIndicatorVisibility() const;

	void OnForceCheckedStateChanged( ECheckBoxState InCheckBoxState );
	ECheckBoxState GetForceCheckBoxState() const;

	void PromptForUnlock();
	FReply UnlockClicked();

private:
	TSharedPtr< SCheckingFileStatesTree > LockedTreePtr;

	TArray< TSharedPtr< FCheckingTreeItemBase > > LockedTreeData;

	bool bIsUnlockingBusy = false;
	bool bForce = false;

	FDelegateHandle SourceControlStateChanged;
	void OnSourceControlOperationComplete(const FSourceControlOperationRef& InOperation,
		ECommandResult::Type InResult);
};
