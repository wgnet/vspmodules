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

#include "CoreMinimal.h"
#include "Slate/Public/Widgets/Views/SListView.h"

class IListItem
{
public:
	virtual ~IListItem() = default;

	virtual FText GetDisplayName()
	{
		return FText();
	};
};

class SListWidget : public SCompoundWidget
{
public:
	using NullableItemType = TListTypeTraits< TSharedPtr< IListItem > >::NullableType;

	using FOnSelectionChanged = TSlateDelegates< NullableItemType >::FOnSelectionChanged;

	SLATE_BEGIN_ARGS( SListWidget )
	{}
		SLATE_ATTRIBUTE( FText, HeaderText )
		SLATE_ARGUMENT( TArray<TSharedPtr<IListItem>>*, Items )
		SLATE_EVENT( FOnSelectionChanged, OnSelectionChanged )
	SLATE_END_ARGS()

	void Construct( const FArguments& Args );

	TSharedRef< ITableRow > OnGenerateRowForList( TSharedPtr< IListItem > Item, const TSharedRef< STableViewBase >& OwnerTable );

	void RequestListRefresh();
	void SetSelection( TSharedPtr< IListItem > InItem );

private:
	TArray< TSharedPtr< IListItem > >* Items = nullptr;
	FText HeaderText;

	TSharedPtr< SListView< TSharedPtr< IListItem > > > ListViewWidget;
};
