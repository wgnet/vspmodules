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

#include "BrushUICustomzation.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SWindow.h"


struct FMaterialData;
class SButton;

/** Options window used to populate provided settings objects */
class SVSPMeshPaintPalette : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SVSPMeshPaintPalette)
	{
	}
	SLATE_ARGUMENT(TSharedPtr<SWindow>, WidgetWindow)
	SLATE_ARGUMENT(TArray<FTextureArrayPaletteItemModelPtr>, TextureArrayMaterialItems)
	SLATE_ARGUMENT(UVSPBrushToolProperties*, BrushProperties)
	SLATE_ARGUMENT(FTextureArrayPaletteItemModelPtr, PaletteItem)
	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs);

	/** Begin SCompoundWidget overrides */
	virtual bool SupportsKeyboardFocus() const override
	{
		return true;
	}
	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;
	/** End SCompoundWidget overrides */

	FReply OnConfirm();
	FReply OnCancel();
	void CloseWindow();

	bool WasUserCancelled();

private:
	TWeakPtr<SWindow> WidgetWindow;
	bool bUserCancelled = false;

	TSharedRef<ITableRow> GenerateMaterialTile(
		FTextureArrayPaletteItemModelPtr Item,
		const TSharedRef<STableViewBase>& OwnerTable);
	TArray<FTextureArrayPaletteItemModelPtr> TextureArrayMaterialItems;
	UVSPBrushToolProperties* BrushProperties {};
};
