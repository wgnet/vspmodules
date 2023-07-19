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

#include "Models.h"
#include "Widgets/SListWidget.h"

class FMapViewModel : public IListItem
{
public:
	FMapViewModel( FMap& InModel );

	virtual FText GetDisplayName() override;

	FMap& GetModel();

	FName GetName();
	void SetName( FName InName );

	FString GetContentPath();
	void SetContentPath( FString InContentPath );

private:
	FMap Model;
};

class FLayerViewModel : public IListItem
{
public:
	FLayerViewModel( FLayer& InModel );

	FLayer& GetModel();

	const FGuid& GetGuid();
	void SetGuid( const FGuid& InGuid );

	virtual FText GetDisplayName() override;

	FName GetName();
	void SetName( FName InName );

	const FString& GetImageBrushName();
	void SetImageBrushName( const FString& InImageBrushName );

	const FVector2D& GetSize();
	void SetSize( const FVector2D& InSize );

	const FVector2D& GetTopLeftCornerPoint();
	void SetTopLeftCornerPoint( const FVector2D& InPoint );

	const FVector2D& GetBottomRightCornerPoint();
	void SetBottomRightCornerPoint( const FVector2D& InPoint );

	const FString& GetParentMapContentPath();
	void SetParentMapContentPath( const FString& InMapContentPath );

private:
	FLayer Model;
};

class FSectorViewModel : public IListItem
{
public:
	FSectorViewModel( FSector& InSector );

	FSector& GetModel();

	virtual FText GetDisplayName() override;

	const FGuid& GetSectorGuid();
	void SetSectorGuid( const FGuid& InGuid );

	FName GetName();
	void SetName( FName InName );

	const FGuid& GetLayerGuid();
	void SetLayerGuid( const FGuid& InLayerGuid );

	const FString& GetLinkedMapContentPath();
	void SetLinkedMapContentPath( const FString& InLinkedMapContentPath );

	const TArray< FSectorPoint >& GetPoints();
	void SetPoints( const TArray< FSectorPoint > InPoints );

	const FColor& GetDefaultColor() const;
	const FColor& GetHoveredColor() const;

	EMapLockState GetLockState() const;
	void SetLockState( EMapLockState InLockState );

	FString GetLockUserName();
	void SetLockUserName( FString InLockUserName );

	void SetSelected( bool bInIsSelected );
	bool IsSelected() const;
private:
	FSector Model;
	TArray< FSectorPoint > Points;
	bool bIsSelected = false;
};

class FSectorProvider
{
public:
	void Initialize();

	// Maps methods for binding
	TArray< TSharedPtr< IListItem > >& GetMapsItems();
	TSharedPtr< IListItem > GetSelectedMap();
	void OnMapSelectionChanged( TSharedPtr< IListItem > InItem, ESelectInfo::Type SelectInfo );

	TArray< TSharedPtr< IListItem > >& GetLayersItems();
	TSharedPtr< IListItem > GetSelectedLayer();
	void OnLayerSelectionChanged( TSharedPtr< IListItem > InItem, ESelectInfo::Type SelectInfo );

	// Sectors method for binding
	TArray< TSharedPtr< FSectorViewModel > >& GetSectors();
	TMap< FString, TArray< TSharedPtr< FSectorViewModel > > >& GetSelectedSectorsMap();
	void SetSelectedSector( const TSharedPtr< FSectorViewModel >& InSectorVm );
	void AddSelectedSector( const TSharedPtr< FSectorViewModel >& InSectorVm );
	void DeSelect();
	void RemoveSelection( const TSharedPtr< FSectorViewModel >& InSectorVm );
	bool IsSelectedSector( const TSharedPtr< FSectorViewModel >& InSectorVm );

private:
	void LoadConfig();
	void LoadDefaultData();
public: // TODO: Make it private
	void SaveConfig();

private:
	// Models tables
	TMap< FString, FMap > Maps;
	TMap< int32, FMapChildren > MapChildren;
	TMap< FGuid, FLayer > Layers;
	TMap< FGuid, FSector > Sectors;
	TMap< FGuid, FSectorPoint > SectorPoints;

	// Bindable properties
	TArray< TSharedPtr< IListItem > > MapsItems;
	TSharedPtr< IListItem > SelectedMap = nullptr;

	TArray< TSharedPtr< IListItem > > LayersItems;
	TSharedPtr< IListItem > SelectedLayer = nullptr;

	TArray< TSharedPtr< FSectorViewModel > > SectorsVmList;
	TMap< FString, TArray< TSharedPtr< FSectorViewModel > > > SelectedSectorsVm;
};
