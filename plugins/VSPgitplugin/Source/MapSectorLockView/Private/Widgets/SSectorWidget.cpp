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
#include "SSectorWidget.h"

#include "IVSPGitModule.h"
#include "MapSectorLockViewStyle.h"
#include "Widgets/Layout/SDPIScaler.h"

SSectorWidget::SSectorWidget()
{
	FSlateApplication::Get().OnWindowDPIScaleChanged().AddRaw( this, &SSectorWidget::HandleWindowDPIScaleChanged );
	auto TopWindow = FSlateApplicationBase::Get().GetActiveTopLevelWindow();
	if (TopWindow.IsValid())
		DPIScale = TopWindow->GetDPIScaleFactor();
	else
		DPIScale = 1.f;
}

SSectorWidget::~SSectorWidget()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().OnWindowDPIScaleChanged().RemoveAll( this );
	}
}

void SSectorWidget::Construct( const FArguments& InArgs )
{
	SectorViewModel = InArgs._SectorViewModel;
	ZoomCoefficient = InArgs._ZoomCoefficient;
	OffsetVM = InArgs._OffsetVM;
	ScaleVM = InArgs._ScaleVM;
	ImageSize = InArgs._ImageSize;
	PhysicalOffset = InArgs._PhysicalOffset;

	const TArray< FSectorPoint >& Points = SectorViewModel->GetPoints();
	for ( const FSectorPoint& Point : Points )
		ViewportPoints.Add( PointToViewport( Point.Point ) );

	ChildSlot
	[
		SAssignNew( SectorViewLayer, SOverlay )
		+ SOverlay::Slot()
		[
			SNew( SGeometryPaintWidget )
			.ViewportPoints( ViewportPoints )
			.OnPaintHandler( this, &SSectorWidget::DrawSector )
		]
		+ SOverlay::Slot()
		.Padding( TAttribute< FMargin >( this, &SSectorWidget::GetPadding ) )
		[
			SNew( SOverlay )
			+ SOverlay::Slot()
			.VAlign( VAlign_Top )
			.HAlign( HAlign_Center )
			.Padding( 5.f )
			[
				SNew( STextBlock )
				.Text( FText::FromName( SectorViewModel->GetName() ) )
				.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 10 ) )
				.ColorAndOpacity( FLinearColor::White )
			]
			+ SOverlay::Slot()
			.VAlign( VAlign_Center )
			.HAlign( HAlign_Center )
			[
				SNew( SImage )
				.Image( this, &SSectorWidget::GetLockIcon)
				.Visibility( this, &SSectorWidget::GetLockVisibility )
			]
		]
	];
}


void SSectorWidget::HandleWindowDPIScaleChanged( TSharedRef<SWindow> InWindow )
{
	DPIScale = InWindow->GetDPIScaleFactor();
}

float SSectorWidget::GetDPIScale() const
{
	return DPIScale;
}


