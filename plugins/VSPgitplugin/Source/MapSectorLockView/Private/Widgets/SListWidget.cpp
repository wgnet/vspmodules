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
#include "SListWidget.h"

#include "Widgets/Layout/SScrollBox.h"

void SListWidget::Construct( const FArguments& Args )
{
	HeaderText = Args._HeaderText.Get();
	Items = Args._Items;

	ChildSlot
	[
		SNew( SScrollBox )
		+ SScrollBox::Slot()
		[
			SNew( SBorder )
			.Padding( 5 )
			.HAlign( HAlign_Center )
			[
				SNew( STextBlock )
				.Text( HeaderText )
			]
		]
		+ SScrollBox::Slot()
		[
			SNew( SBorder )
			.BorderImage( FCoreStyle::Get().GetBrush( "ToolPanel.GroupBorder" ) )
			[
				SAssignNew( ListViewWidget, SListView<TSharedPtr<IListItem>> )
				.ItemHeight( 24 )
				.ListItemsSource( Items )
				.SelectionMode( ESelectionMode::Single )
				.OnSelectionChanged( Args._OnSelectionChanged )
				.OnGenerateRow( this, &SListWidget::OnGenerateRowForList )
			]
		]
	];
}

TSharedRef< ITableRow > SListWidget::OnGenerateRowForList(
	TSharedPtr< IListItem > Item,
	const TSharedRef< STableViewBase >& OwnerTable )
{
	//Create the row
	return SNew( STableRow< TSharedPtr<FString> >, OwnerTable )
		.Padding( FMargin( 10.f, 5.f ) )
		[
			SNew( STextBlock )
			.Text( Item->GetDisplayName() )
			.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 10 ) )
		];
}

void SListWidget::RequestListRefresh()
{
	ListViewWidget->RequestListRefresh();
}

void SListWidget::SetSelection( TSharedPtr< IListItem > InItem )
{
	ListViewWidget->SetSelection( InItem );
}

