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
#include "Framework/Layout/ScrollyZoomy.h"

class SZoomablePanel : public IScrollableZoomable, public SPanel
{
public:
	class FZoomablePanelSlot : public TSupportsOneChildMixin< FZoomablePanelSlot >
	{
	public:
		FZoomablePanelSlot( SWidget* InOwner )
			: TSupportsOneChildMixin< FZoomablePanelSlot >( InOwner )
		{
		}
	};

	SLATE_BEGIN_ARGS( SZoomablePanel )
	{
		_Visibility = EVisibility::Visible;
	}
		SLATE_DEFAULT_SLOT( FArguments, Content )
	SLATE_END_ARGS()

	SZoomablePanel()
		: PhysicalOffset( ForceInitToZero )
		, CachedSize( ForceInitToZero )
		, ZoomLevel( 1.0f )
		, bFitToWindow( true )
		, ChildSlot( this )
		, ScrollyZoomy( true )
	{}

	void Construct( const FArguments& InArgs );

	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;
	virtual FVector2D ComputeDesiredSize( float ) const override;
	virtual FChildren* GetChildren() override;
	FVector2D GetPhysicalOffset() const;

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;
	virtual FReply OnMouseButtonDown( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseButtonUp( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseMove( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual void OnMouseLeave( const FPointerEvent& MouseEvent ) override;
	virtual FReply OnMouseWheel( const FGeometry& MyGeometry, const FPointerEvent& MouseEvent ) override;
	virtual FCursorReply OnCursorQuery( const FGeometry& MyGeometry, const FPointerEvent& CursorEvent ) const override;
	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled ) const override;

	virtual bool ScrollBy( const FVector2D& Offset ) override;
	virtual bool ZoomBy( const float Amount ) override;

	float GetZoomLevel() const;
	void FitToWindow();
	bool IsFitToWindow() const;
	void FitToSize();

private:
	void UpdateFitToWindowZoom( const FVector2D& ViewportSize, const FVector2D& LocalSize );
	void ClampViewOffset( const FVector2D& ViewportSize, const FVector2D& LocalSize );
	float ClampViewOffsetAxis( const float ViewportSize, const float LocalSize, const float CurrentOffset );

private:
	FVector2D PhysicalOffset;
	mutable FVector2D CachedSize;
	float ZoomLevel;
	bool bFitToWindow;

	FZoomablePanelSlot ChildSlot;
	FScrollyZoomy ScrollyZoomy;
};
