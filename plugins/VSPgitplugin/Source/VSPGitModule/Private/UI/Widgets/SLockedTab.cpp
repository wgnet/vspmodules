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
#include "SLockedTab.h"

#include "VSPMessageBox.h"
#include "Framework/Notifications/NotificationManager.h"
#include "Core/VSPGitProvider.h"
#include "Core/VSPGitState.h"
#include "Core/Base/VSPGitWork.h"
#include "Core/Commands/GitHighLevelWorkers.h"
#include "UI/VSPGitSlateLayout.h"
#include "UI/VSPGitStyle.h"
#include "Widgets/Images/SThrobber.h"
#include "Widgets/Notifications/SNotificationList.h"

#define LOCTEXT_NAMESPACE "SLocedTab"

SLockedTab::SLockedTab()
{
	SourceControlStateChanged = FVSPGitProvider::AccessProvider().RegisterSourceControlStateChanged_Handle(
		FSourceControlStateChanged::FDelegate::CreateRaw(this, &SLockedTab::SourceControlStateChanged_Handle));
}

SLockedTab::~SLockedTab()
{
	FVSPGitProvider::AccessProvider().UnregisterSourceControlStateChanged_Handle(SourceControlStateChanged);
}

void SLockedTab::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			[
				SNew(SSplitter)
				.Orientation(Orient_Vertical)
				+ SSplitter::Slot()
				.Value(8.0f)
				[
					// Locked files tree
					SNew(SBorder)
					.BorderImage(FCoreStyle::Get().GetBrush("ToolPanel.GroupBorder"))
					[
						// TODO: Add Scroll
						SAssignNew(LockedTreePtr, SCheckingFileStatesTree)
						.HeaderText("locked files")
						.RootList(&LockedTreeData)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			.Padding(5.f)
			[
				SNew( SBorder )
				[
					SNew(SVerticalBox)
					+SVerticalBox::Slot()
					.Padding( FMargin(5.f, 5.f, 5.f, 2.f) )
					[
						SNew( SHorizontalBox )
						+SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew(SCheckBox)
							.OnCheckStateChanged_Raw( this, &SLockedTab::OnForceCheckedStateChanged )
							.IsChecked_Raw( this, &SLockedTab::GetForceCheckBoxState )
						]
						+SHorizontalBox::Slot()
						.FillWidth( 1.f )
						.HAlign( HAlign_Center )
						[
							SNew( STextBlock )
							.Text( LOCTEXT( "Force", "Force" ) )
						]
					]
					+SVerticalBox::Slot()
					.Padding( FMargin(5.f, 2.f, 5.f, 5.f) )
					[
						SNew(SButton)
						.HAlign(HAlign_Center)
						.Text(this, &SLockedTab::GetUnlockButtonText)
						.IsEnabled_Raw(this, &SLockedTab::GetUnlockButtonEnable)
						.OnClicked_Raw(this, &SLockedTab::UnlockClicked)
					]
				]
			]
			+ SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SProgressBar)
				.Visibility_Raw(this, &SLockedTab::GetUnlockBusyIndicatorVisibility)
			]
		]
		+ SOverlay::Slot()
		[
			SNew(SColorBlock)
			.Color(FLinearColor::FromSRGBColor(FColor(100, 100, 100, 80)))
			.Visibility(this, &SLockedTab::GetLockedStateBusyIndicatorVisibility)
		]
		+ SOverlay::Slot()
		.HAlign(HAlign_Center)
		.VAlign(VAlign_Center)
		[
			SNew(SCircularThrobber)
			.Visibility(this, &SLockedTab::GetLockedStateBusyIndicatorVisibility)
		]
	];

	SourceControlStateChanged_Handle();
}

