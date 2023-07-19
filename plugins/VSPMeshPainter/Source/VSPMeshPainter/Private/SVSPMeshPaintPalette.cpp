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

#include "SVSPMeshPaintPalette.h"
#include "IDetailTreeNode.h"
#include "Widgets/Layout/SUniformGridPanel.h"


#define LOCTEXT_NAMESPACE "SVSPMeshPaintPalette"

TSharedRef<ITableRow> SVSPMeshPaintPalette::GenerateMaterialTile(
	FTextureArrayPaletteItemModelPtr Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STextureArrayPaletteItemTile, OwnerTable, Item, false);
}


void SVSPMeshPaintPalette::Construct(const FArguments& InArgs)
{
	WidgetWindow = InArgs._WidgetWindow;
	TextureArrayMaterialItems = InArgs._TextureArrayMaterialItems;
	BrushProperties = InArgs._BrushProperties;

	this->ChildSlot
	[
		SNew( SVerticalBox )
		+SVerticalBox::Slot()
		.MaxHeight( 1300 )
		.VAlign( VAlign_Fill )
		.HAlign( HAlign_Fill )
		.Padding( 5 )
		.FillHeight( 100 )
		[
			SNew( STileView<FTextureArrayPaletteItemModelPtr> )
			.ListItemsSource( &TextureArrayMaterialItems )
			.OnGenerateTile( this, &SVSPMeshPaintPalette::GenerateMaterialTile )
			.SelectionMode( ESelectionMode::Single)
			.OnSelectionChanged_Lambda([InArgs, this](FTextureArrayPaletteItemModelPtr Item, ESelectInfo::Type SelectInfo)
			{
				if ( Item != nullptr && BrushProperties != nullptr && SelectInfo != ESelectInfo::Direct && BrushProperties && Item && InArgs._PaletteItem != nullptr )
				{
					InArgs._PaletteItem->SetTexture( Item->GetTextureIdx() );
					SVSPMeshPaintPalette::OnConfirm();
				}
			})
			.AllowOverscroll(EAllowOverscroll::No)
			.ItemAlignment(EListItemAlignment::Fill)
			.ItemWidth(128)
			.ItemHeight(128)
		]
	];
}

FReply SVSPMeshPaintPalette::OnConfirm()
{
	bUserCancelled = false;
	CloseWindow();

	return FReply::Handled();
}

void SVSPMeshPaintPalette::CloseWindow()
{
	if (WidgetWindow.IsValid())
	{
		WidgetWindow.Pin()->RequestDestroyWindow();
	}
	if (BrushProperties != nullptr && BrushProperties->ShutdownLambdaHandleForPalette.IsValid())
	{
		BrushProperties->OnShutdownDelegate.Remove(BrushProperties->ShutdownLambdaHandleForPalette);
		BrushProperties->ShutdownLambdaHandleForPalette.Reset();
		BrushProperties = nullptr;
	}
	WidgetWindow.Reset();
}

FReply SVSPMeshPaintPalette::OnCancel()
{
	bUserCancelled = true;

	CloseWindow();
	return FReply::Handled();
}

FReply SVSPMeshPaintPalette::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	if (InKeyEvent.GetKey() == EKeys::Escape)
	{
		return OnCancel();
	}

	return FReply::Unhandled();
}

bool SVSPMeshPaintPalette::WasUserCancelled()
{
	return bUserCancelled;
}

#undef LOCTEXT_NAMESPACE //"SVSPMeshPaintPalette"
