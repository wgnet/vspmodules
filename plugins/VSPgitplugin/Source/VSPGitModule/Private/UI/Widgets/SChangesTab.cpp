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
#include "SChangesTab.h"

#include "VSPMessageBox.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Core/VSPGitProvider.h"
#include "Core/Commands/GitHighLevelWorkers.h"
#include "UI/VSPGitSlateLayout.h"
#include "UI/VSPGitStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "SChangesTab"

SChangesTab::SChangesTab()
{
	SourceControlStateChanged = FVSPGitProvider::AccessProvider().RegisterSourceControlStateChanged_Handle(
		FSourceControlStateChanged::FDelegate::CreateRaw( this, &SChangesTab::SourceControlStateChanged_Handle ) );
}

SChangesTab::~SChangesTab()
{
	FVSPGitProvider::AccessProvider().UnregisterSourceControlStateChanged_Handle( SourceControlStateChanged );
}

void SChangesTab::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		SNew( SOverlay )
		+ SOverlay::Slot()
		[
			SNew( SVerticalBox )
			+ SVerticalBox::Slot()
			.FillHeight( 1.0f )
			[
				SNew( SSplitter )
				.Orientation( Orient_Vertical )
				+ SSplitter::Slot()
				.Value( 8.0f )
				[
					// Changed files tree
					SNew( SBorder )
					.BorderImage( FCoreStyle::Get().GetBrush( "ToolPanel.GroupBorder" ) )
					[
						// TODO: Add Scroll
						SAssignNew( ChangesTreePtr, SCheckingFileStatesTree )
						.HeaderText( "changed files" )
						.RootList( &ChangesTreeData )
					]
				]
				+ SSplitter::Slot()
				.Value( 2.0f )
				[
					// Commit view
					SNew( SBorder )
					[
						SNew( SVerticalBox )
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding( 5.f )
						[
							SAssignNew( CommitMessageEditableTextBoxPtr, SEditableTextBox )
							.HintText_Raw( this, &SChangesTab::GetCommitHintText )
							.ToolTipText_Raw( this, &SChangesTab::GetCommitHintText )
						]
						+ SVerticalBox::Slot()
						.FillHeight( 1.0f )
						.Padding( 5.f )
						[
							SAssignNew( CommitDescriptionMultiLineEditableTextBoxPtr, SMultiLineEditableTextBox )
							.HintText( LOCTEXT( "Description", "Description" ) )
						]
						+ SVerticalBox::Slot()
						.AutoHeight()
						.Padding( 5.f )
						[
							SNew( SButton )
							.HAlign( HAlign_Center )
							.Text( this, &SChangesTab::GetCommitToText )
							.IsEnabled_Raw( this, &SChangesTab::GetCommitButtonEnable )
							.OnClicked_Raw( this, &SChangesTab::CommitClicked )
						]
						+ SVerticalBox::Slot()
						[
							SNew( SProgressBar )
							.Visibility_Raw( this, &SChangesTab::GetCommitBusyIndicatorVisibility )
						]
						// TODO: Collapsable last commit info
						// +SVerticalBox::Slot()
						// .AutoHeight()
						// .Padding(5.f, 2.f)
						// [
						// ]
					]
				]
			]
		]
		+ SOverlay::Slot()
		[
			SNew( SColorBlock )
			.Color( FLinearColor::FromSRGBColor( FColor( 100, 100, 100, 80 ) ) )
			.Visibility( this, &SChangesTab::GetStatusBusyIndicatorVisibility )
		]
		+ SOverlay::Slot()
		.HAlign( HAlign_Center )
		.VAlign( VAlign_Center )
		[
			SNew( SCircularThrobber )
			.Visibility( this, &SChangesTab::GetStatusBusyIndicatorVisibility )
		]
	];

	SourceControlStateChanged_Handle();
}