void SLockedTab::SourceControlStateChanged_Handle()
{
	TMap<FString, TSharedRef<ISourceControlState, ESPMode::ThreadSafe>> FullStatesMap
		= FVSPGitProvider::AccessProvider().GetFilesStates();

	// We filter all known states and get only locked ones.
	TMap<FString, TSharedRef<ISourceControlState, ESPMode::ThreadSafe>> LockedStatesMap;
	for (auto&& StatesPair : FullStatesMap)
	{
		TSharedRef<FVSPGitState, ESPMode::ThreadSafe> GitState = StaticCastSharedRef<FVSPGitState>(StatesPair.Value);
		if (GitState->bIsLocked)
			LockedStatesMap.Add(StatesPair.Key, StatesPair.Value);
	}

	// We get a list of current known states and actual changed states
	TArray<FString> ActualItems;
	LockedStatesMap.GetKeys(ActualItems);
	TArray<FString> CurrentItems = LockedTreePtr->GetAllFiles();

	{
		// Putting a list to remove from the tree.
		TArray<FString> RemoveStatesPaths;
		for (FString& ItemFromTree : CurrentItems)
		{
			FString* Founded = ActualItems.FindByPredicate([ItemFromTree](const FString& FilePath)
				{
					return FilePath.Equals(ItemFromTree);
				});
			if (Founded == nullptr)
			{
				RemoveStatesPaths.Add(ItemFromTree);
			}
		}

		// And remove them
		for (const FString& RemovePath : RemoveStatesPaths)
		{
			TArray<FString> TreePath;
			const TCHAR* Delims[] = { TEXT("\\"), TEXT("/") };
			RemovePath.ParseIntoArray(TreePath, Delims, 2);

			TSharedPtr<FCheckingDirectoryItem> CurrentParent = nullptr;
			TArray<TSharedPtr<FCheckingTreeItemBase>>* CurrentLevel = &LockedTreeData;
			for (const FString& TreeBranchPath : TreePath)
			{
				TSharedPtr<FCheckingTreeItemBase>* FoundedBranch = CurrentLevel->FindByPredicate(
					[TreeBranchPath](const TSharedPtr<FCheckingTreeItemBase> TreeItem)
					{
						return TreeItem->GetDisplayName().Equals(TreeBranchPath);
					});

				check(FoundedBranch != nullptr);

				if (!(*FoundedBranch)->IsFile())
				{
					TSharedPtr<FCheckingDirectoryItem> FoundedFolder = StaticCastSharedPtr<FCheckingDirectoryItem>(*FoundedBranch);
					CurrentLevel = &FoundedFolder->AccessSubItems();
					CurrentParent = FoundedFolder;
				}
				else
				{
					TSharedPtr<FCheckingFileStateItem> FoundedRemovingItem = StaticCastSharedPtr<FCheckingFileStateItem>(*FoundedBranch);
					CurrentLevel->Remove(FoundedRemovingItem);
				}
			}

			// Removing empty folders
			while (CurrentParent != nullptr)
			{
				if (CurrentParent->GetSubItems().Num() == 0)
				{
					TSharedPtr<FCheckingDirectoryItem> GrandParent = nullptr;
					if (CurrentParent->GetParentItem() != nullptr)
					{
						GrandParent = StaticCastSharedPtr<FCheckingDirectoryItem>(CurrentParent->GetParentItem());
						CurrentLevel = &GrandParent->AccessSubItems();
					}
					else
					{
						GrandParent = nullptr;
						CurrentLevel = &LockedTreeData;
					}

					CurrentLevel->Remove(CurrentParent);
					CurrentParent = GrandParent;
				}
				else
				{
					break;
				}
			}
		}
	}

	{
		// Putting to tree a new items paths and exist
		TArray<FString> NewItemsPaths;
		TArray<FString> ExistItemsPaths;
		for (FString& ActualItemPath : ActualItems)
		{
			FString* Founded = CurrentItems.FindByPredicate(
				[ActualItemPath](const FString& FilePath)
				{
					return FilePath.Equals(ActualItemPath);
				});
			if (Founded == nullptr)
			{
				NewItemsPaths.Add(ActualItemPath);
			}
			else
			{
				ExistItemsPaths.Add(ActualItemPath);
			}
		}

		// Update exist states
		for (FString& ExistItemPath : ExistItemsPaths)
		{
			TSharedPtr<FCheckingTreeItemBase> Item = LockedTreePtr->GetItemByPath(ExistItemPath);
			if ((Item != nullptr) && (Item->IsFile()))
			{
				TSharedPtr<FCheckingFileStateItem> FileItem = StaticCastSharedPtr<FCheckingFileStateItem>(Item);
				check(FileItem.IsValid())

					TSharedRef<ISourceControlState, ESPMode::ThreadSafe>* State = LockedStatesMap.Find(ExistItemPath);
				check(State != nullptr)

					FileItem->UpdateState(*State);
			}
		}

		// And add them
		for (FString& NewItemPath : NewItemsPaths)
		{
			TArray<FString> TreePath;
			const TCHAR* Delims[] = { TEXT("\\"), TEXT("/") };
			NewItemPath.ParseIntoArray(TreePath, Delims, 2);

			TSharedPtr<FCheckingDirectoryItem> CurrentParent = nullptr;
			TArray<TSharedPtr<FCheckingTreeItemBase>>* CurrentLevel = &LockedTreeData;
			for (const FString& TreeBranchPath : TreePath)
			{
				TSharedPtr<FCheckingTreeItemBase>* Founded = CurrentLevel->FindByPredicate([TreeBranchPath](TSharedPtr<FCheckingTreeItemBase> ItemBase)
					{
						return ItemBase->GetDisplayName().Equals(TreeBranchPath);
					});
				bool bIsFile = TreePath.IndexOfByKey(TreeBranchPath) == (TreePath.Num() - 1);
				if (Founded != nullptr)
				{
					if (!(*Founded)->IsFile())
					{
						TSharedPtr<FCheckingDirectoryItem> DirectoryItem = StaticCastSharedPtr<
							FCheckingDirectoryItem>(*Founded);

						check(DirectoryItem.IsValid())

							CurrentParent = DirectoryItem;
						CurrentLevel = &DirectoryItem->AccessSubItems();
					}

				}
				else
				{
					if (bIsFile)
					{
						TSharedRef<ISourceControlState, ESPMode::ThreadSafe>* State = LockedStatesMap.Find(
							NewItemPath);

						check(State != nullptr)

							TSharedPtr<FCheckingFileStateItem> FileStateItem =
							MakeShareable(new FCheckingFileStateItem(CurrentParent, *State));

						if (CurrentParent == nullptr)
						{
							LockedTreeData.Add(FileStateItem);
						}
						else
						{
							CurrentParent->AddSubItem(FileStateItem);
						}

						// Expand item
						while (CurrentParent != nullptr)
						{
							LockedTreePtr->ExpandItem(CurrentParent, true);
							CurrentParent = StaticCastSharedPtr<FCheckingDirectoryItem>(CurrentParent->GetParentItem());
						}
					}
					else
					{
						TSharedPtr<FCheckingDirectoryItem> DirectoryItem =
							MakeShareable(new FCheckingDirectoryItem(CurrentParent, TreeBranchPath));
						if (CurrentParent == nullptr)
						{
							LockedTreeData.Add(DirectoryItem);
						}
						else
						{
							CurrentParent->AddSubItem(DirectoryItem);
						}
						CurrentParent = DirectoryItem;
						CurrentLevel = &DirectoryItem->AccessSubItems();
					}
				}
			}
		}
	}

	LockedTreePtr->RedrawTree();
}

