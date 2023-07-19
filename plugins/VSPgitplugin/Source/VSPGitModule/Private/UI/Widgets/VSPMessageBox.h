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

enum EMessageBoxButton
{
	OkButton,
	OkCancelButtons,
	YesNoButtons,
	YesNoCancelButtons
};

enum EMessageBoxImage
{
	Error,
	Info,
	Question,
	Warning
};

enum EMessageBoxResult
{
	Ok,
	Yes,
	No,
	Cancel,
	Ignore
};

class SMessageBoxWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SMessageBoxWidget )
	{}
		SLATE_ATTRIBUTE( TSharedPtr<SWindow>, ParentWindow )
		SLATE_ATTRIBUTE( FString, Message )
		SLATE_ATTRIBUTE( EMessageBoxButton, MessageBoxButton )
		SLATE_ATTRIBUTE( EMessageBoxImage, MessageBoxImage )
	SLATE_END_ARGS()

	SMessageBoxWidget()
		: Buttons( OkCancelButtons )
		, Image( Info )
		, DialogResult( Ignore )
	{}

	void Construct( const FArguments& InArgs );

	EMessageBoxResult GetDialogResult() const;

private:
	FReply OkButtonClicked();
	FReply CancelButtonClicked();
	FReply YesButtonClicked();
	FReply NoButtonClicked();

private:
	TWeakPtr< SWindow > ParentFrame;

	EMessageBoxButton Buttons;
	EMessageBoxImage Image;
	FString Message;

	EMessageBoxResult DialogResult;
};

class FVSPMessageBox
{
public:
	static EMessageBoxResult ShowDialog(
		const FString& InMessage,
		const FString& InCaption,
		const EMessageBoxButton& InMessageBoxButton = OkCancelButtons,
		const EMessageBoxImage& InMessageBoxImage = Info,
		const FVector2D& InSize = FVector2D( 400, 200 ) );
};
