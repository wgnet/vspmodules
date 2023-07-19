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
#include "SZoomablePanel.h"

void SZoomablePanel::Construct( const FArguments& InArgs )
{
	ChildSlot
	[
		InArgs._Content.Widget
	];
}

void SZoomablePanel::OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const
{
	CachedSize = AllottedGeometry.GetLocalSize();

	const TSharedRef< SWidget >& ChildWidget = ChildSlot.GetWidget();
	if ( ChildWidget->GetVisibility() != EVisibility::Collapsed )
	{
		const FVector2D& WidgetDesiredSize = ChildWidget->GetDesiredSize();

		// Update the zoom level, and clamp the pan offset based on our current geometry
		SZoomablePanel* const NonConstThis = const_cast< SZoomablePanel* >(this);
		NonConstThis->UpdateFitToWindowZoom( WidgetDesiredSize, CachedSize );
		NonConstThis->ClampViewOffset( WidgetDesiredSize, CachedSize );

		ArrangedChildren.AddWidget( AllottedGeometry.MakeChild( ChildWidget, PhysicalOffset * ZoomLevel, WidgetDesiredSize * ZoomLevel ) );
	}
}

FVector2D SZoomablePanel::ComputeDesiredSize( float ) const
{
	FVector2D ThisDesiredSize = FVector2D::ZeroVector;

	const TSharedRef< SWidget >& ChildWidget = ChildSlot.GetWidget();
	if ( ChildWidget->GetVisibility() != EVisibility::Collapsed )
		ThisDesiredSize = ChildWidget->GetDesiredSize() * ZoomLevel;

	return ThisDesiredSize;
}

FChildren* SZoomablePanel::GetChildren()
{
	return &ChildSlot;
}

FVector2D SZoomablePanel::GetPhysicalOffset() const
{
	return PhysicalOffset;
}

void SZoomablePanel::Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime )
{
	ScrollyZoomy.Tick( InDeltaTime, *this );
}

FReply SZoomablePanel::OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return ScrollyZoomy.OnMouseButtonDown( MouseEvent );
}

FReply SZoomablePanel::OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return ScrollyZoomy.OnMouseButtonUp( AsShared(), MyGeometry, MouseEvent );
}

FReply SZoomablePanel::OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return ScrollyZoomy.OnMouseMove( AsShared(), *this, MyGeometry, MouseEvent );
}

void SZoomablePanel::OnMouseLeave( const FPointerEvent& MouseEvent )
{
	ScrollyZoomy.OnMouseLeave( AsShared(), MouseEvent );
}

FReply SZoomablePanel::OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent )
{
	return ScrollyZoomy.OnMouseWheel( MouseEvent, *this );
}

FCursorReply SZoomablePanel::OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const
{
	return ScrollyZoomy.OnCursorQuery();
}

int32 SZoomablePanel::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled ) const
{
	LayerId = SPanel::OnPaint( Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled );
	LayerId = ScrollyZoomy.PaintSoftwareCursorIfNeeded( AllottedGeometry, MyCullingRect, OutDrawElements, LayerId );
	return LayerId;
}

bool SZoomablePanel::ScrollBy( const FVector2D& Offset )
{
	if ( bFitToWindow )
		return false;

	const FVector2D PrevPhysicalOffset = PhysicalOffset;
	const float InverseZoom = 1.0f / ZoomLevel;
	PhysicalOffset += ( Offset * InverseZoom );

	const TSharedRef< SWidget >& ChildWidget = ChildSlot.GetWidget();
	const FVector2D& WidgetDesiredSize = ChildWidget->GetDesiredSize();
	ClampViewOffset( WidgetDesiredSize, CachedSize );

	return PhysicalOffset != PrevPhysicalOffset;
}

bool SZoomablePanel::ZoomBy( const float Amount )
{
	static const float MinZoomLevel = 0.2f;
	// TODO: This is a very annoying bug. You need to figure out how to fix it, or get around it.
	//
	// SSectorWidget -> FMargin GetPadding() const - counts indentation for sector labels
	//
	// These indents are approximately correct, but when ZoomLevel becomes greater than 1,
	// the image magnification occurs non-stop and indefinitely,
	// probably due to the cropped mantis of the floating-point number in the margin.
	//
	//static const float MaxZoomLevel = 4.0f;
	static const float MaxZoomLevel = 1.0f;

	bFitToWindow = false;

	const float PrevZoomLevel = ZoomLevel;
	ZoomLevel = FMath::Clamp( ZoomLevel + ( Amount * 0.05f ), MinZoomLevel, MaxZoomLevel );
	return ZoomLevel != PrevZoomLevel;
}

float SZoomablePanel::GetZoomLevel() const
{
	return ZoomLevel;
}

void SZoomablePanel::FitToWindow()
{
	bFitToWindow = true;
	PhysicalOffset = FVector2D::ZeroVector;
}

bool SZoomablePanel::IsFitToWindow() const
{
	return bFitToWindow;
}

void SZoomablePanel::FitToSize()
{
	bFitToWindow = false;
	ZoomLevel = 1.0f;
	PhysicalOffset = FVector2D::ZeroVector;
}

void SZoomablePanel::UpdateFitToWindowZoom( const FVector2D& ViewportSize, const FVector2D& LocalSize )
{
	if ( bFitToWindow )
	{
		const float ZoomHoriz = LocalSize.X / ViewportSize.X;
		const float ZoomVert = LocalSize.Y / ViewportSize.Y;
		ZoomLevel = FMath::Min( ZoomHoriz, ZoomVert );
	}
}

void SZoomablePanel::ClampViewOffset( const FVector2D& ViewportSize, const FVector2D& LocalSize )
{
	PhysicalOffset.X = ClampViewOffsetAxis( ViewportSize.X, LocalSize.X, PhysicalOffset.X );
	PhysicalOffset.Y = ClampViewOffsetAxis( ViewportSize.Y, LocalSize.Y, PhysicalOffset.Y );
}

float SZoomablePanel::ClampViewOffsetAxis( const float ViewportSize, const float LocalSize, const float CurrentOffset )
{
	const float ZoomedViewportSize = ViewportSize * ZoomLevel;

	if ( ZoomedViewportSize <= LocalSize )
		// If the viewport is smaller than the available size, then we can't be scrolled
		return 0.0f;

	// Given the size of the viewport, and the current size of the window, work how far we can scroll
	// Note: This number is negative since scrolling down/right moves the viewport up/left
	const float MaxScrollOffset = ( LocalSize - ZoomedViewportSize ) / ZoomLevel;

	// Clamp the left/top edge
	if ( CurrentOffset < MaxScrollOffset )
		return MaxScrollOffset;

	// Clamp the right/bottom edge
	if ( CurrentOffset > 0.0f )
		return 0.0f;

	return CurrentOffset;
}
