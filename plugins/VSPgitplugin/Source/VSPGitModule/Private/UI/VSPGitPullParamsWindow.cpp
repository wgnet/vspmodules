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
#include "VSPGitPullParamsWindow.h"
#include "EditorStyle.h"

#define LOCTEXT_NAMESPACE "SSourceControlPull"

void SVSPGitPullParamsWidget::Construct( const FArguments& InArgs )
{
	ParentFrame = InArgs._ParentWindow.Get();

	ChildSlot
	[
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush( "ToolPanel.GroupBorder" ) )
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			//.AutoHeight()
			.FillHeight( .5f )
			.Padding( 5 )
			[
				SNew( SVerticalBox )
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 5 )
				[
					SNew( SCheckBox )
					.ToolTipText(
						NSLOCTEXT(
							"VSPSourceControl.PullPanel",
							"FFOnlyCheckBoxTooltip",
							"Refuse to merge and exit with a non-zero status unless the current HEAD is already up to date or the merge can be resolved as a fast-forward" ) )
					.OnCheckStateChanged( this, &SVSPGitPullParamsWidget::OnCheckStateChanged_FastForwardOnly )
					.IsChecked( this, &SVSPGitPullParamsWidget::GetCheckedFastForward )
					//.IsEnabled(this, &SSourceControlSubmitWidget::CanCheckOut)
					[
						SNew( STextBlock )
						.Text( NSLOCTEXT( "VSPSourceControl.PullPanel", "FFOnlyCheckBox", "FF-only" ) )
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 5 )
				[
					SNew( SCheckBox )
					.ToolTipText( NSLOCTEXT( "VSPSourceControl.PullPanel", "FFwithForceCheckBoxTooltip", "Ignore local changes" ) )
					.OnCheckStateChanged( this, &SVSPGitPullParamsWidget::OnCheckStateChanged_ForceParam )
					.IsChecked( this, &SVSPGitPullParamsWidget::GetCheckedForceParam )
					//.IsEnabled(this, &SVSPGitPullParamsWidget::CanCheckOut)
					[
						SNew( STextBlock )
						.Text( NSLOCTEXT( "VSPSourceControl.PullPanel", "FFwithForceCheckBox", "--force" ) )
					]
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.Padding( 5 )
				[
					SNew( SCheckBox )
					.ToolTipText(
						NSLOCTEXT(
							"VSPSourceControl.PullPanel",
							"ViaRebaseCheckBoxTooltip",
							"If the checkout is done via rebase, local submodule commits are rebased as well" ) )
					.OnCheckStateChanged( this, &SVSPGitPullParamsWidget::OnCheckStateChanged_RebaseParam )
					.IsChecked( this, &SVSPGitPullParamsWidget::GetCheckedRebaseParam )
					//.IsEnabled(this, &SSourceControlSubmitWidget::CanCheckOut)
					[
						SNew( STextBlock )
						.Text( NSLOCTEXT( "VSPSourceControl.PullPanel", "ViaRebaseCheckBox", "Via rebase" ) )
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign( HAlign_Right )
			.VAlign( VAlign_Bottom )
			.Padding( 0.0f, 0.0f, 0.0f, 5.0f )
			[
				SNew( SUniformGridPanel )
				.SlotPadding( FEditorStyle::GetMargin( "StandardDialog.SlotPadding" ) )
				.MinDesiredSlotWidth( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotWidth" ) )
				.MinDesiredSlotHeight( FEditorStyle::GetFloat( "StandardDialog.MinDesiredSlotHeight" ) )
				+ SUniformGridPanel::Slot( 0, 0 )
				[
					SNew( SButton )
					.HAlign( HAlign_Center )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					//.Text("Pull")
					.Text( NSLOCTEXT( "VSPSourceControl.PullPanel", "OkButton", "Pull" ) )
					.OnClicked( this, &SVSPGitPullParamsWidget::OKClicked )
				]
				+ SUniformGridPanel::Slot( 1, 0 )
				[
					SNew( SButton )
					.HAlign( HAlign_Center )
					.ContentPadding( FEditorStyle::GetMargin( "StandardDialog.ContentPadding" ) )
					//.Text("Cancel")
					.Text( NSLOCTEXT( "VSPSourceControl.PullPanel", "CancelButton", "Cancel" ) )
					.OnClicked( this, &SVSPGitPullParamsWidget::CancelClicked )
				]
			]
		]
	];
}

FVSPGitPullParamsResultInfo SVSPGitPullParamsWidget::GetResultParams() const
{
	return ResultInfo;
}

FReply SVSPGitPullParamsWidget::OKClicked()
{
	ResultInfo.bDialogResult = true;
	ParentFrame.Pin()->RequestDestroyWindow();

	return FReply::Handled();
}

FReply SVSPGitPullParamsWidget::CancelClicked()
{
	ResultInfo.bDialogResult = false;
	ParentFrame.Pin()->RequestDestroyWindow();

	return FReply::Handled();
}

void SVSPGitPullParamsWidget::OnCheckStateChanged_FastForwardOnly( ECheckBoxState CheckBoxState )
{
	ResultInfo.bFFOnlyParam = CheckState( CheckBoxState );
}

ECheckBoxState SVSPGitPullParamsWidget::GetCheckedFastForward() const
{
	return ResultInfo.bFFOnlyParam ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SVSPGitPullParamsWidget::OnCheckStateChanged_ForceParam( ECheckBoxState CheckBoxState )
{
	ResultInfo.bFFOnlyWithForceParam = CheckState( CheckBoxState );
}

ECheckBoxState SVSPGitPullParamsWidget::GetCheckedForceParam() const
{
	return ResultInfo.bFFOnlyWithForceParam ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SVSPGitPullParamsWidget::OnCheckStateChanged_RebaseParam( ECheckBoxState CheckBoxState )
{
	ResultInfo.bViaRebase = CheckState( CheckBoxState );
}

ECheckBoxState SVSPGitPullParamsWidget::GetCheckedRebaseParam() const
{
	return ResultInfo.bViaRebase ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

bool SVSPGitPullParamsWidget::CheckState( ECheckBoxState CheckBoxState )
{
	switch ( CheckBoxState )
	{
		case ECheckBoxState::Checked:
			return true;
		default:
			return false;
	}
}

FVSPGitPullParamsResultInfo FVSPGitPullParamsWindow::ShowDialog()
{
	TSharedRef< SWindow > GitPullParamsWindow = SNew( SWindow )
		.Title( FText::FromString( TEXT( "Git pull parameters" ) ) )
		.ClientSize( FVector2D( 240, 140 ) )
		.SupportsMaximize( false )
		.SupportsMinimize( false );

	const TSharedRef< SVSPGitPullParamsWidget > GitPullParamsWidget = SNew( SVSPGitPullParamsWidget )
		.ParentWindow( GitPullParamsWindow );

	GitPullParamsWindow->SetContent( GitPullParamsWidget );

	FSlateApplication::Get().AddModalWindow( GitPullParamsWindow, nullptr );

	return GitPullParamsWidget->GetResultParams();
}

#undef LOCTEXT_NAMESPACE
