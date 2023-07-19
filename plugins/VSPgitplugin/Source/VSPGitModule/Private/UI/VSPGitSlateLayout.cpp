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
#include "VSPGitSlateLayout.h"
// Engine headers
#include "IVSPExternalModule.h"
#include "VSPGitPullParamsWindow.h"
#include "VSPGitUiCommands.h"
#include "ToolMenus.h"
#include "Core/Commands/GitHighLevelWorkers.h"
#include "VSPGitModule.h"
#include "Logging/MessageLog.h"
#include "Widgets/VSPMessageBox.h"
#include "Widgets/SGitToolbarTab.h"
#include "Widgets/Notifications/SNotificationList.h"
#include "Widgets/SChangesTab.h"
#include "Widgets/SLockedTab.h"

#define LOCTEXT_NAMESPACE "VSPGitSourceControl"

TSharedPtr< class FVSPGitSlateLayout > FVSPGitSlateLayout::LayoutInstance = nullptr;

static TSharedPtr< FTabManager > VSPGitTabManager;

void FVSPGitSlateLayout::Initialize()
{
	if ( !LayoutInstance.IsValid() )
	{
		LayoutInstance = Create();

		ensure( LayoutInstance.IsUnique() );
		LayoutInstance->RegisterTabSpawners();
		LayoutInstance->RegisterToolbarExtender();
	}
}

void FVSPGitSlateLayout::Shutdown()
{
	LayoutInstance->bIsToolbarButtonVisible = false;
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner( "VSPGitTab" );
	FVSPGitUiCommands::Unregister();

	ensure( LayoutInstance.IsUnique() );
}

FVSPGitSlateLayout& FVSPGitSlateLayout::Get()
{
	return *LayoutInstance;
}

FName FVSPGitSlateLayout::GetLayoutName()
{
	static FName LayoutName( TEXT( "VSPGitLayout" ) );
	return LayoutName;
}

void FVSPGitSlateLayout::RestoreLayout() const
{
	//FGlobalTabmanager::Get()->RestoreFrom( MainLayout.ToSharedRef(), TSharedPtr<SWindow>() );
	FGlobalTabmanager::Get()->TryInvokeTab( FName( "VSPGitTab" ) );
}

void FVSPGitSlateLayout::ShowUiExtenders()
{
	bIsToolbarButtonVisible = true;
}

void FVSPGitSlateLayout::HideUiExtenders()
{
	bIsToolbarButtonVisible = false;
}

void FVSPGitSlateLayout::SetBranchesDivergence(
	FString InLocalBranch,
	FString InUpstreamBranch,
	VSPGitHelpers::FDivergence InDivergence )
{
	LocalBranch = InLocalBranch;
	UpstreamBranch = InUpstreamBranch;
	Divergence = InDivergence;
}

void FVSPGitSlateLayout::PushClicked()
{
	UE_LOG( VSPGitLog, Log, TEXT("FVSPGitModule: Push menu item clicked") );
	if ( !OperationInProgressNotification.IsValid() )
	{
		// Launch a "Push" Operation
		const TSharedRef< GitHighLevelWorkers::FPush, ESPMode::ThreadSafe > PushOperation = ISourceControlOperation::Create< GitHighLevelWorkers::FPush >();

		const ECommandResult::Type Result = FVSPGitProvider::AccessProvider().Execute(
			PushOperation,
			TArray< FString >(),
			EConcurrency::Asynchronous,
			FSourceControlOperationComplete::CreateRaw(
				this,
				&FVSPGitSlateLayout::OnSourceControlOperationComplete ) );

		if ( Result == ECommandResult::Succeeded )
			// Display an ongoing notification during the whole operation
			DisplayInProgressNotification( PushOperation->GetInProgressString() );
		else
			// Report failure with a notification
			DisplayFailureNotification( PushOperation );
	}
	else
	{
		FMessageLog SourceControlLog( "SourceControl" );
		SourceControlLog.Warning( LOCTEXT( "SourceControlMenu_InProgress", "Source control operation already in progress" ) );
		SourceControlLog.Notify();
	}
}

