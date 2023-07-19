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
#include "SVSPGitRepoSettings.h"
// Engine headers
#include "EditorStyle.h"
#include "EditorDirectories.h"
#include "Widgets/Input/SFilePathPicker.h"
// Module headers
#include "Core/VSPGitProvider.h"

void SVSPGitRepoSettings::Construct( const FArguments& InArgs )
{
	const FSlateFontInfo Font = FEditorStyle::GetFontStyle( TEXT( "SourceControl.LoginWindow.Font" ) );

	const FText FileFilterType = FText::FromString( "Executables" );
	const FString FileFilterText = FString::Printf( TEXT( "%s (*.exe)|*.exe" ), *FileFilterType.ToString() );

	ChildSlot
	[
		SNew( SBorder )
		.BorderImage( FEditorStyle::GetBrush( "DetailsView.CategoryBottom" ) )
		.Padding( FMargin( 0.0f, 3.0f, 0.0f, 0.0f ) )
		[
			SNew( SVerticalBox )
			// Path to the Git command line executable
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 2.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SHorizontalBox )
				.ToolTipText( FText::FromString( "Path to Git binary" ) )
				+ SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				[
					SNew( STextBlock )
					.Text( FText::FromString( "Git Path" ) )
					.Font( Font )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 2.0f )
				[
					SNew( SFilePathPicker )
					.BrowseButtonImage( FEditorStyle::GetBrush( "PropertyWindow.Button_Ellipsis" ) )
					.BrowseButtonStyle( FEditorStyle::Get(), "HoverHintOnly" )
#if WITH_EDITOR
					.BrowseDirectory( FEditorDirectories::Get().GetLastDirectory( ELastDirectory::GENERIC_OPEN ) )
#endif
					.BrowseButtonToolTip( FText::FromString( "Path to Git binary" ) )
					.BrowseTitle( FText::FromString( "File picker..." ) )
					.FilePath( this, &SVSPGitRepoSettings::GetBinaryPathString )
					.FileTypeFilter( FileFilterText )
					.OnPathPicked( this, &SVSPGitRepoSettings::OnBinaryPathPicked )
				]
			]
			// Git version & features
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 2.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SHorizontalBox )
				.ToolTipText( FText::FromString( "Git version" ) )
				+ SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				[
					SNew( STextBlock )
					.Text( FText::FromString( "Git version" ) )
					.Font( Font )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 2.0f )
				[
					SNew( STextBlock )
					.Text( this, &SVSPGitRepoSettings::GetGitVersion )
					.Font( Font )
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 2.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SHorizontalBox )
				.ToolTipText( FText::FromString( "Git available LFS features" ) )
				+ SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				[
					SNew( STextBlock )
					.Text( FText::FromString( "Git LFS version" ) )
					.Font( Font )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 2.0f )
				[
					SNew( STextBlock )
					.Text( this, &SVSPGitRepoSettings::GetGitLfsVersion )
					.Font( Font )
				]
			]
			// Repository root
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 2.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SHorizontalBox )
				.ToolTipText( FText::FromString( "Path to the root of the Git repository" ) )
				+ SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				[
					SNew( STextBlock )
					.Text( FText::FromString( "Repository root" ) )
					.Font( Font )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 2.0f )
				[
					SNew( STextBlock )
					.Text( this, &SVSPGitRepoSettings::GetRepoRoot )
					.Font( Font )
				]
			]
			// Server params
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 2.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SHorizontalBox )
				.ToolTipText( FText::FromString( "Remote server alias and URL" ) )
				+ SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				[
					SNew( STextBlock )
					.Text( FText::FromString( "Server" ) )
					.Font( Font )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 2.0f )
				[
					SNew( STextBlock )
					.Text( this, &SVSPGitRepoSettings::GetServerData )
					.Font( Font )
				]
			]
			// Branch params
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 2.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SHorizontalBox )
				.ToolTipText( FText::FromString( "Local Branch name and upstream branch (if exist)" ) )
				+ SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				[
					SNew( STextBlock )
					.Text( FText::FromString( "Branch" ) )
					.Font( Font )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 2.0f )
				[
					SNew( STextBlock )
					.Text( this, &SVSPGitRepoSettings::GetBranchData )
					.Font( Font )
				]
			]
			// User Name <EMail>
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 2.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SHorizontalBox )
				.ToolTipText( FText::FromString( "User name & email configured for the Git repository" ) )
				+ SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				[
					SNew( STextBlock )
					.Text( FText::FromString( "User Name & Email" ) )
					.Font( Font )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 2.0f )
				[
					SNew( STextBlock )
					.Text( this, &SVSPGitRepoSettings::GetUserData )
					.Font( Font )
				]
			]
			// ----
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding( 2.0f )
			.VAlign( VAlign_Center )
			[
				SNew( SSeparator )
			]
			// Explanation text
			/*
            + SVerticalBox::Slot()
            .FillHeight(1.0f)
            .Padding(2.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                .Visibility(this, &SVSPGitRepoSettings::MustInitializeGitRepository)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                .HAlign(HAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Current Project is not contained in a Git Repository. Fill the form below to initialize a new Repository."))
                    .ToolTipText(FText::FromString("No Repository found at the level or above the current Project"))
                    .Font(Font)
                ]
            ]
            // Option to configure the URL of the default remote 'origin'
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                .Visibility(this, &SVSPGitSourceControlSettings::MustInitializeGitRepository)
                .ToolTipText(FText::FromString("Configure the URL of the default remote 'origin'"))
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("URL of the remote server 'origin'"))
                    .Font(Font)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(2.0f)
                .VAlign(VAlign_Center)
                [
                    SNew(SEditableTextBox)
                    .Text(this, &SVSPGitSourceControlSettings::GetRemoteUrl)
                    .OnTextCommitted(this, &SVSPGitSourceControlSettings::OnRemoteUrlCommitted)
                    .Font(Font)
                ]
            ]
            // Option to add a proper .gitignore file (true by default)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                .Visibility(this, &SVSPGitSourceControlSettings::MustInitializeGitRepository)
                .ToolTipText(FText::FromString("Create and add a standard '.gitignore' file"))
                + SHorizontalBox::Slot()
                .FillWidth(0.1f)
                [
                    SNew(SCheckBox)
                    .IsChecked(ECheckBoxState::Checked)
                    .OnCheckStateChanged(this, &SVSPGitSourceControlSettings::OnCheckedCreateGitIgnore)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(2.9f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Add a .gitignore file"))
                .Font(Font)
                ]
            ]
            // Option to add a README.md file with custom content
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                .Visibility(this, &SVSPGitSourceControlSettings::MustInitializeGitRepository)
                .ToolTipText(FText::FromString("Add a README.md file"))
                + SHorizontalBox::Slot()
                .FillWidth(0.1f)
                [
                    SNew(SCheckBox)
                    .IsChecked(ECheckBoxState::Checked)
                    .OnCheckStateChanged(this, &SVSPGitSourceControlSettings::OnCheckedCreateReadme)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.9f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Add a basic README.md file"))
                    .Font(Font)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(2.0f)
                .Padding(2.0f)
                [
                    SNew(SMultiLineEditableTextBox)
                    .Text(this, &SVSPGitSourceControlSettings::GetReadmeContent)
                    .OnTextCommitted(this, &SVSPGitSourceControlSettings::OnReadmeContentCommitted)
                    .IsEnabled(this, &SVSPGitSourceControlSettings::GetAutoCreateReadme)
                    .SelectAllTextWhenFocused(true)
                    .Font(Font)
                ]
            ]
            // Option to add a proper .gitattributes file for Git LFS (false by default)
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                .Visibility(this, &SVSPGitSourceControlSettings::MustInitializeGitRepository)
                .ToolTipText(FText::FromString("Create and add a '.gitattributes' file to enable Git LFS for the whole 'Content/' directory (needs Git LFS extensions to be installed)."))
                + SHorizontalBox::Slot()
                .FillWidth(0.1f)
                [
                    SNew(SCheckBox)
                    .IsChecked(ECheckBoxState::Unchecked)
                    .OnCheckStateChanged(this, &SVSPGitSourceControlSettings::OnCheckedCreateGitAttributes)
                    .IsEnabled(this, &SVSPGitSourceControlSettings::CanInitializeGitLfs)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(2.9f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Add a .gitattributes file to enable Git LFS"))
                    .Font(Font)
                ]
            ]
            // Option to Make the initial Git commit with custom message
            + SVerticalBox::Slot()
            .AutoHeight()
            .Padding(2.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                .Visibility(this, &SVSPGitSourceControlSettings::MustInitializeGitRepository)
                .ToolTipText(FText::FromString("Make the initial Git Commit"))
                + SHorizontalBox::Slot()
                .FillWidth(0.1f)
                [
                    SNew(SCheckBox)
                    .IsChecked(ECheckBoxState::Checked)
                    .OnCheckStateChanged(this, &SVSPGitSourceControlSettings::OnCheckedInitialCommit)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(0.9f)
                .VAlign(VAlign_Center)
                [
                    SNew(STextBlock)
                    .Text(FText::FromString("Make the initial Git commit"))
                    .Font(Font)
                ]
                + SHorizontalBox::Slot()
                .FillWidth(2.0f)
                .Padding(2.0f)
                [
                    SNew(SMultiLineEditableTextBox)
                    .Text(this, &SVSPGitSourceControlSettings::GetInitialCommitMessage)
                    .OnTextCommitted(this, &SVSPGitSourceControlSettings::OnInitialCommitMessageCommitted)
                    .IsEnabled(this, &SVSPGitSourceControlSettings::GetAutoInitialCommit)
                    .SelectAllTextWhenFocused(true)
                    .Font(Font)
                ]
            ]
            // Button to initialize the project with Git, create .gitignore/.gitattributes files, and make the first commit)
            +SVerticalBox::Slot()
            .FillHeight(2.5f)
            .Padding(4.0f)
            .VAlign(VAlign_Center)
            [
                SNew(SHorizontalBox)
                .Visibility(this, &SVSPGitSourceControlSettings::MustInitializeGitRepository)
                + SHorizontalBox::Slot()
                .FillWidth(1.0f)
                [
                    SNew(SButton)
                    .Text(FText::FromString("Initialize project with Git"))
                    .ToolTipText(FText::FromString("Initialize current project as a new Git repository"))
                    .OnClicked(this, &SVSPGitSourceControlSettings::OnClickedInitializeGitRepository)
                    .IsEnabled(this, &SVSPGitSourceControlSettings::CanInitializeGitRepository)
                    .HAlign(HAlign_Center)
                    .ContentPadding(6)
                ]
            ]
			*/
		]
	];
}

