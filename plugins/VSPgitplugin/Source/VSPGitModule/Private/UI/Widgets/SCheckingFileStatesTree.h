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

#include "SlateBasics.h"
#include "UI/CheckingFileStateItem.h"

typedef STreeView< TSharedPtr< FCheckingTreeItemBase > > SFilesTreeView;

class SCheckingFileStatesTree : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SCheckingFileStatesTree )
	{}
		SLATE_ARGUMENT( TArray< TSharedPtr<FCheckingTreeItemBase> >*, RootList )
		SLATE_ATTRIBUTE( FString, HeaderText )
	SLATE_END_ARGS()

public:
	/** Constructs the widget */
	void Construct( const FArguments& InArgs );

	void ExpandAll() const;
	void ExpandItem(
		const TSharedPtr< FCheckingTreeItemBase >& ItemPtr,
		bool bInShouldExpandItem,
		bool bInShouldExpandChild = false ) const;

	void OnExpansionChanged( TSharedPtr< FCheckingTreeItemBase > Item, bool bIsExpanded );

	FText GetCheckedNumString() const;
	FText GetAllNumString() const;

	void OnCommonCheckedStateChanged( ECheckBoxState InCheckBoxState );
	ECheckBoxState GetCommonCheckBoxState() const;

	TSharedRef< ITableRow > OnGeneratedRow(
		TSharedPtr< FCheckingTreeItemBase > Item,
		const TSharedRef< STableViewBase >& OwnerTable );
	void OnGetChildren(
		TSharedPtr< FCheckingTreeItemBase > Item,
		TArray< TSharedPtr< FCheckingTreeItemBase > >& OutChildren );

	void OnSelectionChanges(
		TSharedPtr< FCheckingTreeItemBase > Item,
		ESelectInfo::Type SelectInfo ) const;

	TArray< FString > GetCheckedFiles() const;
	TArray< FString > GetAllFiles() const;

	TSharedPtr< FCheckingTreeItemBase > GetItemByPath( const FString& InPath );

	void RedrawTree();

private:
	TSharedPtr< SFilesTreeView > TreeView;

	TArray< TSharedPtr< FCheckingTreeItemBase > >* RootList = nullptr;
	FString HeaderText;
};