void FVSPGitSlateLayout::PullClicked()
{
	UE_LOG( VSPGitLog, Log, TEXT("FVSPGitModule: Pull menu item clicked") );

	const auto ResultParams = FVSPGitPullParamsWindow::ShowDialog();

	if ( ResultParams.bDialogResult )
	{
		if ( !OperationInProgressNotification.IsValid() )
		{
			// Launch a "Pull" Operation
			TSharedRef< GitHighLevelWorkers::FPull, ESPMode::ThreadSafe > PullOperation = ISourceControlOperation::Create< GitHighLevelWorkers::FPull >();

			UE_LOG( VSPGitLog, Log, TEXT("FVSPGitModule: Pull button clicked. Parameters checked.") );

			if ( ResultParams.bFFOnlyParam )
			{
				UE_LOG( VSPGitLog, Log, TEXT("FVSPGitModule: Checked --ff-only for pull command") );
				PullOperation->AddArg( TEXT( "--ff-only" ) );
			}
			if ( ResultParams.bFFOnlyWithForceParam )
			{
				UE_LOG( VSPGitLog, Log, TEXT("FVSPGitModule: Checked --force for pull command") );
				PullOperation->AddArg( TEXT( "--force" ) );
			}
			if ( ResultParams.bViaRebase )
			{
				UE_LOG( VSPGitLog, Log, TEXT("FVSPGitModule: Checked -r for pull command") );
				PullOperation->AddArg( TEXT( "--rebase" ) );
			}

			const ECommandResult::Type Result = FVSPGitProvider::AccessProvider().Execute(
				PullOperation,
				TArray< FString >() );

			//	EConcurrency::Asynchronous,
			//	FSourceControlOperationComplete::CreateRaw(
			//		this,
			//		&FVSPGitSlateLayout::OnSourceControlOperationComplete));

			//if (Result == ECommandResult::Succeeded)
			//{
			//	// Display an ongoing notification during the whole operation
			//	DisplayInProgressNotification(PullOperation->GetInProgressString());
			//}
			//else
			//{
			//	// Report failure with a notification
			//	DisplayFailureNotification(PullOperation->GetName());
			//}
			if ( Result == ECommandResult::Succeeded )
				// Display an ongoing notification during the whole operation
				//DisplayInProgressNotification(PullOperation->GetInProgressString());
				DisplaySuccessNotification( PullOperation );
			else
				DisplayFailureNotification( PullOperation );
		}
		else
		{
			FMessageLog SourceControlLog( "SourceControl" );
			SourceControlLog.Warning( LOCTEXT( "SourceControlMenu_InProgress", "Source control operation already in progress" ) );
			SourceControlLog.Notify();
		}
	}
}

void FVSPGitSlateLayout::FetchClicked()
{
	UE_LOG( VSPGitLog, Log, TEXT("FVSPGitModule: Fetch menu item clicked") );
	if ( !OperationInProgressNotification.IsValid() )
	{
		// Launch a "Fetch" Operation
		const TSharedRef< GitHighLevelWorkers::FFetch, ESPMode::ThreadSafe > FetchOperation = ISourceControlOperation::Create< GitHighLevelWorkers::FFetch >();

		const ECommandResult::Type Result = FVSPGitProvider::AccessProvider().Execute(
			FetchOperation,
			TArray< FString >(),
			EConcurrency::Asynchronous,
			FSourceControlOperationComplete::CreateRaw(
				this,
				&FVSPGitSlateLayout::OnSourceControlOperationComplete ) );

		if ( Result == ECommandResult::Succeeded )
			// Display an ongoing notification during the whole operation
			DisplayInProgressNotification( FetchOperation->GetInProgressString() );
		else
			// Report failure with a notification
			DisplayFailureNotification( FetchOperation );
	}
	else
	{
		FMessageLog SourceControlLog( "SourceControl" );
		SourceControlLog.Warning( LOCTEXT( "SourceControlMenu_InProgress", "Source control operation already in progress" ) );
		SourceControlLog.Notify();
	}
}

