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
#include "SGitToolbarTab.h"

#include "Core/VSPGitProvider.h"
#include "UI/VSPGitSlateLayout.h"
#include "UI/VSPGitStyle.h"

#define LOCTEXT_NAMESPACE "SVSPToolbarTab"

void SGitToolbarTab::Construct( const FArguments& InArgs )
{
	this->ChildSlot
		.HAlign( HAlign_Left )
		.VAlign( VAlign_Center )
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			.VAlign( VAlign_Center )
			.AutoWidth()
			.Padding( 12.f )
			[
				//SNew(SToolbarExtension)
				SNew( SImage )
				.Image( FVSPGitStyle::Get().GetBrush( "VSPGit.GitBranch" ) )
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SVerticalBox )
				+ SVerticalBox::Slot()
				.VAlign( VAlign_Bottom )
				.Padding( 2.f )
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( 2.f )
					[
						SNew( SBox )
						.WidthOverride( 100.f )
						[
							SNew( STextBlock )
							.Text( LOCTEXT( "RemoteBranch", "Upstream branch" ) )
							//.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8))
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( 2.f )
					[
						SNew( STextBlock )
						.Text_Raw( this, &SGitToolbarTab::GetRemoteBranch )
						.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 10 ) )
						.ColorAndOpacity( FLinearColor::White )
					]
				]
				+ SVerticalBox::Slot()
				.VAlign( VAlign_Top )
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( 2.f )
					[
						SNew( SBox )
						.WidthOverride( 100.f )
						[
							SNew( STextBlock )
							.Text( LOCTEXT( "CurrentBranch", "Current branch" ) )
							//.Font(FSlateFontInfo(FPaths::EngineContentDir() / TEXT("Slate/Fonts/Roboto-Bold.ttf"), 8))
						]
					]
					+ SHorizontalBox::Slot()
					.Padding( 2.f )
					.AutoWidth()
					[
						SNew( STextBlock )
						.Text_Raw( this, &SGitToolbarTab::GetLocalBranch )
						.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 10 ) )
						.ColorAndOpacity( FLinearColor::White )
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride( 20.f )
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 10.f, 5.f )
			[
				SNew( SSeparator )
				.Orientation( Orient_Vertical )
			]
			// Process plash
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SBox )
				.Visibility_Raw( this, &SGitToolbarTab::GetBusyVisibility )
				[
					SNew( SOverlay )
					+ SOverlay::Slot()
					.Padding( 10.f, 2.f )
					[
						SNew( SVerticalBox )
						+ SVerticalBox::Slot()
						.FillHeight( 0.8f )
						+ SVerticalBox::Slot()
						.FillHeight( 0.2f )
						[
							SNew( SProgressBar )
							//.Visibility_Raw(this, &SGitToolboxTab::GetFetchBusyVisibility)
						]
					]
					+ SOverlay::Slot()
					.Padding( 10.f, 2.f )
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.VAlign( VAlign_Center )
						.AutoWidth()
						[
							SNew( SSpinningImage )
							.Image( FVSPGitStyle::Get().GetBrush( "VSPGit.FetchSpinner" ) )
							//.Visibility_Raw(this, &SGitToolboxTab::GetFetchBusyVisibility)
						]
						+ SHorizontalBox::Slot()
						.Padding( 10.f, 2.f )
						[
							SNew( SVerticalBox )
							+ SVerticalBox::Slot()
							.VAlign( VAlign_Bottom )
							[
								SNew( STextBlock )
								.Text_Raw( this, &SGitToolbarTab::GetBusyStepString )
								.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 10 ) )
								.ColorAndOpacity( FLinearColor::White )
							]
							+ SVerticalBox::Slot()
							.Padding( 2.f )
							[
								SNew( STextBlock )
								.Text_Raw( this, &SGitToolbarTab::GetBusyProcessString )
							]
						]
					]
				]
			]
			// Smart button
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SButton )
				// Use the tool bar item style for this button
				.ButtonStyle( FEditorStyle::Get(), FName( "ToolBar.Button" ) )
				.ForegroundColor( FSlateColor::UseForeground() )
				.Visibility_Raw( this, &SGitToolbarTab::GetNotBusyVisibility )
				.OnClicked( this, &SGitToolbarTab::SmartButtonClicked )
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.VAlign( VAlign_Center )
					.AutoWidth()
					[
						SNew( SImage )
						//.Image(FVSPGitStyle::Get().GetBrush("VSPGit.FetchSpinner"))
						.Image_Raw( this, &SGitToolbarTab::GetSmartButtonIcon )
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( 10.f, 2.f, 2.f, 2.f )
					[
						SNew( SVerticalBox )
						+ SVerticalBox::Slot()
						.VAlign( VAlign_Bottom )
						.Padding( 2.f )
						[
							SNew( STextBlock )
							.Text_Raw( this, &SGitToolbarTab::GetSmartStepString )
							.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 10 ) )
							.ColorAndOpacity( FLinearColor::White )
						]
						+ SVerticalBox::Slot()
						.Padding( 2.f )
						[
							SNew( STextBlock )
							.Text_Raw( this, &SGitToolbarTab::GetLastFetchedString )
						]
					]
					+ SHorizontalBox::Slot()
					.AutoWidth()
					.Padding( 2.f, 2.f, 10.f, 2.f )
					.VAlign( VAlign_Center )
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.Padding( 2.f )
						.AutoWidth()
						[
							SNew( STextBlock )
							.Text_Raw( this, &SGitToolbarTab::GetAheadNumText )
							.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 8 ) )
							.Visibility_Raw( this, &SGitToolbarTab::GetAheadVisibility )
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign( VAlign_Center )
						[
							SNew( SBox )
							.WidthOverride( 12 )
							.HeightOverride( 12 )
							[
								SNew( SImage )
								.Image( FEditorStyle::GetBrush( "Kismet.TitleBarEditor.ArrowUp" ) )
								.Visibility_Raw( this, &SGitToolbarTab::GetAheadVisibility )
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding( 2.f )
						[
							SNew( STextBlock )
							.Text_Raw( this, &SGitToolbarTab::GetBehindNumText )
							.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 8 ) )
							.Visibility_Raw( this, &SGitToolbarTab::GetBehindVisibility )
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.VAlign( VAlign_Center )
						[
							SNew( SBox )
							.WidthOverride( 12 )
							.HeightOverride( 12 )
							[
								SNew( SImage )
								.Image( FEditorStyle::GetBrush( "Kismet.TitleBarEditor.ArrowDown" ) )
								.Visibility_Raw( this, &SGitToolbarTab::GetBehindVisibility )
							]
						]
					]
				]
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			.Padding( 10.f, 5.f )
			[
				SNew( SSeparator )
				.Orientation( Orient_Vertical )
			]
		];
}