int32 SSectorWidget::DrawSector( const FOnPaintHandlerParams& InParams, const TArray< FVector2D >& InPoints )
{
	if ( InPoints.Num() < 2 )
		return InParams.Layer;

	const FSlateBrush* MyBrush = FCoreStyle::Get().GetBrush( "ColorWheel.HueValueCircle" );

	FSlateResourceHandle Handle = MyBrush->GetRenderingResource();
	const FSlateShaderResourceProxy* ResourceProxy = Handle.GetResourceProxy();

	FVector2D UVCenter = FVector2D::ZeroVector;
	FVector2D UVRadius = FVector2D( 1, 1 );
	if ( ResourceProxy != nullptr )
	{
		UVRadius = 0.5f * ResourceProxy->SizeUV;
		UVCenter = ResourceProxy->StartUV + UVRadius;
	}

	// Draw sector poligons
	const FVector2D TopLeft = InParams.Geometry.AbsolutePosition;
	TArray< FSlateVertex > Verts;
	for ( const FVector2D& Point : InPoints )
	{
		FColor Color = bIsHovered ? SectorViewModel->GetHoveredColor() : SectorViewModel->GetDefaultColor();
		if ( SectorViewModel->IsSelected() )
		{
			int32 SelectedAlpha = static_cast<int32>(SectorViewModel->GetHoveredColor().A) + FMath::Abs(static_cast<int32>(SectorViewModel->GetHoveredColor().A) - static_cast<int32>(SectorViewModel->GetDefaultColor().A)) * 0.25;
			if ( SelectedAlpha > 255 )
				SelectedAlpha = 255;
			if ( SelectedAlpha < 0 )
				SelectedAlpha = 0;
			Color = Color.WithAlpha( static_cast<uint8>( SelectedAlpha ) );
		}
		FSlateVertex NewVert;
		NewVert.Position.X = TopLeft.X + Point.X * ZoomCoefficient.Get() * DPIScale;
		NewVert.Position.Y = TopLeft.Y + Point.Y * ZoomCoefficient.Get() * DPIScale;
		NewVert.TexCoords[ 0 ] = UVCenter.X;
		NewVert.TexCoords[ 1 ] = UVCenter.Y;
		NewVert.TexCoords[ 2 ] = NewVert.TexCoords[ 3 ] = 1.0f;
		NewVert.Color = Color;
		Verts.Add( NewVert );
	}

	TArray< SlateIndex > Indexes;
	for ( int i = 1; i < InPoints.Num() - 1; ++i )
	{
		Indexes.Add( 0 );
		Indexes.Add( i );
		Indexes.Add( i + 1 );
	}

	FSlateDrawElement::MakeCustomVerts( InParams.OutDrawElements, InParams.Layer, Handle, Verts, Indexes, nullptr, 0, 0 );

	if (SectorViewModel->GetLockState() != EMapLockState::NotLocked)
	{
		const FSlateBrush* LockBrush = FMapSectorLockViewStyle::Get().GetBrush( "MSLV.LockedGrid.Small" );

		FSlateResourceHandle LockHandle = LockBrush->GetRenderingResource();
		const FSlateShaderResourceProxy* LockResourceProxy = Handle.GetResourceProxy();

		TArray< FSlateVertex > LockVerts;
		for ( const FVector2D& Point : InPoints )
		{
			FColor Color = SectorViewModel->GetDefaultColor();
			if (SectorViewModel->GetLockState() == EMapLockState::LockedByMe)
				Color = Color.WithAlpha( 150 );
			else
				Color = FColor::Black.WithAlpha( 150 );

			FSlateVertex NewVert;
			NewVert.Position.X = TopLeft.X + Point.X * ZoomCoefficient.Get() * DPIScale;
			NewVert.Position.Y = TopLeft.Y + Point.Y * ZoomCoefficient.Get() * DPIScale;
			NewVert.TexCoords[ 0 ] = (LockResourceProxy->StartUV.X + Point.X * ZoomCoefficient.Get()) / ( ImageSize.X * ZoomCoefficient.Get() );
			NewVert.TexCoords[ 1 ] = (LockResourceProxy->StartUV.Y + Point.Y * ZoomCoefficient.Get()) / ( ImageSize.Y * ZoomCoefficient.Get() );
			NewVert.TexCoords[ 2 ] = NewVert.TexCoords[ 3 ] = 1.0f;
			NewVert.Color = Color;
			LockVerts.Add( NewVert );
		}

		FSlateDrawElement::MakeCustomVerts( InParams.OutDrawElements, InParams.Layer, LockHandle, LockVerts, Indexes, nullptr, 0, 0 );
	}

	// Draw sector outline
	TArray<FVector2D> LinePoints;
	for ( const FVector2D& Point : InPoints )
		LinePoints.Add( Point * ZoomCoefficient.Get() );
	if ( InPoints.Num() > 0)
		LinePoints.Add( InPoints[ 0 ] * ZoomCoefficient.Get() );

	FSlateDrawElement::MakeLines(
			InParams.OutDrawElements,
			InParams.Layer,
			InParams.Geometry.ToPaintGeometry(),
			LinePoints,
			InParams.bEnabled ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect,
			FColor::White,
			true,
			InParams.Geometry.Scale * (SectorViewModel->IsSelected() ? 5 : 1)
		);

	return InParams.Layer;
}