void FVSPGitSlateLayout::LockMapsAsync( const TArray<FString>& InContentPathList )
{
	UE_LOG(VSPGitLog, Log, TEXT("FVSPGitModule: Lock sector button clicked"));
	if (!OperationInProgressNotification.IsValid())
	{
		TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > OutStates;
		FVSPGitProvider::AccessProvider().GetState(InContentPathList, OutStates, EStateCacheUsage::Use);
		if (OutStates.Num() == 0)
			return;

		if (OutStates[0]->CanCheckout())
		{
			TSharedRef< GitHighLevelWorkers::FLock, ESPMode::ThreadSafe > LockOperation = GitHighLevelWorkers::FLock::Create<
				GitHighLevelWorkers::FLock >();

			const ECommandResult::Type Result = FVSPGitProvider::AccessProvider().Execute(
				LockOperation,
				InContentPathList,
				EConcurrency::Asynchronous,
				FSourceControlOperationComplete::CreateRaw( this, &FVSPGitSlateLayout::OnSourceControlOperationComplete ));

			if (Result == ECommandResult::Succeeded)
				// Display an ongoing notification during the whole operation
				DisplayInProgressNotification(LockOperation->GetInProgressString());
			else
				// Report failure with a notification
				DisplayFailureNotification(LockOperation);
		}
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

void FVSPGitSlateLayout::UnlockMapsAsync( const TArray<FString>& InContentPathList, bool bIsForce )
{
	UE_LOG(VSPGitLog, Log, TEXT("FVSPGitModule: Lock sector button clicked"));
	if (!OperationInProgressNotification.IsValid())
	{
		TArray< TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > OutStates;
		FVSPGitProvider::AccessProvider().GetState(InContentPathList, OutStates, EStateCacheUsage::Use);
		if (OutStates.Num() == 0)
			return;

		if (!bIsForce)
		{
			bool bNeedAskAboutForce = false;
			for (TSharedRef< ISourceControlState, ESPMode::ThreadSafe > SourceControlState : OutStates)
			{
				if (SourceControlState->IsCheckedOutOther())
				{
					bNeedAskAboutForce = true;
					break;
				}
			}

			if (bNeedAskAboutForce)
			{
				EMessageBoxResult Result = FVSPMessageBox::ShowDialog(
					TEXT("You are trying to unlock one or more files locked by another user.\n\nForce unlock all selected files?"),
					TEXT("-Warning- Enable Force Unlock?"),
					EMessageBoxButton::YesNoCancelButtons,
					EMessageBoxImage::Question);

				if (Result == EMessageBoxResult::Yes)
					bIsForce = true;
				else if (Result == EMessageBoxResult::Cancel)
					return;
			}
		}

		TSharedRef< GitHighLevelWorkers::FUnlock, ESPMode::ThreadSafe > UnlockOperation =
			GitHighLevelWorkers::FUnlock::Create< GitHighLevelWorkers::FUnlock >();
		if (bIsForce)
			UnlockOperation->SetForceArg(bIsForce);

		const ECommandResult::Type Result = FVSPGitProvider::AccessProvider().Execute(
			UnlockOperation,
			InContentPathList,
			EConcurrency::Asynchronous,
			FSourceControlOperationComplete::CreateRaw(this, &FVSPGitSlateLayout::OnSourceControlOperationComplete));

		if (Result == ECommandResult::Succeeded)
			// Display an ongoing notification during the whole operation
			DisplayInProgressNotification(UnlockOperation->GetInProgressString());
		else
			// Report failure with a notification
			DisplayFailureNotification(UnlockOperation);
	}
	else
	{
		FMessageLog SourceControlLog("SourceControl");
		SourceControlLog.Warning(LOCTEXT("SourceControlMenu_InProgress", "Source control operation already in progress"));
		SourceControlLog.Notify();
	}
}

TSharedPtr< FTabManager > FVSPGitSlateLayout::GetTabManager()
{
	return VSPGitTabManager;
}

namespace VSPGitMenu
{
	TSharedRef< FWorkspaceItem > MenuRoot = FWorkspaceItem::NewGroup( NSLOCTEXT( "VSPGit", "MenuRoot", "MenuRoot" ) );
	TSharedRef< FWorkspaceItem > SubTabs = MenuRoot->AddGroup( NSLOCTEXT( "VSPGit", "SecondaryTabs", "Git sections Tabs" ) );
}

void FVSPGitSlateLayout::RegisterTabSpawners()
{
	FGlobalTabmanager::Get()->RegisterNomadTabSpawner( "VSPGitTab", FOnSpawnTab::CreateRaw( this, &FVSPGitSlateLayout::SpawnMainTab ) )
		.SetDisplayName( LOCTEXT( "VSPGitTabTitle", "VSP Git" ) )
		.SetMenuType( ETabSpawnerMenuType::Hidden );

	MainLayout = FTabManager::NewLayout( GetLayoutName() )
		->AddArea(
			FTabManager::NewArea( 1024, 768 )
			->Split(
				FTabManager::NewStack()
				->AddTab( "VSPGitTab", ETabState::OpenedTab )
			)
		);
}

void FVSPGitSlateLayout::RegisterToolbarExtender()
{
	FVSPGitUiCommands::Register();

	PluginCommands = MakeShareable( new FUICommandList );
	PluginCommands->MapAction(
		FVSPGitUiCommands::Get().OpenMainWindow,
		FExecuteAction::CreateRaw( this, &FVSPGitSlateLayout::RestoreLayout ),
		FCanExecuteAction(),
		FIsActionChecked(),
		FIsActionButtonVisible::CreateRaw( this, &FVSPGitSlateLayout::HandleToolbarButtonVisible ) );

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw( this, &FVSPGitSlateLayout::RegisterMenusAndButton ) );
}

void FVSPGitSlateLayout::RegisterMenusAndButton()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped( this );

	{
		UToolMenu* ToolbarMenu = UToolMenus::Get()->ExtendMenu( "LevelEditor.LevelEditorToolBar" );
		{
			FToolMenuSection& Section = ToolbarMenu->FindOrAddSection( "File" );
			{
				FToolMenuEntry& Entry = Section.AddEntry(
					FToolMenuEntry::InitToolBarButton( FVSPGitUiCommands::Get().OpenMainWindow ) );
				Entry.SetCommandList( PluginCommands );
			}
		}
	}
}