FText SGitToolbarTab::GetLocalBranch() const
{
	return FText::FromString( FVSPGitProvider::Get().GetSettings()->GetCurrentRepoSettings()->LocalBranch );
}

FText SGitToolbarTab::GetRemoteBranch() const
{
	return FText::FromString( FVSPGitProvider::Get().GetSettings()->GetCurrentRepoSettings()->UpstreamBranch );
}

void SGitToolbarTab::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	auto& Provider = FVSPGitProvider::Get();

	// Check state
	if ( Provider.GetSettings()->GetCurrentRepoSettings()->UpstreamBranch.Contains( TEXT( "There is no" ) ) )
		SmartButtonState = NoUpstreamBranch;
	else if ( Provider.GetDivergence().Behind != 0 )
		SmartButtonState = NeedPull;
	else if ( Provider.GetDivergence().Ahead != 0 )
		SmartButtonState = NeedPush;
	else
		SmartButtonState = AlreadyUpToDate;
}

bool SGitToolbarTab::IsBusy() const
{
	TArray< FVSPGitWork* > ActiveWorks = FVSPGitProvider::Get().GetWorksQueue();

	int32 CountWorksWithoutBgTask = 0;
	for ( auto&& ActiveWork : ActiveWorks )
		if ( !ActiveWork->Operation->GetName().IsEqual( "Background" ) )
			CountWorksWithoutBgTask++;

	return CountWorksWithoutBgTask > 0;
}

bool SGitToolbarTab::IsNotBusy() const
{
	return !IsBusy();
}

