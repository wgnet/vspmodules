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

#include "Slate.h"

struct FVSPGitPullParamsResultInfo
{
	FVSPGitPullParamsResultInfo()
		: bDialogResult( false )
		, bFFOnlyParam( false )
		, bFFOnlyWithForceParam( false )
		, bViaRebase( true )
	{}

	bool bDialogResult;
	bool bFFOnlyParam;
	bool bFFOnlyWithForceParam;
	bool bViaRebase;
};

class SVSPGitPullParamsWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SVSPGitPullParamsWidget )
	{}
		SLATE_ATTRIBUTE( TSharedPtr<SWindow>, ParentWindow )
	SLATE_END_ARGS()

	SVSPGitPullParamsWidget()
	{}

	/** Constructs the widget */
	void Construct( const FArguments& InArgs );

	FVSPGitPullParamsResultInfo GetResultParams() const;

private:
	/** Called when the settings of the dialog are to be accepted*/
	FReply OKClicked();

	/** Called when the settings of the dialog are to be ignored*/
	FReply CancelClicked();

	void OnCheckStateChanged_FastForwardOnly( ECheckBoxState CheckBoxState );
	ECheckBoxState GetCheckedFastForward() const;

	void OnCheckStateChanged_ForceParam( ECheckBoxState CheckBoxState );
	ECheckBoxState GetCheckedForceParam() const;

	void OnCheckStateChanged_RebaseParam( ECheckBoxState CheckBoxState );
	ECheckBoxState GetCheckedRebaseParam() const;

	static bool CheckState( ECheckBoxState CheckBoxState );
private:
	/** Pointer to the parent modal window */
	TWeakPtr< SWindow > ParentFrame;

	FVSPGitPullParamsResultInfo ResultInfo;
};

class FVSPGitPullParamsWindow
{
public:
	static FVSPGitPullParamsResultInfo ShowDialog();
};
