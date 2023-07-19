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
#include "CheckingFileStateItem.h"

TSharedPtr< FCheckingTreeItemBase > FCheckingTreeItemBase::GetParentItem()
{
	return ParentItem.Pin();
}

bool FCheckingTreeItemBase::IsChecked()
{
	return CheckBoxState == ECheckBoxState::Checked;
}

FString FCheckingDirectoryItem::GetDisplayName()
{
	return DirectoryName;
}

bool FCheckingDirectoryItem::IsExpanded() const
{
	return bIsExpanded;
}

void FCheckingDirectoryItem::SetIsExpanded( bool bInIsExpanded )
{
	bIsExpanded = bInIsExpanded;
}

TArray< TSharedPtr< FCheckingTreeItemBase > >& FCheckingDirectoryItem::AccessSubItems()
{
	return SubItems;
}

const TArray< TSharedPtr< FCheckingTreeItemBase > >& FCheckingDirectoryItem::GetSubItems()
{
	return SubItems;
}

void FCheckingDirectoryItem::AddSubItem( const TSharedPtr< FCheckingTreeItemBase >& NewSubItemPtr )
{
	SubItems.Add( NewSubItemPtr );
}

ECheckBoxState FCheckingDirectoryItem::GetCheckBoxState() const
{
	int32 CheckedCount = 0, UncheckedCount = 0;
	for ( auto&& SubItem : SubItems )
	{
		const auto LastCheckStatus = SubItem->GetCheckBoxState();
		switch ( LastCheckStatus )
		{
			case ECheckBoxState::Unchecked:
				UncheckedCount++;
				break;
			case ECheckBoxState::Checked:
				CheckedCount++;
				break;
			case ECheckBoxState::Undetermined:
				return ECheckBoxState::Undetermined;
				break;
			default: ;
		}
	}

	if ( ( CheckedCount != 0 ) && ( UncheckedCount != 0 ) )
		return ECheckBoxState::Undetermined;
	if ( CheckedCount != 0 )
		return ECheckBoxState::Checked;
	if ( UncheckedCount != 0 )
		return ECheckBoxState::Unchecked;
	return ECheckBoxState::Undetermined;
}

void FCheckingDirectoryItem::OnCheckedStateChanged( ECheckBoxState InCheckBoxState )
{
	CheckBoxState = InCheckBoxState;

	for ( TSharedPtr< FCheckingTreeItemBase, ESPMode::Fast >& ItemPtr : SubItems )
		ItemPtr->OnCheckedStateChanged( InCheckBoxState );
}

FString FCheckingDirectoryItem::GetStringPath()
{
	if ( ParentItem.Pin() == nullptr )
		return DirectoryName;

	return FPaths::Combine( ParentItem.Pin()->GetStringPath(), DirectoryName );
}

FText FCheckingDirectoryItem::GetTextPath()
{
	return FText::FromString( GetStringPath() );
}

const FSlateBrush* FCheckingDirectoryItem::GetItemIcon() const
{
	return bIsExpanded
		? FEditorStyle::GetBrush( "ContentBrowser.AssetTreeFolderOpen" )
		: FEditorStyle::GetBrush( "ContentBrowser.AssetTreeFolderClosed" );
}

void FCheckingDirectoryItem::GetCheckedFiles( TArray< FString >& OutFilesList )
{
	for ( TSharedPtr< FCheckingTreeItemBase, ESPMode::Fast >& SubItem : SubItems )
		SubItem->GetCheckedFiles( OutFilesList );
}

void FCheckingDirectoryItem::GetAllFiles( TArray< FString >& OutFilesList )
{
	for ( TSharedPtr< FCheckingTreeItemBase, ESPMode::Fast >& SubItem : SubItems )
		SubItem->GetAllFiles( OutFilesList );
}

const FSlateBrush* FCheckingDirectoryItem::GetStateImage() const
{
	return nullptr;
}

FText FCheckingDirectoryItem::GetStateToolTip() const
{
	return FText();
}

FString FCheckingFileStateItem::GetDisplayName()
{
	return FString::Printf( TEXT( "%s.%s" ), *FPaths::GetBaseFilename( FileState->GetFilename() ), *FPaths::GetExtension( FileState->GetFilename() ) );
}

ECheckBoxState FCheckingFileStateItem::GetCheckBoxState() const
{
	return CheckBoxState;
}

void FCheckingFileStateItem::OnCheckedStateChanged( ECheckBoxState InCheckBoxState )
{
	CheckBoxState = InCheckBoxState;
}

FString FCheckingFileStateItem::GetStringPath()
{
	return FileState->GetFilename();
}

FText FCheckingFileStateItem::GetTextPath()
{
	return FText::FromString( GetStringPath() );
}

const FSlateBrush* FCheckingFileStateItem::GetItemIcon() const
{
	return FileState->GetFilename().EndsWith( TEXT( "umap" ) )
		? FEditorStyle::GetBrush( "LandscapeEditor.Target_Heightmap" ) /*"WorldBrowser.DetailsButtonBrush")*/
		: FEditorStyle::GetBrush( "ContentBrowser.NewAsset" );
}

void FCheckingFileStateItem::GetCheckedFiles( TArray< FString >& OutFilesList )
{
	if ( CheckBoxState == ECheckBoxState::Checked )
		OutFilesList.Add( FileState->GetFilename() );
}

void FCheckingFileStateItem::GetAllFiles( TArray< FString >& OutFilesList )
{
	OutFilesList.Add( FileState->GetFilename() );
}

const FSlateBrush* FCheckingFileStateItem::GetStateImage() const
{
	return FEditorStyle::GetBrush( FileState->GetIconName() );
}

FText FCheckingFileStateItem::GetStateToolTip() const
{
	return FileState->GetDisplayTooltip();
}

const TSharedRef< ISourceControlState, ESPMode::ThreadSafe >& FCheckingFileStateItem::GetState() const
{
	return FileState;
}

void FCheckingFileStateItem::UpdateState( const TSharedRef< ISourceControlState, ESPMode::ThreadSafe >& InState )
{
	FileState = InState;
}