EVisibility SGitToolbarTab::GetBusyVisibility() const
{
	return IsBusy() ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SGitToolbarTab::GetNotBusyVisibility() const
{
	return IsBusy() ? EVisibility::Collapsed : EVisibility::Visible;
}

int32 SGitToolbarTab::GetAheadNum() const
{
	return FVSPGitProvider::Get().Divergence.Ahead;
}

FText SGitToolbarTab::GetAheadNumText() const
{
	return FText::FromString( FString::Printf( TEXT( "%d" ), GetAheadNum() ) );
}

EVisibility SGitToolbarTab::GetAheadVisibility() const
{
	return GetAheadNum() == 0 ? EVisibility::Collapsed : EVisibility::Visible;
}

int32 SGitToolbarTab::GetBehindNum() const
{
	return FVSPGitProvider::Get().Divergence.Behind;
}

FText SGitToolbarTab::GetBehindNumText() const
{
	return FText::FromString( FString::Printf( TEXT( "%d" ), GetBehindNum() ) );
}

EVisibility SGitToolbarTab::GetBehindVisibility() const
{
	return GetBehindNum() == 0 ? EVisibility::Collapsed : EVisibility::Visible;
}

FText SGitToolbarTab::GetLastFetchedString() const
{
	FString FetchProcess = TEXT( "Never fetched" );

	const FString FetchHeadFilePath = FPaths::Combine( FPaths::ProjectDir(), TEXT( ".git/FETCH_HEAD" ) );
	if ( FPaths::FileExists( FetchHeadFilePath ) )
	{
		const FDateTime LastModificationTime = IFileManager::Get().GetTimeStamp( *FetchHeadFilePath );
		const FTimespan DeltaTime = FDateTime::UtcNow() - LastModificationTime;
		if ( ( DeltaTime.GetDays() == 0 ) && ( DeltaTime.GetHours() == 0 ) )
		{
			if ( DeltaTime.GetMinutes() < 1 )
				FetchProcess = TEXT( "Last fetched just now" );
			else
				FetchProcess = FString::Printf( TEXT( "Last fetched %d minutes ago" ), DeltaTime.GetMinutes() );
		}
		else if ( ( DeltaTime.GetDays() == 0 ) )
			FetchProcess = FString::Printf( TEXT( "Last fetched %d hourses ago" ), DeltaTime.GetHours() );
		else
			FetchProcess = FString::Printf( TEXT( "Last fetched %d days ago" ), DeltaTime.GetDays() );
	}

	return FText::FromString( FetchProcess );
}

FReply SGitToolbarTab::SmartButtonClicked()
{
	switch ( SmartButtonState )
	{
		case NoUpstreamBranch:
			FVSPGitSlateLayout::Get().PushClicked();
			break;
		case NeedPull:
			FVSPGitSlateLayout::Get().PullClicked();
			break;
		case NeedPush:
			FVSPGitSlateLayout::Get().PushClicked();
			break;
		case AlreadyUpToDate:
			FVSPGitSlateLayout::Get().FetchClicked();
			break;
		default: ;
	}

	return FReply::Handled();
}

FText SGitToolbarTab::GetSmartStepString() const
{
	FString SmartStepString;
	switch ( SmartButtonState )
	{
		case NoUpstreamBranch:
			SmartStepString = TEXT( "Publish branch" );
			break;
		case NeedPull:
			SmartStepString = TEXT( "Pull origin" );
			break;
		case NeedPush:
			SmartStepString = TEXT( "Push origin" );
			break;
		case AlreadyUpToDate:
		default:
			SmartStepString = TEXT( "Fetch origin" );
	}
	return FText::FromString( SmartStepString );
}

const FSlateBrush* SGitToolbarTab::GetSmartButtonIcon() const
{
	switch ( SmartButtonState )
	{
		case NoUpstreamBranch:
			return FEditorStyle::Get().GetBrush( "BlendSpaceEditor.ArrowUp" );
		case NeedPull:
			return FEditorStyle::Get().GetBrush( "BlendSpaceEditor.ArrowDown" );
		case NeedPush:
			return FEditorStyle::Get().GetBrush( "BlendSpaceEditor.ArrowUp" );
		case AlreadyUpToDate:
		default:
			return FVSPGitStyle::Get().GetBrush( "VSPGit.FetchSpinner" );
	}
}

FText SGitToolbarTab::GetBusyStepString() const
{
	auto WorksQueue = FVSPGitProvider::Get().GetWorksQueue();

	FString ProcessStep = TEXT( "Refreshing repository" );

	if ( WorksQueue.Num() != 0 )
	{
		// "UpdateStatus"
		if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "UpdateStatus" ) ) )
		{
			// Nothing
		}
			// "Fetch"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Fetch" ) ) )
			ProcessStep = TEXT( "Fetching origin" );
			// "Commit"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Commit" ) ) )
			ProcessStep = FString::Printf( TEXT( "Commiting to %s" ), *FVSPGitProvider::Get().GetSettings()->GetCurrentRepoSettings()->LocalBranch );
			// "Connect"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Connect" ) ) )
			ProcessStep = TEXT( "Connecting SourceControl" );
			// "CheckOut"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "CheckOut" ) ) )
			ProcessStep = TEXT( "Locking LFS changes" );
			// "CheckIn"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "CheckIn" ) ) )
			ProcessStep = FString::Printf( TEXT( "Commiting to %s" ), *FVSPGitProvider::Get().GetSettings()->GetCurrentRepoSettings()->LocalBranch );
			// "MarkForAdd"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "MarkForAdd" ) ) )
			ProcessStep = TEXT( "Marking to add" );
			// "Pull"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Pull" ) ) )
			ProcessStep = TEXT( "Pulling from origin" );
			// "Push"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Push" ) ) )
			ProcessStep = TEXT( "Pushing to origin" );
			// "Delete"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Delete" ) ) )
			ProcessStep = TEXT( "Marking to delete" );
			// "Switch"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Switch" ) ) )
			ProcessStep = TEXT( "Switching to branch" );
			// "Branch"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Branch" ) ) )
			ProcessStep = TEXT( "Updating devergences" );
			// "Revert"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Revert" ) ) )
			ProcessStep = TEXT( "Reverting files" );
			// "Resolve"
		else if ( WorksQueue[ 0 ]->Operation->GetName().IsEqual( FName( "Resolve" ) ) )
			ProcessStep = TEXT( "Resolving conflict" );
	}

	return FText::FromString( ProcessStep );
}

FText SGitToolbarTab::GetBusyProcessString() const
{
	FString ProcessString = TEXT( "Hang on..." );

	return FText::FromString( ProcessString );
}
#undef LOCTEXT_NAMESPACE