bool SSectorWidget::HoveredTest( const FVector2D& LocalPos )
{
	const auto Offset = PhysicalOffset.Get();
	const auto Left = MinX( ViewportPoints ) + Offset.X;
	const auto Right = MaxX( ViewportPoints ) + Offset.X;
	const auto Top = MinY( ViewportPoints ) + Offset.Y;
	const auto Bottom = MaxY( ViewportPoints ) + Offset.Y;
	if ( ( LocalPos.X >= Left ) &&
		( LocalPos.X <= Right ) &&
		( LocalPos.Y >= Top ) &&
		( LocalPos.Y <= Bottom ) )
		bIsHovered = true;
	else
		bIsHovered = false;
	return bIsHovered;
}

void SSectorWidget::MouseLeave()
{
	bIsHovered = false;
}

bool SSectorWidget::IsHovered() const
{
	return bIsHovered;
}

TSharedPtr< FSectorViewModel >& SSectorWidget::GetSectorViewModel()
{
	return SectorViewModel;
}

void SSectorWidget::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	IVSPGitModule& GitModule = FModuleManager::LoadModuleChecked< IVSPGitModule >( "VSPGitModule" );
	FString UserName;
	int32 LockId;
	if ( GitModule.IsFileLocked( SectorViewModel->GetLinkedMapContentPath(), UserName, LockId ) )
	{
		const FString CurrentUserName = GitModule.GetGitUserName();
		const FString CurrentEmail = GitModule.GetGitUserEmail();

		if ( UserName.Contains( CurrentUserName ) || UserName.Contains( CurrentEmail ) )
			SectorViewModel->SetLockState( EMapLockState::LockedByMe );
		else
			SectorViewModel->SetLockState( EMapLockState::LockedOther );

		SectorViewModel->SetLockUserName( UserName );
	}
	else
	{
		SectorViewModel->SetLockState( EMapLockState::NotLocked );
		SectorViewModel->SetLockUserName( FString() );
	}
}

float SSectorWidget::MinX( const TArray< FVector2D >& InPoint ) const
{
	float Min = InPoint.Num() > 0 ? InPoint[ 0 ].X : 0;
	for ( const FVector2D& Point : InPoint )
		if ( Min > Point.X )
			Min = Point.X;
	return Min;
}

float SSectorWidget::MinY( const TArray< FVector2D >& InPoint ) const
{
	float Min = InPoint.Num() > 0 ? InPoint[ 0 ].Y : 0;
	for ( const FVector2D& Point : InPoint )
		if ( Min > Point.Y )
			Min = Point.Y;
	return Min;
}

float SSectorWidget::MaxX( const TArray< FVector2D >& InPoint ) const
{
	float Max = InPoint.Num() > 0 ? InPoint[ 0 ].X : 0;
	for ( const FVector2D& Point : InPoint )
		if ( Max < Point.X )
			Max = Point.X;
	return Max;
}

float SSectorWidget::MaxY( const TArray< FVector2D >& InPoint ) const
{
	float Max = InPoint.Num() > 0 ? InPoint[ 0 ].Y : 0;
	for ( const FVector2D& Point : InPoint )
		if ( Max < Point.Y )
			Max = Point.Y;
	return Max;
}

FVector2D SSectorWidget::PointToViewport( const FVector2D& Point )
{
	return ( Point - OffsetVM ) * ScaleVM;
}

FMargin SSectorWidget::GetPadding() const
{
	const FMargin Margin = FMargin(
		MinX( ViewportPoints ) * ZoomCoefficient.Get(),
		MinY( ViewportPoints ) * ZoomCoefficient.Get(),
		( ImageSize.X - MaxX( ViewportPoints ) ) * ZoomCoefficient.Get(),
		( ImageSize.Y - MaxY( ViewportPoints ) ) * ZoomCoefficient.Get()
	);
	return Margin;
}

EVisibility SSectorWidget::GetLockVisibility() const
{
	if ( SectorViewModel->GetLockState() == EMapLockState::NotLocked )
		return EVisibility::Collapsed;
	return EVisibility::Visible;
}

const FSlateBrush* SSectorWidget::GetLockIcon() const
{
	if (SectorViewModel->GetLockState() == EMapLockState::LockedOther)
		return FMapSectorLockViewStyle::Get().GetBrush( "MSLV.UserLock" );
	return FMapSectorLockViewStyle::Get().GetBrush( "MSLV.MyLock" );
}