EVisibility SChangesTab::GetStatusBusyIndicatorVisibility() const
{
	const TArray< FVSPGitWork* > ActiveWorks = FVSPGitProvider::AccessProvider().GetWorksQueue();

	const int32 IndexWorkInQueue = ActiveWorks.FindLastByPredicate(
		[]( FVSPGitWork* Work )
		{
			return Work->Operation->GetName().IsEqual( "UpdateStatus" );
		} );

	return ( IndexWorkInQueue != INDEX_NONE ) ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SChangesTab::GetCommitBusyIndicatorVisibility() const
{
	return bIsCommitBusy ? EVisibility::Visible : EVisibility::Collapsed;
}

void SChangesTab::SourceControlStateChanged_Handle()
{
	TMap< FString, TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > FullStatesMap
		= FVSPGitProvider::AccessProvider().GetFilesStates();

	// We filter all known states and get only modified ones.
	TMap< FString, TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > ModifiedStatesMap;
	for ( auto&& StatesPair : FullStatesMap )
		if ( StatesPair.Value->IsModified() || StatesPair.Value->CanAdd() || StatesPair.Value->IsUnknown() )
			ModifiedStatesMap.Add( StatesPair.Key, StatesPair.Value );

	// We get a list of current known states and actual changed states
	TArray< FString > ActualItems;
	ModifiedStatesMap.GetKeys( ActualItems );
	TArray< FString > CurrentItems = ChangesTreePtr->GetAllFiles();

	{
		// Putting a list to remove from the tree.
		TArray< FString > RemoveStatesPaths;
		for ( FString& ItemFromTree : CurrentItems )
		{
			FString* Founded = ActualItems.FindByPredicate(
				[ItemFromTree]( const FString& FilePath )
				{
					return FilePath.Equals( ItemFromTree );
				} );
			if ( Founded == nullptr )
				RemoveStatesPaths.Add( ItemFromTree );
		}

		// And remove them
		for ( const FString& RemovePath : RemoveStatesPaths )
		{
			TArray< FString > TreePath;
			const TCHAR* Delims[ ] = { TEXT( "\\" ), TEXT( "/" ) };
			RemovePath.ParseIntoArray( TreePath, Delims, 2 );

			TSharedPtr< FCheckingDirectoryItem > CurrentParent = nullptr;
			TArray< TSharedPtr< FCheckingTreeItemBase > >* CurrentLevel = &ChangesTreeData;
			for ( const FString& TreeBranchPath : TreePath )
			{
				TSharedPtr< FCheckingTreeItemBase >* FoundedBranch = CurrentLevel->FindByPredicate(
					[TreeBranchPath]( const TSharedPtr< FCheckingTreeItemBase > TreeItem )
					{
						return TreeItem->GetDisplayName().Equals( TreeBranchPath );
					} );

				check( FoundedBranch != nullptr );

				if ( !( *FoundedBranch )->IsFile() )
				{
					TSharedPtr< FCheckingDirectoryItem > FoundedFolder = StaticCastSharedPtr< FCheckingDirectoryItem >( *FoundedBranch );
					CurrentLevel = &FoundedFolder->AccessSubItems();
					CurrentParent = FoundedFolder;
				}
				else
				{
					TSharedPtr< FCheckingFileStateItem > FoundedRemovingItem = StaticCastSharedPtr< FCheckingFileStateItem >( *FoundedBranch );
					CurrentLevel->Remove( FoundedRemovingItem );
				}
			}

			// Removing empty folders
			while ( CurrentParent != nullptr )
				if ( CurrentParent->GetSubItems().Num() == 0 )
				{
					TSharedPtr< FCheckingDirectoryItem > GrandParent = nullptr;
					if ( CurrentParent->GetParentItem() != nullptr )
					{
						GrandParent = StaticCastSharedPtr< FCheckingDirectoryItem >( CurrentParent->GetParentItem() );
						CurrentLevel = &GrandParent->AccessSubItems();
					}
					else
					{
						GrandParent = nullptr;
						CurrentLevel = &ChangesTreeData;
					}

					CurrentLevel->Remove( CurrentParent );
					CurrentParent = GrandParent;
				}
				else
					break;
		}
	}

	{
		// Putting to tree a new items paths and exist
		TArray< FString > NewItemsPaths;
		TArray< FString > ExistItemsPaths;
		for ( FString& ActualItemPath : ActualItems )
		{
			FString* Founded = CurrentItems.FindByPredicate(
				[ActualItemPath]( const FString& FilePath )
				{
					return FilePath.Equals( ActualItemPath );
				} );
			if ( Founded == nullptr )
				NewItemsPaths.Add( ActualItemPath );
			else
				ExistItemsPaths.Add( ActualItemPath );
		}

		// Update exist states
		for ( FString& ExistItemPath : ExistItemsPaths )
		{
			TSharedPtr< FCheckingTreeItemBase > Item = ChangesTreePtr->GetItemByPath( ExistItemPath );
			if ( ( Item != nullptr ) && ( Item->IsFile() ) )
			{
				TSharedPtr< FCheckingFileStateItem > FileItem = StaticCastSharedPtr< FCheckingFileStateItem >( Item );
				check( FileItem.IsValid() )

				TSharedRef< ISourceControlState, ESPMode::ThreadSafe >* State = ModifiedStatesMap.Find( ExistItemPath );
				check( State != nullptr )

				FileItem->UpdateState( *State );
			}
		}

		// And add them
		for ( FString& NewItemPath : NewItemsPaths )
		{
			TArray< FString > TreePath;
			const TCHAR* Delims[ ] = { TEXT( "\\" ), TEXT( "/" ) };
			NewItemPath.ParseIntoArray( TreePath, Delims, 2 );

			TSharedPtr< FCheckingDirectoryItem > CurrentParent = nullptr;
			TArray< TSharedPtr< FCheckingTreeItemBase > >* CurrentLevel = &ChangesTreeData;
			for ( const FString& TreeBranchPath : TreePath )
			{
				TSharedPtr< FCheckingTreeItemBase >* Founded = CurrentLevel->FindByPredicate(
					[TreeBranchPath]( TSharedPtr< FCheckingTreeItemBase > ItemBase )
					{
						return ItemBase->GetDisplayName().Equals( TreeBranchPath );
					} );
				bool bIsFile = TreePath.IndexOfByKey( TreeBranchPath ) == ( TreePath.Num() - 1 );
				if ( Founded != nullptr )
				{
					if ( !( *Founded )->IsFile() )
					{
						TSharedPtr< FCheckingDirectoryItem > DirectoryItem = StaticCastSharedPtr<
							FCheckingDirectoryItem >( *Founded );

						check( DirectoryItem.IsValid() )

						CurrentParent = DirectoryItem;
						CurrentLevel = &DirectoryItem->AccessSubItems();
					}
				}
				else
				{
					if ( bIsFile )
					{
						TSharedRef< ISourceControlState, ESPMode::ThreadSafe >* State = ModifiedStatesMap.Find(
							NewItemPath );

						check( State != nullptr )

						TSharedPtr< FCheckingFileStateItem > FileStateItem =
							MakeShareable( new FCheckingFileStateItem( CurrentParent, *State ) );

						if ( CurrentParent == nullptr )
							ChangesTreeData.Add( FileStateItem );
						else
							CurrentParent->AddSubItem( FileStateItem );

						// Expand item
						while ( CurrentParent != nullptr )
						{
							ChangesTreePtr->ExpandItem( CurrentParent, true );
							CurrentParent = StaticCastSharedPtr< FCheckingDirectoryItem >( CurrentParent->GetParentItem() );
						}
					}
					else
					{
						TSharedPtr< FCheckingDirectoryItem > DirectoryItem =
							MakeShareable( new FCheckingDirectoryItem( CurrentParent, TreeBranchPath ) );
						if ( CurrentParent == nullptr )
							ChangesTreeData.Add( DirectoryItem );
						else
							CurrentParent->AddSubItem( DirectoryItem );
						CurrentParent = DirectoryItem;
						CurrentLevel = &DirectoryItem->AccessSubItems();
					}
				}
			}
		}
	}

	ChangesTreePtr->RedrawTree();
}

FReply SChangesTab::CommitClicked()
{
	PromptForCommit();
	return FReply::Handled();
}

bool SChangesTab::GetCommitButtonEnable() const
{
	auto CheckedFiles = ChangesTreePtr->GetCheckedFiles();

	if ( ( CheckedFiles.Num() > 1 ) && ( !CommitMessageEditableTextBoxPtr->GetText().IsEmpty() ) )
		return true;

	if ( CheckedFiles.Num() == 1 )
		return true;

	return false;
}

FText SChangesTab::GetCommitHintText() const
{
	const TArray< FString > CheckedFiles = ChangesTreePtr->GetCheckedFiles();
	if ( CheckedFiles.Num() != 1 )
		return LOCTEXT( "SummaryCommit", "Summary (required)" );

	for ( const FString& FileName : CheckedFiles )
		/*switch (FileStatePair.Value->State)
		{
		case EGitState::Added:
			return FText::FromString(FString::Printf(TEXT("Added %s"), *RelativeFileName));
		case EGitState::Deleted:
			return FText::FromString(FString::Printf(TEXT("Deleted %s"), *RelativeFileName));
		case EGitState::Renamed:
		case EGitState::Modified:
			return FText::FromString(FString::Printf(TEXT("Updated %s"), *RelativeFileName));
		}*/
		return FText::FromString( FString::Printf( TEXT( "Updated %s" ), *FileName ) );

	return LOCTEXT( "SummaryCommit", "Summary (required)" );
}

void SChangesTab::PromptForCommit()
{
	bIsCommitBusy = true;

	// Get checked files
	TArray< FString > CheckedFiles =
		ChangesTreePtr->GetCheckedFiles();

	FText CommitMsg = CommitMessageEditableTextBoxPtr->GetText();
	if ( CommitMsg.IsEmpty() )
		CommitMsg = GetCommitHintText();
	const FText CommitDescription = CommitDescriptionMultiLineEditableTextBoxPtr->GetText();

	// Mark files for add as needed
	bool bSuccess = true; // Overall success
	bool bAddSuccess = true;
	bool bCheckinSuccess = true;

	TMap< FString, TSharedRef< ISourceControlState, ESPMode::ThreadSafe > > States = FVSPGitProvider::AccessProvider().
		GetFilesStates();

	TArray< FString > FilesForAdd;
	for ( const FString& FileName : CheckedFiles )
	{
		TSharedRef< ISourceControlState, ESPMode::ThreadSafe >* State = States.Find( FileName );
		if ( ( State != nullptr ) && ( ( *State )->IsAdded() || ( *State )->CanAdd() ) )
			FilesForAdd.Add( FileName );
	}

	if ( FilesForAdd.Num() > 0 )
	{
		bAddSuccess = FVSPGitProvider::AccessProvider().Execute( ISourceControlOperation::Create< FMarkForAdd >(), FilesForAdd ) == ECommandResult::Succeeded;
		bSuccess &= bAddSuccess;
	}

	// Check in files
	TSharedRef< GitHighLevelWorkers::FCommit, ESPMode::ThreadSafe > CommitOperation = ISourceControlOperation::Create< GitHighLevelWorkers::FCommit >();
	CommitOperation->SetHeaderText( CommitMsg );
	CommitOperation->SetDescription( CommitDescription );

	bCheckinSuccess = FVSPGitProvider::AccessProvider().Execute( CommitOperation, CheckedFiles ) == ECommandResult::Succeeded;
	bSuccess &= bCheckinSuccess;

	if ( bCheckinSuccess )
		FVSPGitSlateLayout::DisplaySuccessNotification( CommitOperation );

	if ( !bSuccess )
		FVSPGitSlateLayout::DisplayFailureNotification( CommitOperation );

	if ( bCheckinSuccess )
	{
		CommitMessageEditableTextBoxPtr->SetText( FText() );
		CommitDescriptionMultiLineEditableTextBoxPtr->SetText( FText() );
	}

	bIsCommitBusy = false;
}

FText SChangesTab::GetCommitToText() const
{
	return FText::FromString(
		FString::Printf( TEXT( "Commit to %s" ), *FVSPGitProvider::AccessProvider().GetSettings()->GetCurrentRepoSettings()->LocalBranch ) );
}

#undef LOCTEXT_NAMESPACE