bool FVSPGitSlateLayout::HandleToolbarButtonVisible()
{
	return bIsToolbarButtonVisible;
}

const FSlateBrush* FVSPGitSlateLayout::GetHolderImage() const
{
	return FVSPGitStyle::Get().GetBrush( "VSPGit.Holder" );
}

TSharedRef< SWidget > FVSPGitSlateLayout::GetHolderWidget()
{
	return SNew( SOverlay )
		+ SOverlay::Slot()
		[
			SNew( SImage )
			.Image_Raw( this, &FVSPGitSlateLayout::GetHolderImage )
		]
		+ SOverlay::Slot()
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Center )
			.FillWidth( 1. )
			[
				SNew( SVerticalBox )
				+ SVerticalBox::Slot()
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				.FillHeight( 1. )
				[
					SNew( STextBlock )
					.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 20 ) )
					.ColorAndOpacity( FLinearColor::FromSRGBColor( FColor( 222, 163, 9 ) ) )
					.Text( NSLOCTEXT( "VSPGit", "ChangesViewerTabHolder", "Coming soon..." ) )
				]
			]
		];
}

TSharedRef< SDockTab > FVSPGitSlateLayout::SpawnMainTab( const FSpawnTabArgs& Args )
{
	TSharedRef< FTabManager::FLayout > Layout = FTabManager::NewLayout( "VSPGit_Layout" )
		->AddArea(
			// The primary area will be restored and returned as a widget.
			// Unlike other areas it will not get its own window.
			// This allows the caller to explicitly place the primary area somewhere in the widget hierarchy.
			FTabManager::NewPrimaryArea()
			->Split(
				//The first cell in the primary area will be occupied by a stack of tabs.
				FTabManager::NewStack()
				->SetSizeCoefficient( 0.2f )
				->SetHideTabWell( true )
				->AddTab( "ChangesTab", ETabState::OpenedTab )
				->AddTab( "HistoryTab", ETabState::ClosedTab )
				->AddTab( "LockedListTab", ETabState::OpenedTab )
				->SetForegroundTab( FName( "ChangesTab" ) )
			)
			->Split(
				// We can subdivide a cell further by using an additional splitter
				FTabManager::NewSplitter()
				->SetOrientation( Orient_Vertical )
				->SetSizeCoefficient( 0.8f )
				->Split(
					FTabManager::NewStack()
					->SetHideTabWell( true )
					->SetSizeCoefficient( 0.25f )
					->AddTab( "GitToolboxTab", ETabState::OpenedTab )
				)
				->Split(
					FTabManager::NewStack()
					->SetSizeCoefficient( 0.75f )
					->SetHideTabWell( true )
					->AddTab( "ChangesViewerTab", ETabState::ClosedTab )
					->AddTab( "HistoryViewerTab", ETabState::ClosedTab )
					->AddTab( "MapSectorLockView", ETabState::OpenedTab )
					->SetForegroundTab( FName( "MapSectorLockView" ) )
				)
			)
		);

	TSharedRef< SDockTab > VSPGitTab =
		SNew( SDockTab )
		//.TabRole( ETabRole::MajorTab )
		.TabRole( NomadTab )
		.Label( LOCTEXT( "VSPGitTabLabel", "ProjectVSP Git" ) )
		.ToolTipText( LOCTEXT( "VSPGitTabToolTip", "Unreal editor extensions for managing GitSourseControl." ) );

	VSPGitTabManager = FGlobalTabmanager::Get()->NewTabManager( VSPGitTab );

	VSPGitTabManager->RegisterTabSpawner( "ChangesTab", FOnSpawnTab::CreateRaw( this, &FVSPGitSlateLayout::SpawnSubTab, FName( "ChangesTab" ) ) )
		.SetDisplayName( NSLOCTEXT( "VSPGitTab", "ChangesTab", "Changes" ) );

	VSPGitTabManager->RegisterTabSpawner( "HistoryTab", FOnSpawnTab::CreateRaw( this, &FVSPGitSlateLayout::SpawnSubTab, FName( "HistoryTab" ) ) )
		.SetDisplayName( NSLOCTEXT( "VSPGitTab", "HistoryTab", "History" ) )
		.SetGroup( VSPGitMenu::SubTabs );

	VSPGitTabManager->RegisterTabSpawner( "LockedListTab", FOnSpawnTab::CreateRaw( this, &FVSPGitSlateLayout::SpawnSubTab, FName( "LockedListTab" ) ) )
		.SetDisplayName( NSLOCTEXT( "VSPGitTab", "LockedListTab", "Locked list" ) )
		.SetGroup( VSPGitMenu::SubTabs );

	VSPGitTabManager->RegisterTabSpawner( "GitToolboxTab", FOnSpawnTab::CreateRaw( this, &FVSPGitSlateLayout::SpawnSubTab, FName( "GitToolboxTab" ) ) )
		.SetDisplayName( NSLOCTEXT( "VSPGitTab", "GitToolboxTab", "Git Toolbox" ) )
		.SetGroup( VSPGitMenu::SubTabs );

	VSPGitTabManager->RegisterTabSpawner( "ChangesViewerTab", FOnSpawnTab::CreateRaw( this, &FVSPGitSlateLayout::SpawnSubTab, FName( "ChangesViewerTab" ) ) )
		.SetDisplayName( NSLOCTEXT( "VSPGitTab", "ChangesViewerTab", "Changes Viewer" ) )
		.SetGroup( VSPGitMenu::SubTabs );

	VSPGitTabManager->RegisterTabSpawner( "HistoryViewerTab", FOnSpawnTab::CreateRaw( this, &FVSPGitSlateLayout::SpawnSubTab, FName( "HistoryViewerTab" ) ) )
		.SetDisplayName( NSLOCTEXT( "VSPGitTab", "HistoryViewerTab", "History Viewer" ) )
		.SetGroup( VSPGitMenu::SubTabs );

	if ( FModuleManager::Get().IsModuleLoaded( FName( "MapSectorLockView" ) ) )
	{
		IVSPExternalModule& MapLockModule = FModuleManager::Get().LoadModuleChecked< IVSPExternalModule >(
			FName( "MapSectorLockView" ) );
		MapLockModule.RegisterTabSpawner();
	}

	// Create menu for VSPGitTab
	FMenuBarBuilder MenuBarBuilder = FMenuBarBuilder( TSharedPtr< FUICommandList >() );
	MenuBarBuilder.AddPullDownMenu(
		NSLOCTEXT( "VSPGit", "WindowMenuLabel", "Window" ),
		FText::GetEmpty(),
		FNewMenuDelegate::CreateSP( VSPGitTabManager.ToSharedRef(), &FTabManager::PopulateTabSpawnerMenu, VSPGitMenu::MenuRoot ) );

	VSPGitTab->SetContent(
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			MenuBarBuilder.MakeWidget()
		]
		+ SVerticalBox::Slot()
		.FillHeight( 1.0f )
		[
			VSPGitTabManager->RestoreFrom( Layout, Args.GetOwnerWindow() ).ToSharedRef()
		]
	);

	return VSPGitTab;
}