FString SVSPGitRepoSettings::GetBinaryPathString() const
{
	return FVSPGitProvider::Get().GetSettings()->GetGitBinPath();
}

void SVSPGitRepoSettings::OnBinaryPathPicked( const FString& PickedPath ) const
{
	const FString PickedFullPath = FPaths::ConvertRelativePathToFull( PickedPath );
	const bool bChanged = FVSPGitProvider::Get().GetSettings()->SetGitBinPath( PickedPath );
	if ( bChanged )
		// Re-Check provided git binary path for each change
		FVSPGitProvider::AccessProvider().CheckGitAndRepository();
}

FText SVSPGitRepoSettings::GetGitVersion() const
{
	const TSharedRef< FVSPGitSettings, ESPMode::ThreadSafe > Settings = FVSPGitProvider::Get().GetSettings();

	FFormatNamedArguments Args;
	Args.Add( TEXT( "Major" ), Settings->GitVersion.Major );
	Args.Add( TEXT( "Minor" ), Settings->GitVersion.Minor );
	Args.Add( TEXT( "Patch" ), Settings->GitVersion.Patch );
	Args.Add( TEXT( "Win" ), Settings->GitVersion.Windows );

	return FText::Format( FText::FromString( "Git version {Major}.{Minor}.{Patch}({Win})" ), Args );
}

