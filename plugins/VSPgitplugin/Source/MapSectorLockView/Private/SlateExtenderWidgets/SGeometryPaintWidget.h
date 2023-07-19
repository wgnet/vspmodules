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

struct FOnPaintHandlerParams
{
	const FGeometry& Geometry;
	const FSlateRect& ClippingRect;
	FSlateWindowElementList& OutDrawElements;
	const int32 Layer;
	const bool bEnabled;

	FOnPaintHandlerParams(
		const FGeometry& InGeometry,
		const FSlateRect& InClippingRect,
		FSlateWindowElementList& InOutDrawElements,
		int32 InLayer,
		bool bInEnabled )
		: Geometry( InGeometry )
		, ClippingRect( InClippingRect )
		, OutDrawElements( InOutDrawElements )
		, Layer( InLayer )
		, bEnabled( bInEnabled )
	{}
};

/** Delegate type for allowing custom OnPaint handlers */
DECLARE_DELEGATE_RetVal_TwoParams(
	int32,
	FOnPaintHandler,
	const FOnPaintHandlerParams&,
	const TArray<FVector2D>& );

/** Widget with a handler for OnPaint; convenient for testing various DrawPrimitives. */
class SGeometryPaintWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS( SGeometryPaintWidget )
		: _OnPaintHandler()
	{}
		SLATE_EVENT( FOnPaintHandler, OnPaintHandler )
		SLATE_ARGUMENT( TArray<FVector2D>, ViewportPoints )
	SLATE_END_ARGS()

	/**
	* Construct this widget
	*
	* @param	InArgs	The declaration data for this widget
	*/
	void Construct( const FArguments& InArgs );

	virtual FVector2D ComputeDesiredSize( float ) const override;

	virtual int32 OnPaint(
		const FPaintArgs& Args,
		const FGeometry& AllottedGeometry,
		const FSlateRect& MyCullingRect,
		FSlateWindowElementList& OutDrawElements,
		int32 LayerId,
		const FWidgetStyle& InWidgetStyle,
		bool bParentEnabled ) const override;

private:
	FOnPaintHandler OnPaintHandler;
	TArray< FVector2D > ViewportPoints;
};