TSharedRef< SDockTab > FVSPGitSlateLayout::SpawnSubTab( const FSpawnTabArgs& Args, FName TabIdentifier )
{
	if ( TabIdentifier == FName( "ChangesTab" ) )
		return SNew( SDockTab )
			.Label( LOCTEXT( "ChangesTab", "Changes" ) )
			.ToolTipText( LOCTEXT( "ChangesTabToolTip", "Area for viewing the list of changes." ) )
			.Clipping( EWidgetClipping::ClipToBounds )
			[
				SNew( SChangesTab )
			];
	if ( TabIdentifier == FName( "HistoryTab" ) )
		return SNew( SDockTab );
	if ( TabIdentifier == FName( "LockedListTab" ) )
		return SNew( SDockTab )
			.Label( LOCTEXT( "LockedListTab", "LockedList" ) )
			.ToolTipText( LOCTEXT( "LockedListTabToolTip", "List of all locked files" ) )
			.Clipping( EWidgetClipping::ClipToBounds )
			[
				SNew( SLockedTab )
			];
	if ( TabIdentifier == FName( "GitToolboxTab" ) )
		return SNew( SDockTab )
			.Label( LOCTEXT( "GitToolboxTab", "Git Toolbox" ) )
			.ToolTipText( LOCTEXT( "GitToolboxTabToolTip", "Git Toolbox with main operations." ) )
			.ShouldAutosize( true )
			.Clipping( EWidgetClipping::ClipToBoundsWithoutIntersecting )
			[
				SNew( SGitToolbarTab )
				// .LocalBranch(LocalBranch)
				// .RemoteBranch(UpstreamBranch)
				// .LocalDivergance(Divergence.Ahead)
				// .RemoteDivergence(Divergence.Behind)
			];
	if ( TabIdentifier == FName( "ChangesViewerTab" ) )
		return SNew( SDockTab )
		[
			GetHolderWidget()
		];
	if ( TabIdentifier == FName( "HistoryViewerTab" ) )
		return SNew( SDockTab )
		[
			GetHolderWidget()
		];
	if ( TabIdentifier == FName( "MapSectorLockViewTab" ) )
		return VSPGitTabManager->TryInvokeTab( FName( "MapSectorLockView" ) ).ToSharedRef();
	ensure( false );
	return SNew( SDockTab );
}