FText SVSPGitRepoSettings::GetGitLfsVersion() const
{
	const TSharedRef< FVSPGitSettings, ESPMode::ThreadSafe > Settings = FVSPGitProvider::Get().GetSettings();
	return FText::FromString( Settings->GitLfsVersion );
}

FText SVSPGitRepoSettings::GetRepoRoot() const
{
	const TSharedRef< FVSPGitSettings, ESPMode::ThreadSafe > Settings = FVSPGitProvider::Get().GetSettings();
	return FText::FromString( Settings->GetCurrentRepoSettings()->RepoRoot );
}

FText SVSPGitRepoSettings::GetBranchData() const
{
	const TSharedRef< FVSPGitSettings, ESPMode::ThreadSafe > Settings = FVSPGitProvider::Get().GetSettings();
	return FText::FromString(
		FString::Printf( TEXT( "%s -> %s" ), *Settings->GetCurrentRepoSettings()->LocalBranch, *Settings->GetCurrentRepoSettings()->UpstreamBranch ) );
}

FText SVSPGitRepoSettings::GetServerData() const
{
	const TSharedRef< FVSPGitSettings, ESPMode::ThreadSafe > Settings = FVSPGitProvider::Get().GetSettings();
	return FText::FromString(
		FString::Printf( TEXT( "%s >> %s" ), *Settings->GetCurrentRepoSettings()->RemoteName, *Settings->GetCurrentRepoSettings()->RemoteUrl ) );
}

