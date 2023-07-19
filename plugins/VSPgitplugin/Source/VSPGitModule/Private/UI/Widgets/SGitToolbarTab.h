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

#include "SlateBasics.h"

enum ESmartButtonState
{
	NoUpstreamBranch,
	NeedPull,
	NeedPush,
	AlreadyUpToDate
};

class SGitToolbarTab : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SGitToolbarTab )
	{}
	SLATE_END_ARGS()

	void Construct( const FArguments& InArgs );

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
private:
	FText GetLocalBranch() const;
	FText GetRemoteBranch() const;

	bool IsBusy() const;
	bool IsNotBusy() const;

	EVisibility GetBusyVisibility() const;
	EVisibility GetNotBusyVisibility() const;

	int32 GetAheadNum() const;
	FText GetAheadNumText() const;
	EVisibility GetAheadVisibility() const;

	int32 GetBehindNum() const;
	FText GetBehindNumText() const;
	EVisibility GetBehindVisibility() const;

	const FSlateBrush* GetSmartButtonIcon() const;

	FText GetBusyStepString() const;
	FText GetBusyProcessString() const;

	FText GetSmartStepString() const;
	FText GetLastFetchedString() const;

	FReply SmartButtonClicked();

private:
	ESmartButtonState SmartButtonState = AlreadyUpToDate;
};
