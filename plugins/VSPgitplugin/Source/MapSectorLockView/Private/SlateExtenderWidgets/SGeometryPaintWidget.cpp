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
#include "SGeometryPaintWidget.h"

void SGeometryPaintWidget::Construct( const FArguments& InArgs )
{
	OnPaintHandler = InArgs._OnPaintHandler;
	ViewportPoints = InArgs._ViewportPoints;
}

FVector2D SGeometryPaintWidget::ComputeDesiredSize( float ) const
{
	return FVector2D( 1, 1 );
}

int32 SGeometryPaintWidget::OnPaint(
	const FPaintArgs& Args,
	const FGeometry& AllottedGeometry,
	const FSlateRect& MyCullingRect,
	FSlateWindowElementList& OutDrawElements,
	int32 LayerId,
	const FWidgetStyle& InWidgetStyle,
	bool bParentEnabled ) const
{
	if ( OnPaintHandler.IsBound() )
	{
		FOnPaintHandlerParams Params( AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, bParentEnabled && IsEnabled() );
		OnPaintHandler.Execute( Params, ViewportPoints );
	}
	else
		FSlateDrawElement::MakeDebugQuad(
			OutDrawElements,
			LayerId,
			AllottedGeometry.ToPaintGeometry()
		);

	return SCompoundWidget::OnPaint( Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled && IsEnabled() );
}