TSharedRef< FVSPGitSlateLayout > FVSPGitSlateLayout::Create()
{
	return MakeShareable( new FVSPGitSlateLayout() );
}

void FVSPGitSlateLayout::DisplayInProgressNotification( const FText& Text )
{
	if ( !OperationInProgressNotification.IsValid() )
	{
		FNotificationInfo Info( Text );
		Info.bFireAndForget = false;
		Info.ExpireDuration = 0.0f;
		Info.FadeOutDuration = 1.0f;
		OperationInProgressNotification = FSlateNotificationManager::Get().AddNotification( Info );
		if ( OperationInProgressNotification.IsValid() )
			OperationInProgressNotification.Pin()->SetCompletionState( SNotificationItem::CS_Pending );
	}
}

void FVSPGitSlateLayout::RemoveInProgressNotification()
{
	if ( OperationInProgressNotification.IsValid() )
	{
		OperationInProgressNotification.Pin()->ExpireAndFadeout();
		OperationInProgressNotification.Reset();
	}
}

void FVSPGitSlateLayout::DisplaySuccessNotification( const FSourceControlOperationRef& InOperation )
{
	const FText NotificationText = FText::Format(
		LOCTEXT( "SourceControlMenu_Success", "{0} operation was successful!" ),
		FText::FromName( InOperation->GetName() )
	);
	FNotificationInfo Info( NotificationText );
	Info.bUseSuccessFailIcons = true;
	Info.ExpireDuration = 3.0f;
	Info.Image = FEditorStyle::GetBrush( TEXT( "NotificationList.SuccessImage" ) );
	Info.HyperlinkText = LOCTEXT( "SCC_Checkin_ShowLog", "Show Message Log" );
	Info.Hyperlink = FSimpleDelegate::CreateStatic( []() { FMessageLog( "SourceControl" ).Open( EMessageSeverity::Info, true ); } );
	FSlateNotificationManager::Get().AddNotification( Info );

	// also add to the log
	FMessageLog( "SourceControl" ).Info( NotificationText );
}

