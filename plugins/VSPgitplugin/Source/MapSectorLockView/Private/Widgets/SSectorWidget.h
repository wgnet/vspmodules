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

#include "Core/SectorProvider.h"
#include "SlateExtenderWidgets/SGeometryPaintWidget.h"

class SSectorWidget : public SCompoundWidget
{
public:
	SSectorWidget();
	~SSectorWidget();

	SLATE_BEGIN_ARGS( SSectorWidget )
	{}
		SLATE_ARGUMENT( TSharedPtr<FSectorViewModel>, SectorViewModel )
		SLATE_ATTRIBUTE( float, ZoomCoefficient )
		SLATE_ATTRIBUTE( FVector2D, PhysicalOffset )
		SLATE_ARGUMENT( FVector2D, OffsetVM )
		SLATE_ARGUMENT( FVector2D, ScaleVM )
		SLATE_ARGUMENT( FVector2D, ImageSize )
	SLATE_END_ARGS()

	FVector2D PointToViewport( const FVector2D& Point );
	/**
	* Construct this widget
	* @param	InArgs	The declaration data for this widget
	*/
	void Construct( const FArguments& InArgs );

	int32 DrawSector( const FOnPaintHandlerParams& InParams, const TArray< FVector2D >& InPoints );
	bool HoveredTest( const FVector2D& LocalPos );
	void MouseLeave();

	virtual bool IsHovered() const override;

	TSharedPtr< FSectorViewModel >& GetSectorViewModel();

	virtual void Tick( const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime ) override;

private:
	void HandleWindowDPIScaleChanged(TSharedRef<SWindow> InWindow);

	float MinX( const TArray< FVector2D >& InPoint ) const;
	float MinY( const TArray< FVector2D >& InPoint ) const;
	float MaxX( const TArray< FVector2D >& InPoint ) const;
	float MaxY( const TArray< FVector2D >& InPoint ) const;

	float GetDPIScale() const;

	FMargin GetPadding() const;
	EVisibility GetLockVisibility() const;
	const FSlateBrush* GetLockIcon() const;
private:
	TSharedPtr< FSectorViewModel > SectorViewModel = nullptr;
	TSharedPtr< SOverlay > SectorViewLayer = nullptr;
	TAttribute< float > ZoomCoefficient = 1.f;
	TAttribute< FVector2D > PhysicalOffset;
	FVector2D OffsetVM;
	FVector2D ScaleVM;
	FVector2D ImageSize;

	float DPIScale = 1.f;

	TArray< FVector2D > ViewportPoints;
	bool bIsHovered = false;
};
