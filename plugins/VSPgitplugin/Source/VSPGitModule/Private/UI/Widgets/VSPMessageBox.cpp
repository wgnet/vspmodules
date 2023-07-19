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
#include "VSPMessageBox.h"
#include "EditorStyle.h"
#include "UI/VSPGitStyle.h"
#include "VSPGitModule.h"

void SMessageBoxWidget::Construct( const FArguments& InArgs )
{
	ParentFrame = InArgs._ParentWindow.Get();
	Message = InArgs._Message.Get();
	Buttons = InArgs._MessageBoxButton.Get();
	Image = InArgs._MessageBoxImage.Get();

	auto MessageTextBlock = SNew( STextBlock )
		.AutoWrapText( true )
		.Text( FText::FromString( Message ) );

	TSharedPtr< SImage > MessageImage;
	switch ( Image )
	{
		case Error:
			MessageImage = SNew( SImage ).Image( FVSPGitStyle::Get().GetBrush( "MessageBox.Error" ) );
			break;
		case Question:
			MessageImage = SNew( SImage ).Image( FVSPGitStyle::Get().GetBrush( "MessageBox.Question" ) );
			break;
		case Warning:
			MessageImage = SNew( SImage ).Image( FVSPGitStyle::Get().GetBrush( "MessageBox.Warning" ) );
			break;
		default:
			MessageImage = SNew( SImage ).Image( FVSPGitStyle::Get().GetBrush( "MessageBox.Info" ) );
	}

	TSharedPtr< SUniformGridPanel > ButtonsGridPanel = SNew( SUniformGridPanel )
		.SlotPadding( FEditorStyle::GetMargin( "StandardDialog.SlotPadding" ) )
		.MinDesiredSlotWidth( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotWidth" ) )
		.MinDesiredSlotHeight( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotHeight" ) );


	ChildSlot
	[
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			.Padding( 5 )
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.MaxWidth( 128.f )
				.Padding( 10 )
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				[
					SNew( SVerticalBox )
					+ SVerticalBox::Slot()
					.MaxHeight( 64.f )
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.MaxWidth( 64.f )
						[
							MessageImage.ToSharedRef()
						]
					]
				]
				+ SHorizontalBox::Slot()
				.Padding( 5 )
				.FillWidth( 2.5f )
				.VAlign( VAlign_Center )
				[
					/*SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
					[*/
					MessageTextBlock
					//]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Bottom )
			.Padding( 0.0f, 0.0f, 0.0f, 5.0f )
			[
				SNew( SUniformGridPanel ).SlotPadding( FEditorStyle::GetMargin( "StandardDialog.SlotPadding" ) )
				.MinDesiredSlotWidth( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotWidth" ) )
				.MinDesiredSlotHeight( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotHeight" ) )
				.Visibility( Buttons == OkButton ? EVisibility::Visible : EVisibility::Collapsed )
				+ SUniformGridPanel::Slot( 0, 0 )
				[
					SNew( SSpacer )
				]
				+ SUniformGridPanel::Slot( 1, 0 )
				[
					SNew( SSpacer )
				]
				+ SUniformGridPanel::Slot( 2, 0 )
				[
					SNew( SSpacer )
				]
				+ SUniformGridPanel::Slot( 3, 0 )
				[
					SNew( SButton )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					//.Visibility(Buttons == OkCancelButtons || Buttons == OkButton ? EVisibility::Visible : EVisibility::Collapsed)
					.Text( NSLOCTEXT( "VSPSourceControl.MessageBox", "OkText", "Ok" ) )
					.OnClicked( this, &SMessageBoxWidget::OkButtonClicked )
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Bottom )
			.Padding( 0.0f, 0.0f, 0.0f, 5.0f )
			[
				SNew( SUniformGridPanel ).SlotPadding( FEditorStyle::GetMargin( "StandardDialog.SlotPadding" ) )
				.MinDesiredSlotWidth( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotWidth" ) )
				.MinDesiredSlotHeight( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotHeight" ) )
				.Visibility( Buttons == OkCancelButtons ? EVisibility::Visible : EVisibility::Collapsed )
				+ SUniformGridPanel::Slot( 0, 0 )
				[
					SNew( SSpacer )
				]
				+ SUniformGridPanel::Slot( 1, 0 )
				[
					SNew( SSpacer )
				]
				+ SUniformGridPanel::Slot( 2, 0 )
				[
					SNew( SButton )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					.Text( NSLOCTEXT( "VSPSourceControl.MessageBox", "OkText", "Ok" ) )
					.OnClicked( this, &SMessageBoxWidget::OkButtonClicked )
				]
				+ SUniformGridPanel::Slot( 3, 0 )
				[
					SNew( SButton )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					.Text( NSLOCTEXT( "VSPSourceControl.MessageBox", "CancelText", "Cancel" ) )
					.OnClicked( this, &SMessageBoxWidget::CancelButtonClicked )
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Bottom )
			.Padding( 0.0f, 0.0f, 0.0f, 5.0f )
			[
				SNew( SUniformGridPanel ).SlotPadding( FEditorStyle::GetMargin( "StandardDialog.SlotPadding" ) )
				.MinDesiredSlotWidth( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotWidth" ) )
				.MinDesiredSlotHeight( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotHeight" ) )
				.Visibility( Buttons == YesNoButtons ? EVisibility::Visible : EVisibility::Collapsed )
				+ SUniformGridPanel::Slot( 0, 0 )
				[
					SNew( SSpacer )
				]
				+ SUniformGridPanel::Slot( 1, 0 )
				[
					SNew( SSpacer )
				]
				+ SUniformGridPanel::Slot( 2, 0 )
				[
					SNew( SButton )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					.Text( NSLOCTEXT( "VSPSourceControl.MessageBox", "YesText", "Yes" ) )
					.OnClicked( this, &SMessageBoxWidget::YesButtonClicked )
				]
				+ SUniformGridPanel::Slot( 3, 0 )
				[
					SNew( SButton )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					.Text( NSLOCTEXT( "VSPSourceControl.MessageBox", "NoText", "No" ) )
					.OnClicked( this, &SMessageBoxWidget::NoButtonClicked )
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Bottom )
			.Padding( 0.0f, 0.0f, 0.0f, 5.0f )
			[
				SNew( SUniformGridPanel ).SlotPadding( FEditorStyle::GetMargin( "StandardDialog.SlotPadding" ) )
				.MinDesiredSlotWidth( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotWidth" ) )
				.MinDesiredSlotHeight( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotHeight" ) )
				.Visibility( Buttons == YesNoCancelButtons ? EVisibility::Visible : EVisibility::Collapsed )
				+ SUniformGridPanel::Slot( 0, 0 )
				[
					SNew( SSpacer )
				]
				+ SUniformGridPanel::Slot( 1, 0 )
				[
					SNew( SButton )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					.Text( NSLOCTEXT( "VSPSourceControl.MessageBox", "YesText", "Yes" ) )
					.OnClicked( this, &SMessageBoxWidget::YesButtonClicked )
				]
				+ SUniformGridPanel::Slot( 2, 0 )
				[
					SNew( SButton )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					.Text( NSLOCTEXT( "VSPSourceControl.MessageBox", "NoText", "No" ) )
					.OnClicked( this, &SMessageBoxWidget::NoButtonClicked )
				]
				+ SUniformGridPanel::Slot( 3, 0 )
				[
					SNew( SButton )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					.Text( NSLOCTEXT( "VSPSourceControl.MessageBox", "CancelText", "Cancel" ) )
					.OnClicked( this, &SMessageBoxWidget::CancelButtonClicked )
				]
			]
		]
	];
}

EMessageBoxResult SMessageBoxWidget::GetDialogResult() const
{
	return DialogResult;
}

FReply SMessageBoxWidget::OkButtonClicked()
{
	DialogResult = Ok;
	ParentFrame.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SMessageBoxWidget::CancelButtonClicked()
{
	DialogResult = Cancel;
	ParentFrame.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SMessageBoxWidget::YesButtonClicked()
{
	DialogResult = Yes;
	ParentFrame.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}

FReply SMessageBoxWidget::NoButtonClicked()
{
	DialogResult = No;
	ParentFrame.Pin()->RequestDestroyWindow();
	return FReply::Handled();
}

EMessageBoxResult FVSPMessageBox::ShowDialog(
	const FString& InMessage,
	const FString& InCaption,
	const EMessageBoxButton& InMessageBoxButton,
	const EMessageBoxImage& InMessageBoxImage,
	const FVector2D& InSize )
{
	TSharedRef< SWindow > MessageBoxWindow = SNew( SWindow )
		.Title( FText::FromString( InCaption ) )
		.ClientSize( InSize )
		.SupportsMaximize( false )
		.SupportsMinimize( false );

	const TSharedRef< SMessageBoxWidget > MessageBoxWidget = SNew( SMessageBoxWidget )
		.ParentWindow( MessageBoxWindow )
		.Message( InMessage )
		.MessageBoxButton( InMessageBoxButton )
		.MessageBoxImage( InMessageBoxImage );

	MessageBoxWindow->SetContent( MessageBoxWidget );

	FSlateApplication::Get().AddModalWindow( MessageBoxWindow, nullptr );

	return MessageBoxWidget->GetDialogResult();
}