FText SLockedTab::GetUnlockButtonText() const
{
	return FText::FromString(FString::Printf(TEXT("Unlock %d files"), LockedTreePtr->GetCheckedFiles().Num()));
}

bool SLockedTab::GetUnlockButtonEnable() const
{
	return LockedTreePtr->GetCheckedFiles().Num() > 0 && !bIsUnlockingBusy;
}

EVisibility SLockedTab::GetUnlockBusyIndicatorVisibility() const
{
	return bIsUnlockingBusy ? EVisibility::Visible : EVisibility::Collapsed;
}

EVisibility SLockedTab::GetLockedStateBusyIndicatorVisibility() const
{
	const TArray<FVSPGitWork*> ActiveWorks = FVSPGitProvider::AccessProvider().GetWorksQueue();

	const int32 IndexWorkInQueue = ActiveWorks.FindLastByPredicate([](FVSPGitWork* Work)
		{
			return Work->Operation->GetName().IsEqual("UpdateStatus");
		});

	return (IndexWorkInQueue != INDEX_NONE) ? EVisibility::Visible : EVisibility::Collapsed;
}

void SLockedTab::OnForceCheckedStateChanged( ECheckBoxState InCheckBoxState )
{
	if ( InCheckBoxState == ECheckBoxState::Checked )
		bForce = true;
	else
		bForce = false;
}

ECheckBoxState SLockedTab::GetForceCheckBoxState() const
{
	return bForce ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;
}

void SLockedTab::PromptForUnlock()
{
	TArray<FString> CheckedFiles = LockedTreePtr->GetCheckedFiles();
	if (CheckedFiles.Num() == 0)
		return;

	TSharedRef<GitHighLevelWorkers::FUnlock, ESPMode::ThreadSafe> UnlockOperation = GitHighLevelWorkers::FUnlock::Create
		<GitHighLevelWorkers::FUnlock>();

	bool bHaveNotMeLocked = false;
	TMap<FString, TSharedRef<ISourceControlState, ESPMode::ThreadSafe>> StateMap = FVSPGitProvider::AccessProvider().
		GetFilesStates();
	for (FString& CheckedFile : CheckedFiles)
	{
		if (StateMap.Contains(CheckedFile))
		{
			TSharedRef<ISourceControlState, ESPMode::ThreadSafe>* State = StateMap.Find(CheckedFile);
			if (State != nullptr)
			{
				if ((*State)->IsCheckedOutOther())
				{
					bHaveNotMeLocked = true;
					break;
				}
			}
		}
	}

	if (bHaveNotMeLocked)
		UnlockOperation->SetForceArg( bForce );

	bIsUnlockingBusy = true;

	const ECommandResult::Type Result = FVSPGitProvider::AccessProvider().Execute(
		UnlockOperation,
		CheckedFiles,
		EConcurrency::Asynchronous,
		FSourceControlOperationComplete::CreateRaw(this, &SLockedTab::OnSourceControlOperationComplete));

	if (Result == ECommandResult::Succeeded)
		// Display an ongoing notification during the whole operation
		FVSPGitSlateLayout::Get().DisplayInProgressNotification(UnlockOperation->GetInProgressString());
	else
	{
		// Report failure with a notification
		FVSPGitSlateLayout::DisplayFailureNotification(UnlockOperation);
		bIsUnlockingBusy = false;
	}
}

FReply SLockedTab::UnlockClicked()
{
	PromptForUnlock();
	return FReply::Handled();
}

void SLockedTab::OnSourceControlOperationComplete( const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult )
{
	FVSPGitSlateLayout::Get().RemoveInProgressNotification();

	// Report result with a notification
	if (InResult == ECommandResult::Succeeded)
		FVSPGitSlateLayout::DisplaySuccessNotification(InOperation);
	else
		FVSPGitSlateLayout::DisplayFailureNotification(InOperation);

	bIsUnlockingBusy = false;
}

#undef LOCTEXT_NAMESPACE
