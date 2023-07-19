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
#include "CoreMinimal.h"
#include "SSectorWidget.h"
#include "Core/SectorProvider.h"
#include "SlateExtenderWidgets/SZoomablePanel.h"

class SMapLayoutView : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SMapLayoutView )
		: _SectorProvider()
	{}
		SLATE_ARGUMENT( TWeakPtr<FSectorProvider>, SectorProvider )
	SLATE_END_ARGS()

	~SMapLayoutView();

	void Construct( const FArguments& InArgs );

	virtual void OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;

	virtual bool SupportsKeyboardFocus() const override;
	virtual FReply OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;
	virtual FReply OnKeyUp( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent ) override;

	virtual FText GetToolTipText() const;

	void CalculateScale(
		const FVector2D& TopLeftViewport,
		const FVector2D& TopLeftAbsolute,
		const FVector2D& BottomRightViewport,
		const FVector2D& BottomRightAbsolute );
private:
	void RebuildToolTip( const TSharedPtr< FSectorViewModel >& InInfo );
	FText GetZoomLevelPercentText() const;
	void OnFitToWindowStateChanged( ECheckBoxState NewState ) const;
	ECheckBoxState OnGetFitToWindowState() const;
	FReply OnActualSizeClicked() const;
	FText OnGetSelectedItemText() const;

	void OnLayerSelected( TSharedPtr< IListItem > InItem, ESelectInfo::Type SelectInfo );

	const FSlateBrush* GetMapHolderBrush() const;
	void LoadMapHolder();

	FReply OnDeSelectClicked();
	FText GetSelectionInfo() const;
	EVisibility GetSelectedOperationsVisibility() const;
	FReply OnClickedLockSelected();
	FReply OnClickedUnlockSelected();

	TSharedRef< SWidget > HandleComboBoxGenerateWidget( TSharedPtr< IListItem > InItem );
	FText HandleMapsComboBoxText() const;
	FText HandleLayersComboBoxText() const;
	float GetZoomLevel() const;

private:
	FText HoveredSectionSlotToolTip;
	TSharedPtr< SComboBox< TSharedPtr< IListItem > > > MapsComboBox = nullptr;
	TSharedPtr< SComboBox< TSharedPtr< IListItem > > > LayersComboBox = nullptr;
	TArray< TSharedPtr< SSectorWidget > > SectorWidgetList;
	TSharedPtr< SZoomablePanel > ScrollPanel = nullptr;
	TWeakPtr< FSectorProvider > SectorProvider = nullptr;
	TSharedPtr< SOverlay > SectorsViewLayer = nullptr;

	bool bIsCtrlHolded = false;

	FVector2D ScaleVM;
	FVector2D OffsetVM;

	TSharedPtr< FSlateDynamicImageBrush > DynamicBrush = nullptr;
};