void FVSPGitSlateLayout::DisplayFailureNotification( const FSourceControlOperationRef& InOperation )
{
	const FText NotificationText = FText::Format(
		LOCTEXT( "SourceControlMenu_Failure", "Error: {0} operation failed!" ),
		FText::FromName( InOperation->GetName() )
	);
	FNotificationInfo Info( NotificationText );
	Info.ExpireDuration = 8.0f;
	Info.HyperlinkText = LOCTEXT( "SCC_Checkin_ShowLog", "Show Message Log" );
	Info.Hyperlink = FSimpleDelegate::CreateStatic( []() { FMessageLog( "SourceControl" ).Open( EMessageSeverity::Error, true ); } );
	FSlateNotificationManager::Get().AddNotification( Info );

	FMessageLog( "SourceControl" ).Error( NotificationText );

	DisplayErrorMessage( InOperation );
}

void FVSPGitSlateLayout::DisplayErrorMessage( const FSourceControlOperationRef& InOperation )
{
	FString FullInfo = "";
	FString FullErrors = "";

	FSourceControlResultInfo Result = InOperation->GetResultInfo();

	FString ErrorMessage;
	for ( auto&& ErrorLine : Result.ErrorMessages )
		ErrorMessage += ErrorLine.ToString() + TEXT( "\n" );
	const FString Caption = FString::Printf( TEXT( "%s operation failed!" ), *InOperation->GetName().ToString() );
	if ( !ErrorMessage.IsEmpty() )
		FVSPMessageBox::ShowDialog( ErrorMessage, Caption, OkButton, Error );
}

void FVSPGitSlateLayout::OnSourceControlOperationComplete(
	const FSourceControlOperationRef& InOperation,
	ECommandResult::Type InResult )
{
	RemoveInProgressNotification();

	// Report result with a notification
	if ( InResult == ECommandResult::Succeeded )
		DisplaySuccessNotification( InOperation );
	else
		DisplayFailureNotification( InOperation );
}

#undef LOCTEXT_NAMESPACE
