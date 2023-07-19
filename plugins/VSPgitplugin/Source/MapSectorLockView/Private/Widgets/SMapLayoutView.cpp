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
#include "SMapLayoutView.h"

#include "IVSPGitModule.h"
#include "Brushes/SlateDynamicImageBrush.h"
#include "MapSectorLockViewStyle.h"
#include "SSectorWidget.h"

#define LOCTEXT_NAMESPACE "FMapSectorLockViewModule"

namespace SMapLayoutViewLocal
{
	static int32 LockedCount = 0;
	static int32 NotLocked = 0;
}

SMapLayoutView::~SMapLayoutView()
{
	SectorsViewLayer = nullptr;
	SectorProvider = nullptr;
}

void SMapLayoutView::Construct( const FArguments& InArgs )
{
	SectorProvider = InArgs._SectorProvider;

	ChildSlot
	[
		SNew( SBorder )
		.ToolTipText( this, &SMapLayoutView::GetToolTipText )
		.BorderImage( FCoreStyle::Get().GetBrush( "Docking.Background" ) )
		[
			SNew( SOverlay )
			+ SOverlay::Slot()
			.Padding( 2.0f )
			[
				SAssignNew( ScrollPanel, SZoomablePanel )
				[
					SNew( SOverlay )
					+ SOverlay::Slot()
					[
						SNew( SImage )
						.Image( FCoreStyle::Get().GetBrush( "Checkerboard" ) )
					]
					+ SOverlay::Slot()
					[
						SNew( SImage )
						.Image_Raw( this, &SMapLayoutView::GetMapHolderBrush )
					]
					+ SOverlay::Slot()
					[
						SAssignNew( SectorsViewLayer, SOverlay )
					]
				]
			]
			+ SOverlay::Slot()
			.Padding( 4.0f )
			[
				SNew( SVerticalBox )
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( SBorder )
					.BorderImage( FCoreStyle::Get().GetBrush( "ToolPanel.GroupBorder" ) )
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.Padding( FMargin( 10.f, 2.f, 5.f, 2.f ) )
						.HAlign( HAlign_Right )
						.VAlign( VAlign_Center )
						[
							SNew( STextBlock )
							.Text( LOCTEXT( "Maps", "Maps:" ) )
						]
						+ SHorizontalBox::Slot()
						.Padding( 2.f )
						.HAlign( HAlign_Left )
						.VAlign( VAlign_Center )
						[
							SAssignNew( MapsComboBox, SComboBox< TSharedPtr<IListItem> > )
							.OptionsSource( &SectorProvider.Pin()->GetMapsItems() )
							.OnSelectionChanged(
								SectorProvider.Pin().ToSharedRef(),
								&FSectorProvider::OnMapSelectionChanged )
							.OnGenerateWidget( this, &SMapLayoutView::HandleComboBoxGenerateWidget )
							[
								SNew( STextBlock )
								.Text( this, &SMapLayoutView::HandleMapsComboBoxText )
							]
						]
						+ SHorizontalBox::Slot()
						.Padding( FMargin( 10.f, 2.f, 5.f, 2.f ) )
						.HAlign( HAlign_Right )
						.VAlign( VAlign_Center )
						[
							SNew( STextBlock )
							.Text( LOCTEXT( "Layers", "Layers:" ) )
						]
						+ SHorizontalBox::Slot()
						.Padding( 2.f )
						.HAlign( HAlign_Left )
						.VAlign( VAlign_Center )
						[
							SAssignNew( LayersComboBox, SComboBox< TSharedPtr<IListItem> > )
							.OptionsSource( &SectorProvider.Pin()->GetLayersItems() )
							.OnSelectionChanged( this, &SMapLayoutView::OnLayerSelected )
							.OnGenerateWidget( this, &SMapLayoutView::HandleComboBoxGenerateWidget )
							[
								SNew( STextBlock )
								.Text( this, &SMapLayoutView::HandleLayersComboBoxText )
							]
						]
						+ SHorizontalBox::Slot()
						.Padding( 2.f )
						.HAlign( HAlign_Fill )
						.FillWidth( 1.f )
						.VAlign( VAlign_Center )
						+ SHorizontalBox::Slot()
						.Padding( 2.0f )
						.HAlign( HAlign_Right )
						.VAlign( VAlign_Center )
						[
							SNew( STextBlock )
							.Text( this, &SMapLayoutView::GetZoomLevelPercentText )
						]
						+ SHorizontalBox::Slot()
						.Padding( 2.0f )
						.AutoWidth()
						.VAlign( VAlign_Center )
						[
							SNew( SCheckBox )
							.OnCheckStateChanged( this, &SMapLayoutView::OnFitToWindowStateChanged )
							.IsChecked( this, &SMapLayoutView::OnGetFitToWindowState )
							.Content()
							[
								SNew( STextBlock )
								.Text( LOCTEXT( "FitToWindow", "Fit to Window" ) )
							]
						]
						+ SHorizontalBox::Slot()
						.Padding( 2.0f )
						.AutoWidth()
						.VAlign( VAlign_Center )
						[
							SNew( SButton )
							.Text( LOCTEXT( "ActualSize", "Actual Size" ) )
							.OnClicked( this, &SMapLayoutView::OnActualSizeClicked )
						]
					]
				]
				+ SVerticalBox::Slot()
				.FillHeight( 1.0f )
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign( HAlign_Center )
				[
					SNew( SBorder )
					.BorderImage( FCoreStyle::Get().GetBrush( "ToolPanel.GroupBorder" ) )
					.Visibility( this, &SMapLayoutView::GetSelectedOperationsVisibility )
					[
						SNew( SHorizontalBox )
						+ SHorizontalBox::Slot()
						.AutoWidth()
						[
							SNew( SButton )
							.HAlign( HAlign_Center )
							.VAlign( VAlign_Center )
							.OnClicked( this, &SMapLayoutView::OnDeSelectClicked )
							[
								SNew( STextBlock )
								.Text( LOCTEXT( "UnsetSelection", "Unset selection" ) )
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding( 5.f, 2.f )
						[
							SNew( SSeparator )
							.Orientation( Orient_Vertical )
						]
						+ SHorizontalBox::Slot()
						.FillWidth( 1.f )
						.VAlign( VAlign_Center )
						[
							SNew( STextBlock )
							.Text( this, &SMapLayoutView::GetSelectionInfo )
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding( 5.f, 2.f )
						[
							SNew( SSeparator )
							.Orientation( Orient_Vertical )
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding( 1.f )
						[
							SNew( SButton )
							.IsEnabled_Static( [] { return SMapLayoutViewLocal::NotLocked > 0; } )
							.OnClicked( this, &SMapLayoutView::OnClickedLockSelected )
							[
								SNew( SImage )
								.Image( FEditorStyle::Get().GetBrush( "Level.LockedIcon16x" ) )
							]
						]
						+ SHorizontalBox::Slot()
						.AutoWidth()
						.Padding( 1.f )
						[
							SNew( SButton )
							.IsEnabled_Static( [] { return SMapLayoutViewLocal::LockedCount > 0; } )
							.OnClicked( this, &SMapLayoutView::OnClickedUnlockSelected )
							[
								SNew( SImage )
								.Image( FEditorStyle::Get().GetBrush( "Level.UnlockedIcon16x" ) )
							]
						]
					]
				]
			]
		]
	];

	OnLayerSelected( SectorProvider.Pin()->GetSelectedLayer(), ESelectInfo::Direct );
}

void SMapLayoutView::OnMouseEnter( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	HoveredSectionSlotToolTip = FText();
}

void SMapLayoutView::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	HoveredSectionSlotToolTip = FText();
	for ( TSharedPtr< SSectorWidget >& SectorWidgetPtr : SectorWidgetList )
		SectorWidgetPtr->MouseLeave();
}

FReply SMapLayoutView::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	if ( !MouseEvent.GetCursorDelta().IsNearlyZero() && !HasMouseCapture() )
	{
		const FVector2D LocalPos = MyGeometry.AbsoluteToLocal( MouseEvent.GetScreenSpacePosition() ) / ScrollPanel->
			GetZoomLevel();

		for ( TSharedPtr< SSectorWidget >& SectorWidgetPtr : SectorWidgetList )
			SectorWidgetPtr->HoveredTest( LocalPos );
		bool bIsSomeHovered = false;
		for ( TSharedPtr< SSectorWidget >& SectorWidgetPtr : SectorWidgetList )
			if ( SectorWidgetPtr->IsHovered() )
			{
				bIsSomeHovered = true;
				RebuildToolTip( SectorWidgetPtr->GetSectorViewModel() );
			}
		if ( !bIsSomeHovered )
			HoveredSectionSlotToolTip = FText();

		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SMapLayoutView::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	for ( TSharedPtr< SSectorWidget, ESPMode::Fast >& SectorWidget : SectorWidgetList )
		if ( SectorWidget->IsHovered() )
		{
			TSharedPtr< FSectorViewModel >& SectorVm = SectorWidget->GetSectorViewModel();
			if ( SectorProvider.Pin()->IsSelectedSector( SectorVm ) )
			{
				if ( bIsCtrlHolded )
					SectorProvider.Pin()->RemoveSelection( SectorVm );
				else
					SectorProvider.Pin()->DeSelect();
			}
			else
			{
				if ( bIsCtrlHolded )
					SectorProvider.Pin()->AddSelectedSector( SectorVm );
				else
					SectorProvider.Pin()->SetSelectedSector( SectorVm );
			}
		}

	return FReply::Handled();
}

bool SMapLayoutView::SupportsKeyboardFocus() const
{
	return true;
}

FReply SMapLayoutView::OnKeyDown( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	const FKey PushedKey = InKeyEvent.GetKey();
	if ( ( PushedKey == EKeys::LeftControl ) || ( PushedKey == EKeys::RightControl ) )
	{
		bIsCtrlHolded = true;
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FReply SMapLayoutView::OnKeyUp( const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent )
{
	const FKey ReleasedKey = InKeyEvent.GetKey();
	if ( ( ReleasedKey == EKeys::LeftControl ) || ( ReleasedKey == EKeys::RightControl ) )
	{
		bIsCtrlHolded = false;
		return FReply::Handled();
	}

	return FReply::Unhandled();
}

FText SMapLayoutView::GetToolTipText() const
{
	return HoveredSectionSlotToolTip;
}

void SMapLayoutView::RebuildToolTip( const TSharedPtr< FSectorViewModel >& InInfo )
{
	FString ToolTipString = FString::Printf(
		TEXT( "Map: %s\nPath: \"%s\"\n" ),
		*InInfo->GetDisplayName().ToString(),
		*InInfo->GetLinkedMapContentPath() );

	if ( InInfo->GetLockState() != EMapLockState::NotLocked )
		ToolTipString += FString::Printf( TEXT( "\nLocked by %s" ), *InInfo->GetLockUserName() );

	HoveredSectionSlotToolTip = FText::FromString( ToolTipString );
}

FText SMapLayoutView::GetZoomLevelPercentText() const
{
	if ( ScrollPanel.IsValid() )
		return FText::Format(
			LOCTEXT( "ZoomLevelPercent", "Zoom Level: {0}" ),
			FText::AsPercent( ScrollPanel->GetZoomLevel() ) );

	return FText::GetEmpty();
}

void SMapLayoutView::OnFitToWindowStateChanged( ECheckBoxState NewState ) const
{
	if ( ScrollPanel.IsValid() )
	{
		if ( NewState == ECheckBoxState::Checked )
			ScrollPanel->FitToWindow();
		else
			ScrollPanel->FitToSize();
	}
}

ECheckBoxState SMapLayoutView::OnGetFitToWindowState() const
{
	if ( ScrollPanel.IsValid() )
		return ( ScrollPanel->IsFitToWindow() ) ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;

	return ECheckBoxState::Undetermined;
}

FReply SMapLayoutView::OnActualSizeClicked() const
{
	if ( ScrollPanel.IsValid() )
		ScrollPanel->FitToSize();

	return FReply::Handled();
}

FText SMapLayoutView::OnGetSelectedItemText() const
{
	// TODO:
	return LOCTEXT( "SelectAPage", "Select a page" );
}

void SMapLayoutView::CalculateScale(
	const FVector2D& TopLeftViewport,
	const FVector2D& TopLeftAbsolute,
	const FVector2D& BottomRightViewport,
	const FVector2D& BottomRightAbsolute )
{
	ScaleVM = ( BottomRightViewport - TopLeftViewport ) / ( BottomRightAbsolute - TopLeftAbsolute );
	//(BottomRightAbsolute - TopLeftAbsolute) / (BottomRightRelative - TopLeftRelative);
	OffsetVM = TopLeftAbsolute; //TopLeftAbsolute - TopLeftRelative * ScaleVM;
}

void SMapLayoutView::OnLayerSelected( TSharedPtr< IListItem > InItem, ESelectInfo::Type SelectInfo )
{
	SectorProvider.Pin()->OnLayerSelectionChanged( InItem, SelectInfo );

	const TSharedPtr< IListItem > LayerItem = SectorProvider.Pin()->GetSelectedLayer();
	SectorsViewLayer->ClearChildren();
	SectorWidgetList.Empty();

	if ( LayerItem == nullptr )
		return;

	LoadMapHolder();

	TSharedPtr< FLayerViewModel > LayerVm = StaticCastSharedPtr< FLayerViewModel >( LayerItem );
	CalculateScale(
		FVector2D( 0, 0 ),
		LayerVm->GetTopLeftCornerPoint(),
		LayerVm->GetSize(),
		LayerVm->GetBottomRightCornerPoint() );

	TArray< TSharedPtr< FSectorViewModel > >& Sectors = SectorProvider.Pin()->GetSectors();
	for ( TSharedPtr< FSectorViewModel >& SectorViewModel : Sectors )
	{
		SectorWidgetList.Add(
			SNew( SSectorWidget )
			.SectorViewModel( SectorViewModel )
			.ZoomCoefficient_Raw( this, &SMapLayoutView::GetZoomLevel )
			.PhysicalOffset_Raw( ScrollPanel.Get(), &SZoomablePanel::GetPhysicalOffset )
			.OffsetVM( OffsetVM )
			.ScaleVM( ScaleVM )
			.ImageSize( LayerVm->GetSize() )
		);
		SOverlay::FOverlaySlot& Slot = SectorsViewLayer->AddSlot();
		Slot.AttachWidget( SectorWidgetList.Last().ToSharedRef() );
	}
}

void SMapLayoutView::LoadMapHolder()
{
	if ( !SectorProvider.Pin().IsValid() )
		return;

	const TSharedPtr< IListItem > Layout = SectorProvider.Pin()->GetSelectedLayer();
	TSharedPtr< FLayerViewModel > LayerVm = StaticCastSharedPtr< FLayerViewModel >( Layout );

	FString Filename = LayerVm->GetImageBrushName();

	if ( FPaths::IsRelative( Filename ) )
		Filename = FPaths::Combine( FPaths::ProjectDir(), Filename );
	if ( FPaths::IsRelative( Filename ) )
		Filename = FPaths::ConvertRelativePathToFull( Filename );

	// bug 'SlateViewer': Image not loaded without extension
	// Note Slate will append the extension automatically so remove the extension
	//const FName BrushName( *FPaths::GetBaseFilename( Filename, false ) );

	const FName BrushName( *Filename );

	DynamicBrush = MakeShareable( new FSlateDynamicImageBrush( BrushName, LayerVm->GetSize() ) );
}

FReply SMapLayoutView::OnDeSelectClicked()
{
	SectorProvider.Pin()->DeSelect();

	return FReply::Handled();
}

FText SMapLayoutView::GetSelectionInfo() const
{
	auto SelectionMap = SectorProvider.Pin()->GetSelectedSectorsMap();

	SMapLayoutViewLocal::LockedCount = 0;
	int32 LockedByMe = 0;
	int32 LockedByOther = 0;
	SMapLayoutViewLocal::NotLocked = 0;
	for ( auto& Pair : SelectionMap )
		for ( auto& SectorVm : Pair.Value )
		{
			const EMapLockState& LockState = SectorVm->GetLockState();
			if ( LockState == EMapLockState::NotLocked )
				SMapLayoutViewLocal::NotLocked++;
			else
			{
				SMapLayoutViewLocal::LockedCount++;
				if ( LockState == EMapLockState::LockedByMe )
					LockedByMe++;
				else
					LockedByOther++;
			}
			break;
		}

	FString Info = FString::Printf( TEXT( "%d selected (" ), SelectionMap.Num() );
	if ( SMapLayoutViewLocal::NotLocked != 0 )
		Info += FString::Printf( TEXT( "%d not locked" ), SMapLayoutViewLocal::NotLocked );
	if ( ( SMapLayoutViewLocal::NotLocked != 0 ) && ( SMapLayoutViewLocal::LockedCount != 0 ) )
		Info += TEXT( " + " );
	if ( SMapLayoutViewLocal::LockedCount != 0 )
	{
		Info += FString::Printf( TEXT( "%d locked (" ), SMapLayoutViewLocal::LockedCount );
		if ( LockedByMe != 0 )
			Info += FString::Printf( TEXT( "%d by me" ), LockedByMe );
		if ( ( LockedByMe != 0 ) && ( LockedByOther != 0 ) )
			Info += TEXT( "/" );
		if ( LockedByOther != 0 )
			Info += FString::Printf( TEXT( "%d by other" ), LockedByOther );
		Info += TEXT( ")" );
	}
	Info += TEXT( ")" );

	return FText::FromString( Info );
}

EVisibility SMapLayoutView::GetSelectedOperationsVisibility() const
{
	return SectorProvider.Pin()->GetSelectedSectorsMap().Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
}

FReply SMapLayoutView::OnClickedLockSelected()
{
	TMap< FString, TArray< TSharedPtr< FSectorViewModel > > >& SelectedSectorsMap = SectorProvider.Pin()->GetSelectedSectorsMap();
	TArray< FString > ListForLock;
	for ( auto& Pair : SelectedSectorsMap )
		for ( TSharedPtr< FSectorViewModel >& SectorVm : Pair.Value )
			if ( SectorVm->GetLockState() == EMapLockState::NotLocked )
			{
				ListForLock.Add( Pair.Key );
				break;
			}

	if ( FModuleManager::Get().IsModuleLoaded( FName( "VSPGitModule" ) ) )
	{
		IVSPGitModule& GitModule = FModuleManager::Get().LoadModuleChecked< IVSPGitModule >( "VSPGitModule" );
		GitModule.LockMaps( ListForLock );
	}

	return FReply::Handled();
}

FReply SMapLayoutView::OnClickedUnlockSelected()
{
	TMap< FString, TArray< TSharedPtr< FSectorViewModel > > >& SelectedSectorsMap = SectorProvider.Pin()->GetSelectedSectorsMap();
	TArray< FString > ListForUnlock;
	for ( auto& Pair : SelectedSectorsMap )
		for ( TSharedPtr< FSectorViewModel >& SectorVm : Pair.Value )
			if ( ( SectorVm->GetLockState() == EMapLockState::LockedByMe ) || ( SectorVm->GetLockState() == EMapLockState::LockedOther ) )
			{
				ListForUnlock.Add( Pair.Key );
				break;
			}

	if ( FModuleManager::Get().IsModuleLoaded( FName( "VSPGitModule" ) ) )
	{
		IVSPGitModule& GitModule = FModuleManager::Get().LoadModuleChecked< IVSPGitModule >( "VSPGitModule" );
		GitModule.UnlockMaps( ListForUnlock );
	}

	return FReply::Handled();
}

TSharedRef< SWidget > SMapLayoutView::HandleComboBoxGenerateWidget( TSharedPtr< IListItem > InItem )
{
	return SNew( STextBlock )
		.Text( InItem->GetDisplayName() );
}

FText SMapLayoutView::HandleMapsComboBoxText() const
{
	const TSharedPtr< IListItem > SelectedMap = SectorProvider.Pin()->GetSelectedMap();
	if ( !SelectedMap.IsValid() )
		return FText::GetEmpty();

	return MapsComboBox.IsValid()
		? SelectedMap->GetDisplayName()
		: FText::GetEmpty();
}

FText SMapLayoutView::HandleLayersComboBoxText() const
{
	const TSharedPtr< IListItem > SelectedLayer = SectorProvider.Pin()->GetSelectedLayer();
	if ( !SelectedLayer.IsValid() )
		return FText::GetEmpty();

	return LayersComboBox.IsValid()
		? SelectedLayer->GetDisplayName()
		: FText::GetEmpty();
}

float SMapLayoutView::GetZoomLevel() const
{
	return ScrollPanel->GetDesiredSize().X / DynamicBrush.Get()->ImageSize.X;
}

const FSlateBrush* SMapLayoutView::GetMapHolderBrush() const
{
	if ( DynamicBrush.IsValid() )
		return DynamicBrush.Get();

	return FCoreStyle::Get().GetBrush( "Checkerboard" );
}

#undef LOCTEXT_NAMESPACE
