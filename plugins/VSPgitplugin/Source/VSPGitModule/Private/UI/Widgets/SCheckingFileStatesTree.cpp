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
#include "SCheckingFileStatesTree.h"

#define LOCTEXT_NAMESPACE "SCheckingTree"

void SCheckingFileStatesTree::Construct( const FArguments& InArgs )
{
	HeaderText = InArgs._HeaderText.Get();
	RootList = InArgs._RootList;

	ChildSlot
	[
		SNew( SVerticalBox )
		+ SVerticalBox::Slot()
		.AutoHeight()
		[
			// Header changed files
			SNew( SBorder )
			.ToolTipText( this, &SCheckingFileStatesTree::GetCheckedNumString )
			[
				SNew( SHorizontalBox )
				+ SHorizontalBox::Slot()
				.Padding( 5 )
				.AutoWidth()
				[
					// Select/deselect all checkbox
					SNew( SCheckBox )
					.OnCheckStateChanged_Raw( this, &SCheckingFileStatesTree::OnCommonCheckedStateChanged )
					.IsChecked_Raw( this, &SCheckingFileStatesTree::GetCommonCheckBoxState )
				]
				+ SHorizontalBox::Slot()
				.FillWidth( 1.0f )
				.HAlign( HAlign_Center )
				[
					SNew( SHorizontalBox )
					+ SHorizontalBox::Slot()
					.Padding( 5.0f, 3.0f )
					.VAlign( VAlign_Center )
					.AutoWidth()
					[
						SNew( STextBlock )
						.Text( this, &SCheckingFileStatesTree::GetAllNumString )
					]
					+ SHorizontalBox::Slot()
					.VAlign( VAlign_Center )
					.AutoWidth()
					[
						SNew( STextBlock )
						.Text( FText::FromString( HeaderText ) )
					]
				]
			]
		]
		+ SVerticalBox::Slot()
		.FillHeight( 1.0f )
		[
			SNew( SBorder )
			.BorderImage( FCoreStyle::Get().GetBrush( "ToolPanel.GroupBorder" ) )
			[
				// Checked files tree
				SAssignNew( TreeView, SFilesTreeView )
				.SelectionMode( ESelectionMode::Multi )
				.ClearSelectionOnClick( false )
				.OnExpansionChanged( this, &SCheckingFileStatesTree::OnExpansionChanged )
				.TreeItemsSource( RootList )
				.OnGenerateRow( this, &SCheckingFileStatesTree::OnGeneratedRow )
				.OnGetChildren( this, &SCheckingFileStatesTree::OnGetChildren )
				.OnSelectionChanged( this, &SCheckingFileStatesTree::OnSelectionChanges )
			]
		]
	];
}

void SCheckingFileStatesTree::ExpandAll() const
{
	for ( TSharedPtr< FCheckingTreeItemBase, ESPMode::Fast >& ItemBase : ( *RootList ) )
		ExpandItem( ItemBase, true, true );
}

void SCheckingFileStatesTree::ExpandItem(
	const TSharedPtr< FCheckingTreeItemBase >& ItemPtr,
	bool bInShouldExpandItem,
	bool bInShouldExpandChild ) const
{
	TreeView->SetItemExpansion( ItemPtr, bInShouldExpandItem );

	TSharedPtr< FCheckingDirectoryItem > DirectoryItem = StaticCastSharedPtr< FCheckingDirectoryItem >( ItemPtr );
	if ( ( DirectoryItem.IsValid() ) && ( !DirectoryItem->IsFile() ) && ( bInShouldExpandChild ) )
		for ( const TSharedPtr< FCheckingTreeItemBase >& SubItem : DirectoryItem->GetSubItems() )
			ExpandItem( SubItem, bInShouldExpandItem, bInShouldExpandChild );
}

void SCheckingFileStatesTree::OnExpansionChanged(
	TSharedPtr< FCheckingTreeItemBase > Item,
	bool bIsExpanded )
{
	TSharedPtr< FCheckingDirectoryItem > DirectoryItem = StaticCastSharedPtr< FCheckingDirectoryItem >( Item );
	if ( ( DirectoryItem.IsValid() ) && ( !DirectoryItem->IsFile() ) )
		DirectoryItem->SetIsExpanded( bIsExpanded );
}

FText SCheckingFileStatesTree::GetCheckedNumString() const
{
	const TArray< FString > CheckedFiles = GetCheckedFiles();
	return FText::FromString( FString::Printf( TEXT( "%d files checked" ), CheckedFiles.Num() ) );
}

FText SCheckingFileStatesTree::GetAllNumString() const
{
	const TArray< FString > Files = GetAllFiles();
	return FText::FromString( FString::Printf( TEXT( "%d" ), Files.Num() ) );
}

void SCheckingFileStatesTree::OnCommonCheckedStateChanged( ECheckBoxState InCheckBoxState )
{
	if ( RootList == nullptr )
		return;

	for ( TSharedPtr< FCheckingTreeItemBase, ESPMode::Fast >& Item : ( *RootList ) )
		Item->OnCheckedStateChanged( InCheckBoxState );
}

ECheckBoxState SCheckingFileStatesTree::GetCommonCheckBoxState() const
{
	if ( RootList == nullptr )
		return ECheckBoxState::Undetermined;

	if ( ( *RootList ).Num() == 0 )
		return ECheckBoxState::Unchecked;

	ECheckBoxState LastState = ( *RootList )[ 0 ]->GetCheckBoxState();
	for ( auto&& TreeItem : ( *RootList ) )
	{
		const ECheckBoxState CurrState = TreeItem->GetCheckBoxState();
		if ( CurrState != LastState )
			return ECheckBoxState::Undetermined;
		LastState = CurrState;
	}

	return LastState;
}