FText SVSPGitRepoSettings::GetUserData() const
{
	const TSharedRef< FVSPGitSettings, ESPMode::ThreadSafe > Settings = FVSPGitProvider::Get().GetSettings();
	return FText::FromString(
		FString::Printf( TEXT( "%s <%s>" ), *Settings->GetCurrentRepoSettings()->UserName, *Settings->GetCurrentRepoSettings()->UserEmail ) );
}

void SVSPGitRepoSettings::OnSourceControlOperationComplete(
	const FSourceControlOperationRef& InOperation,
	ECommandResult::Type InResult )
{
	RemoveInProgressNotification();

	// Report result with a notification
	if ( InResult == ECommandResult::Succeeded )
		DisplaySuccessNotification( InOperation );
	else
		DisplayFailureNotification( InOperation );
	// May be needed in the future
	// if ((InOperation->GetName() == "MarkForAdd") && (InResult == ECommandResult::Succeeded) && bAutoInitialCommit)
	// {
	//     // 4. optional initial Asynchronous commit with custom message: launch a "CheckIn" Operation
	//     LaunchCheckInOperation();
	// }
}

void SVSPGitRepoSettings::DisplayInProgressNotification( const FSourceControlOperationRef& InOperation )
{
	FNotificationInfo Info( InOperation->GetInProgressString() );
	Info.bFireAndForget = false;
	Info.ExpireDuration = 0.0f;
	Info.FadeOutDuration = 1.0f;
	OperationInProgressNotification = FSlateNotificationManager::Get().AddNotification( Info );
	if ( OperationInProgressNotification.IsValid() )
		OperationInProgressNotification.Pin()->SetCompletionState( SNotificationItem::CS_Pending );
}

void SVSPGitRepoSettings::RemoveInProgressNotification()
{
	if ( OperationInProgressNotification.IsValid() )
	{
		OperationInProgressNotification.Pin()->ExpireAndFadeout();
		OperationInProgressNotification.Reset();
	}
}

void SVSPGitRepoSettings::DisplaySuccessNotification( const FSourceControlOperationRef& InOperation ) const
{
	const FText NotificationText = FText::FromString( FString( FText::FromName( InOperation->GetName() ).ToString() ) + " operation was successful!" );
	FNotificationInfo Info( NotificationText );
	Info.bUseSuccessFailIcons = true;
	Info.Image = FEditorStyle::GetBrush( TEXT( "NotificationList.SuccessImage" ) );
	FSlateNotificationManager::Get().AddNotification( Info );
}

void SVSPGitRepoSettings::DisplayFailureNotification( const FSourceControlOperationRef& InOperation ) const
{
	const FText NotificationText = FText::FromString( "Error: " + FString( FText::FromName( InOperation->GetName() ).ToString() ) + " operation failed!" );
	FNotificationInfo Info( NotificationText );
	Info.ExpireDuration = 8.0f;
	FSlateNotificationManager::Get().AddNotification( Info );
}