TSharedRef< ITableRow > SCheckingFileStatesTree::OnGeneratedRow(
	TSharedPtr< FCheckingTreeItemBase > Item,
	const TSharedRef< STableViewBase >& OwnerTable )
{
	if ( !Item.IsValid() )
		return SNew( STableRow< TSharedPtr< FCheckingTreeItemBase > >, OwnerTable )
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			[
				SNew( STextBlock )
				.Text( LOCTEXT( "THISWASNULLSOMEHOW", "THIS WAS NULL SOMEHOW" ) )
				.ColorAndOpacity( FLinearColor::Red )
			]
		];

	return SNew( STableRow< TSharedPtr< FCheckingTreeItemBase > >, OwnerTable )
		.Padding( 2.0f )
		[
			SNew( SHorizontalBox )
			+ SHorizontalBox::Slot()
			.VAlign( VAlign_Center )
			.AutoWidth()
			[
				SNew( SCheckBox )
				.OnCheckStateChanged_Raw( Item.Get(), &FCheckingTreeItemBase::OnCheckedStateChanged )
				.IsChecked_Raw( Item.Get(), &FCheckingTreeItemBase::GetCheckBoxState )
			]
			+ SHorizontalBox::Slot()
			.Padding( 5.0f, 2.0f )
			.VAlign( VAlign_Center )
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride( 18 )
				.HeightOverride( 16 )
				[
					SNew( SImage )
					.Image_Raw( Item.Get(), &FCheckingTreeItemBase::GetItemIcon )
				]
			]
			+ SHorizontalBox::Slot()
			.VAlign( VAlign_Center )
			.FillWidth( 1.0f )
			[
				SNew( STextBlock )
				.Text( FText::FromString( Item->GetDisplayName() ) )
				.ToolTipText( Item->GetTextPath() )
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew( SBox )
				.WidthOverride( 18 )
				.HeightOverride( 18 )
				[
					SNew( SImage )
					.Image_Raw( Item.Get(), &FCheckingTreeItemBase::GetStateImage /*FEditorStyle::GetBrush(Status->GetIconName())*/ )
					.ToolTipText_Raw( Item.Get(), &FCheckingTreeItemBase::GetStateToolTip/*Status->GetDisplayTooltip()*/ )
				]
			]
		];
}

void SCheckingFileStatesTree::OnGetChildren(
	TSharedPtr< FCheckingTreeItemBase > Item,
	TArray< TSharedPtr< FCheckingTreeItemBase > >& OutChildren )
{
	TSharedPtr< FCheckingDirectoryItem > DirectoryItem = StaticCastSharedPtr< FCheckingDirectoryItem >( Item );
	if ( ( !DirectoryItem.IsValid() ) || ( DirectoryItem->IsFile() ) )
		return;

	OutChildren.Append( DirectoryItem->GetSubItems() );
}

void SCheckingFileStatesTree::OnSelectionChanges(
	TSharedPtr< FCheckingTreeItemBase > Item,
	ESelectInfo::Type SelectInfo ) const
{
	if ( !Item.IsValid() )
		return;

	// TODO:
}

TArray< FString > SCheckingFileStatesTree::GetCheckedFiles() const
{
	TArray< FString > CheckedFiles;
	for ( TSharedPtr< FCheckingTreeItemBase, ESPMode::Fast >& ItemBase : ( *RootList ) )
		ItemBase->GetCheckedFiles( CheckedFiles );

	return CheckedFiles;
}

TArray< FString > SCheckingFileStatesTree::GetAllFiles() const
{
	TArray< FString > Files;
	for ( TSharedPtr< FCheckingTreeItemBase, ESPMode::Fast >& ItemBase : ( *RootList ) )
		ItemBase->GetAllFiles( Files );

	return Files;
}

TSharedPtr< FCheckingTreeItemBase > SCheckingFileStatesTree::GetItemByPath( const FString& InPath )
{
	TArray< FString > TreePath;
	const TCHAR* Delims[ ] = { TEXT( "\\" ), TEXT( "/" ) };
	InPath.ParseIntoArray( TreePath, Delims, 2 );

	TSharedPtr< FCheckingTreeItemBase >* Founded = nullptr;
	TArray< TSharedPtr< FCheckingTreeItemBase > >* CurrentLevel = RootList;
	for ( FString& TreeBranch : TreePath )
	{
		Founded = CurrentLevel->FindByPredicate(
			[TreeBranch]( TSharedPtr< FCheckingTreeItemBase > ItemBase )
			{
				return ItemBase->GetDisplayName().Equals( TreeBranch );
			} );
		if ( Founded != nullptr )
		{
			if ( !( *Founded )->IsFile() )
			{
				TSharedPtr< FCheckingDirectoryItem > DirectoryItem = StaticCastSharedPtr<
					FCheckingDirectoryItem >( *Founded );

				check( DirectoryItem.IsValid() )

				CurrentLevel = &DirectoryItem->AccessSubItems();
			}
		}
		else
			return nullptr;
	}

	return *Founded;
}

void SCheckingFileStatesTree::RedrawTree()
{
	TreeView->RequestTreeRefresh();
}

#undef LOCTEXT_NAMESPACE
