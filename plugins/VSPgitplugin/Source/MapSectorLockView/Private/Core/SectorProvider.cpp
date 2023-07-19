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
#include "SectorProvider.h"

#include "Models.h"
#include "MapSectorLockView.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Misc/FileHelper.h"
#include "Misc/LocalTimestampDirectoryVisitor.h"
#include "Serialization/JsonSerializer.h"

namespace SectorProviderLocal
{
	struct FDirectoryVisitor : public IPlatformFile::FDirectoryVisitor
	{
		//This function is called for every file or directory it finds.
		virtual bool Visit( const TCHAR* FilenameOrDirectory, bool bIsDirectory ) override
		{
			// did we find a Directory or a file?
			if ( bIsDirectory )
				Directories.Add( FString( FilenameOrDirectory ) );
			else
				Files.Add( FString( FilenameOrDirectory ) );
			return true;
		}

		TArray< FString > Directories;
		TArray< FString > Files;
	};

	TSharedPtr< FJsonValue > LoadJsonFile( FString const& FilePath )
	{
		// load text file
		FString FileText;
		if ( !FFileHelper::LoadFileToString( FileText, *FilePath ) )
			return nullptr;

		// parse as json
		const TSharedRef< TJsonReader< > > JsonReader = TJsonReaderFactory< >::Create( FileText );
		TSharedPtr< FJsonValue > JsonFile;
		if ( !FJsonSerializer::Deserialize( JsonReader, JsonFile ) )
			return nullptr;

		return JsonFile;
	}

	bool TryGetVector2DField( const TSharedPtr< FJsonObject >& InJsonObject, FVector2D& OutVector2D )
	{
		if ( ( InJsonObject == nullptr ) || ( !InJsonObject.IsValid() ) )
			return false;

		double ValueX, ValueY;
		if ( !InJsonObject->TryGetNumberField( TEXT( "X" ), ValueX ) )
			return false;
		if ( !InJsonObject->TryGetNumberField( TEXT( "Y" ), ValueY ) )
			return false;

		OutVector2D.X = ValueX;
		OutVector2D.Y = ValueY;
		return true;
	}

	bool TryGetColorField( const TSharedPtr< FJsonObject >& InJsonObject, FColor& OutColor )
	{
		if ( ( InJsonObject == nullptr ) || ( !InJsonObject.IsValid() ) )
			return false;

		int32 R, G, B, A;
		if ( !InJsonObject->TryGetNumberField( TEXT( "R" ), R ) )
			return false;
		if ( !InJsonObject->TryGetNumberField( TEXT( "G" ), G ) )
			return false;
		if ( !InJsonObject->TryGetNumberField( TEXT( "B" ), B ) )
			return false;
		if ( !InJsonObject->TryGetNumberField( TEXT( "A" ), A ) )
			return false;

		OutColor = FColor( R, G, B, A );
		return true;
	}
}

// Map ViewModel

FMapViewModel::FMapViewModel( FMap& InModel )
{
	Model = InModel;
}

FText FMapViewModel::GetDisplayName()
{
	return FText::FromName( Model.Name );
}

FMap& FMapViewModel::GetModel()
{
	return Model;
}

FName FMapViewModel::GetName()
{
	return Model.Name;
}

void FMapViewModel::SetName( FName InName )
{
	Model.Name = InName;
}

FString FMapViewModel::GetContentPath()
{
	return Model.ContentPath;
}

void FMapViewModel::SetContentPath( FString InContentPath )
{
	Model.ContentPath = InContentPath;
}

// Layer ViewModel

FLayerViewModel::FLayerViewModel( FLayer& InModel )
{
	Model = InModel;
}

FLayer& FLayerViewModel::GetModel()
{
	return Model;
}

const FGuid& FLayerViewModel::GetGuid()
{
	return Model.LayerGuid;
}

void FLayerViewModel::SetGuid( const FGuid& InGuid )
{
	Model.LayerGuid = InGuid;
}

FText FLayerViewModel::GetDisplayName()
{
	return FText::FromName( Model.Name );
}

FName FLayerViewModel::GetName()
{
	return Model.Name;
}

void FLayerViewModel::SetName( FName InName )
{
	Model.Name = InName;
}

const FString& FLayerViewModel::GetImageBrushName()
{
	return Model.ImageBrushName;
}

void FLayerViewModel::SetImageBrushName( const FString& InImageBrushName )
{
	Model.ImageBrushName = InImageBrushName;
}

const FVector2D& FLayerViewModel::GetSize()
{
	return Model.Size;
}

void FLayerViewModel::SetSize( const FVector2D& InSize )
{
	Model.Size = InSize;
}

const FVector2D& FLayerViewModel::GetTopLeftCornerPoint()
{
	return Model.TopLeft;
}

void FLayerViewModel::SetTopLeftCornerPoint( const FVector2D& InPoint )
{
	Model.TopLeft = InPoint;
}

const FVector2D& FLayerViewModel::GetBottomRightCornerPoint()
{
	return Model.BottomRight;
}

void FLayerViewModel::SetBottomRightCornerPoint( const FVector2D& InPoint )
{
	Model.BottomRight = InPoint;
}

const FString& FLayerViewModel::GetParentMapContentPath()
{
	return Model.ParentMapContentPath;
}

void FLayerViewModel::SetParentMapContentPath( const FString& InMapContentPath )
{
	Model.ParentMapContentPath = InMapContentPath;
}

// Sector View Model

FSectorViewModel::FSectorViewModel( FSector& InSector )
{
	Model = InSector;
}

FSector& FSectorViewModel::GetModel()
{
	return Model;
}

FText FSectorViewModel::GetDisplayName()
{
	return FText::FromName( Model.Name );
}

const FGuid& FSectorViewModel::GetSectorGuid()
{
	return Model.SectorGuid;
}

void FSectorViewModel::SetSectorGuid( const FGuid& InGuid )
{
	Model.SectorGuid = InGuid;
}

FName FSectorViewModel::GetName()
{
	return Model.Name;
}

void FSectorViewModel::SetName( FName InName )
{
	Model.Name = InName;
}

const FGuid& FSectorViewModel::GetLayerGuid()
{
	return Model.LayerGuid;
}

void FSectorViewModel::SetLayerGuid( const FGuid& InLayerGuid )
{
	Model.LayerGuid = InLayerGuid;
}

const FString& FSectorViewModel::GetLinkedMapContentPath()
{
	return Model.LinkedMapContentPath;
}

void FSectorViewModel::SetLinkedMapContentPath( const FString& InLinkedMapContentPath )
{
	Model.LinkedMapContentPath = InLinkedMapContentPath;
}

const TArray< FSectorPoint >& FSectorViewModel::GetPoints()
{
	return Points;
}

void FSectorViewModel::SetPoints( const TArray< FSectorPoint > InPoints )
{
	Points.Empty();
	Points.Append( InPoints );
}

const FColor& FSectorViewModel::GetDefaultColor() const
{
	return Model.DefaultColor;
}

const FColor& FSectorViewModel::GetHoveredColor() const
{
	return Model.HoveredColor;
}

EMapLockState FSectorViewModel::GetLockState() const
{
	return Model.LockState;
}

void FSectorViewModel::SetLockState( EMapLockState InLockState )
{
	Model.LockState = InLockState;
}

FString FSectorViewModel::GetLockUserName()
{
	return Model.LockUserName;
}

void FSectorViewModel::SetLockUserName( FString InLockUserName )
{
	Model.LockUserName = InLockUserName;
}

void FSectorViewModel::SetSelected( bool bInIsSelected )
{
	bIsSelected = bInIsSelected;
}

bool FSectorViewModel::IsSelected() const
{
	return bIsSelected;
}

// FSectorProvider implementation

void FSectorProvider::Initialize()
{
	LoadConfig();
	//LoadDefaultData();

	// Data preparation
	for ( auto&& MapsPair : Maps )
	{
		TSharedPtr< FMapViewModel > Map = MakeShareable( new FMapViewModel( MapsPair.Value ) );
		MapsItems.Add( Map );
	}

	SelectedMap = MapsItems.Num() > 0 ? MapsItems[ 0 ] : nullptr;
	OnMapSelectionChanged( SelectedMap, ESelectInfo::Direct );
}

TArray< TSharedPtr< IListItem > >& FSectorProvider::GetMapsItems()
{
	return MapsItems;
}

TSharedPtr< IListItem > FSectorProvider::GetSelectedMap()
{
	return SelectedMap;
}

void FSectorProvider::OnMapSelectionChanged( TSharedPtr< IListItem > InItem, ESelectInfo::Type SelectInfo )
{
	LayersItems.Empty();
	DeSelect();
	SelectedMap = InItem;
	if ( InItem.IsValid() )
		// Get layers list for show in UI
		for ( auto&& LayerPair : Layers )
		{
			TSharedPtr< FMapViewModel > SelectedMapVm = StaticCastSharedPtr< FMapViewModel >( SelectedMap );
			if ( LayerPair.Value.ParentMapContentPath.Equals( SelectedMapVm->GetContentPath() ) )
			{
				TSharedPtr< FLayerViewModel > LayerVm = MakeShareable( new FLayerViewModel( LayerPair.Value ) );
				LayersItems.Add( LayerVm );
			}
		}

	SelectedLayer = LayersItems.Num() > 0 ? LayersItems[ 0 ] : nullptr;
	OnLayerSelectionChanged( SelectedLayer, ESelectInfo::Direct );
}

TArray< TSharedPtr< IListItem > >& FSectorProvider::GetLayersItems()
{
	return LayersItems;
}

TSharedPtr< IListItem > FSectorProvider::GetSelectedLayer()
{
	return SelectedLayer;
}

void FSectorProvider::OnLayerSelectionChanged( TSharedPtr< IListItem > InItem, ESelectInfo::Type SelectInfo )
{
	SectorsVmList.Empty();
	DeSelect();
	SelectedLayer = InItem;
	if ( SelectedLayer.IsValid() )
		for ( auto&& SectorPair : Sectors )
		{
			TSharedPtr< FLayerViewModel > SelectedLayerVm = StaticCastSharedPtr< FLayerViewModel >( SelectedLayer );
			if ( SectorPair.Value.LayerGuid == SelectedLayerVm->GetGuid() )
			{
				TSharedPtr< FSectorViewModel > SectorVm = MakeShareable( new FSectorViewModel( SectorPair.Value ) );
				TArray< FSectorPoint > Points;
				for ( auto&& SectorPointPair : SectorPoints )
					if ( SectorPointPair.Value.SectorGuid == SectorVm->GetSectorGuid() )
						Points.Add( SectorPointPair.Value );
				SectorVm->SetPoints( Points );
				SectorsVmList.Add( SectorVm );
			}
		}
}

TArray< TSharedPtr< FSectorViewModel > >& FSectorProvider::GetSectors()
{
	return SectorsVmList;
}

TMap< FString, TArray< TSharedPtr< FSectorViewModel > > >& FSectorProvider::GetSelectedSectorsMap()
{
	return SelectedSectorsVm;
}

void FSectorProvider::SetSelectedSector( const TSharedPtr< FSectorViewModel >& InSectorVm )
{
	DeSelect();
	AddSelectedSector( InSectorVm );
}

void FSectorProvider::AddSelectedSector( const TSharedPtr< FSectorViewModel >& InSectorVm )
{
	if ( !IsSelectedSector( InSectorVm ) )
	{
		TArray< TSharedPtr< FSectorViewModel > > List;
		for ( TSharedPtr< FSectorViewModel >& SectorViewModel : SectorsVmList )
			if ( SectorViewModel->GetLinkedMapContentPath().Equals( InSectorVm->GetLinkedMapContentPath() ) )
			{
				SectorViewModel->SetSelected( true );
				List.Add( SectorViewModel );
			}
		SelectedSectorsVm.Add( InSectorVm->GetLinkedMapContentPath(), List );
	}
}

void FSectorProvider::DeSelect()
{
	for ( auto& SelectedPair : SelectedSectorsVm )
		for ( TSharedPtr< FSectorViewModel, ESPMode::Fast >& SectorPtr : SelectedPair.Value )
			SectorPtr->SetSelected( false );
	SelectedSectorsVm.Empty();
}

void FSectorProvider::RemoveSelection( const TSharedPtr< FSectorViewModel >& InSectorVm )
{
	if ( IsSelectedSector( InSectorVm ) )
	{
		TArray< TSharedPtr< FSectorViewModel > >* Founded = SelectedSectorsVm.Find( InSectorVm->GetLinkedMapContentPath() );
		if ( Founded != nullptr )
			for ( TSharedPtr< FSectorViewModel, ESPMode::Fast >& SectorPtr : *Founded )
				SectorPtr->SetSelected( false );
		SelectedSectorsVm.Remove( InSectorVm->GetLinkedMapContentPath() );
	}
}

bool FSectorProvider::IsSelectedSector( const TSharedPtr< FSectorViewModel >& InSectorVm )
{
	TArray< TSharedPtr< FSectorViewModel > >* Founded = SelectedSectorsVm.Find( InSectorVm->GetLinkedMapContentPath() );
	return Founded != nullptr;
}

void FSectorProvider::LoadConfig()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// Check config folder
	const FString ConfigPath = FPaths::Combine( FPaths::ProjectDir(), TEXT( "Tools" ), TEXT( "MapSectorsConfig" ) );
	if ( !PlatformFile.DirectoryExists( *ConfigPath ) )
	UE_LOG( MapSectorLockLog, Error, TEXT("Configuration directory \"%s\" not found."), *ConfigPath );

	//Now we need to create a DirectoryVisitor
	SectorProviderLocal::FDirectoryVisitor RootConfigVisitor;
	// The IterateDirectory takes two arguments The directory and the Visitor we created above.
	PlatformFile.IterateDirectory( *ConfigPath, RootConfigVisitor );

	for ( const FString& Path : RootConfigVisitor.Directories )
	{
		// Check map config file
		const FString DirName = FPaths::GetBaseFilename( Path );
		const FString MapConfigPath = FPaths::Combine( ConfigPath, DirName );
		const FString MapJsonConfigPath = FPaths::Combine( MapConfigPath, FString::Printf( TEXT( "%s_MapConfig.json" ), *DirName ) );
		if ( !PlatformFile.FileExists( *MapJsonConfigPath ) )
		{
			UE_LOG( MapSectorLockLog, Error, TEXT("Configuration map file \"%s\" not found."), *MapJsonConfigPath );
			continue;
		}

		// Load map config file
		TSharedPtr< FJsonValue > MapJsonValue = SectorProviderLocal::LoadJsonFile( MapJsonConfigPath );
		if ( MapJsonValue == nullptr )
		{
			UE_LOG( MapSectorLockLog, Error, TEXT("Unable to read json file \"%s\""), *MapJsonConfigPath );
			continue;
		}

		// Creat Map Row in db
		const TSharedPtr< FJsonObject > MapObject = MapJsonValue->AsObject();
		FMap CurrMap;
		FString TempName;
		if ( MapObject->TryGetStringField( TEXT( "Name" ), TempName ) )
			CurrMap.Name = FName( TempName );
		MapObject->TryGetStringField( TEXT( "ContentPath" ), CurrMap.ContentPath );
		MapObject->TryGetStringField( TEXT( "DefaultImage" ), CurrMap.DefaultImage );
		SectorProviderLocal::TryGetVector2DField( MapObject->GetObjectField( TEXT( "DefaultImageSize" ) ), CurrMap.DefaultImageSize );
		Maps.Add( CurrMap.ContentPath, CurrMap );

		// Get list layers
		const TArray< TSharedPtr< FJsonValue > >* LayersJsonValues;
		if ( MapObject->TryGetArrayField( TEXT( "Layers" ), LayersJsonValues ) )
			if ( LayersJsonValues != nullptr )
				for ( const TSharedPtr< FJsonValue >& LayerNameJsonValue : ( *LayersJsonValues ) )
				{
					const FString LayerName = LayerNameJsonValue->AsString();

					// Load layer config
					const FString LayerConfigPath = FPaths::Combine( MapConfigPath, LayerName );
					const FString LayerConfigJsonPath = FPaths::Combine( LayerConfigPath, FString::Printf( TEXT( "%s_LayerConfig.json" ), *LayerName ) );
					TSharedPtr< FJsonValue > LayerJsonValue = SectorProviderLocal::LoadJsonFile( LayerConfigJsonPath );
					if ( LayerJsonValue == nullptr )
					{
						UE_LOG( MapSectorLockLog, Error, TEXT("Unable to read json file \"%s\""), *LayerConfigJsonPath );
						continue;
					}

					// Creat Layer Row in db
					const TSharedPtr< FJsonObject > LayerObject = LayerJsonValue->AsObject();
					FLayer CurrLayer{ FGuid::NewGuid() };
					TempName = LayerName;
					LayerObject->TryGetStringField( TEXT( "Name" ), TempName );
					CurrLayer.Name = FName( TempName );
					{
						FString ImagePath;
						LayerObject->TryGetStringField( TEXT( "Image" ), ImagePath );
						if ( ( PlatformFile.FileExists( *FPaths::Combine( FPaths::ProjectDir(), ImagePath ) ) ) &&
							( SectorProviderLocal::TryGetVector2DField( LayerObject->GetObjectField( TEXT( "ImageSize" ) ), CurrLayer.Size ) ) )
							CurrLayer.ImageBrushName = ImagePath;
						else
						{
							CurrLayer.ImageBrushName = CurrMap.DefaultImage;
							CurrLayer.Size = CurrMap.DefaultImageSize;
						}
					}
					SectorProviderLocal::TryGetVector2DField( LayerObject->GetObjectField( TEXT( "TopLeftReal" ) ), CurrLayer.TopLeft );
					SectorProviderLocal::TryGetVector2DField( LayerObject->GetObjectField( TEXT( "BottomRightReal" ) ), CurrLayer.BottomRight );
					LayerObject->TryGetStringField( TEXT( "ParentMapContentPath" ), CurrLayer.ParentMapContentPath );
					if ( !CurrMap.ContentPath.Equals( CurrLayer.ParentMapContentPath ) )
					{
						UE_LOG(
							MapSectorLockLog,
							Error,
							TEXT("The config for Map \"%s\" includes child Layer \"%s\", but layer \"%s\" points to the other parent \"%s\"."),
							*CurrMap.ContentPath,
							*CurrLayer.Name.ToString(),
							*CurrLayer.Name.ToString(),
							*CurrLayer.ParentMapContentPath );
						continue;
					}
					Layers.Add( CurrLayer.LayerGuid, CurrLayer );

					// Get sectors list
					const TArray< TSharedPtr< FJsonValue > >* SectorsJsonValues;
					if ( LayerObject->TryGetArrayField( TEXT( "Sectors" ), SectorsJsonValues ) )
						if ( SectorsJsonValues != nullptr )
							for ( const TSharedPtr< FJsonValue >& SectorNameJsonValue : ( *SectorsJsonValues ) )
							{
								const FString SectorName = SectorNameJsonValue->AsString();

								// Load Sectors config
								const FString SectorsConfigPath = FPaths::Combine( MapConfigPath, LayerName );
								const FString SectorConfigJsonPath = FPaths::Combine(
									LayerConfigPath,
									FString::Printf( TEXT( "%s_SectorConfig.json" ), *SectorName ) );
								TSharedPtr< FJsonValue > SectorJsonValue = SectorProviderLocal::LoadJsonFile( SectorConfigJsonPath );
								if ( SectorJsonValue == nullptr )
								{
									UE_LOG( MapSectorLockLog, Error, TEXT("Unable to read json file \"%s\""), *SectorConfigJsonPath );
									continue;
								}

								// Creat Sector Row in db
								const TSharedPtr< FJsonObject > SectorObject = SectorJsonValue->AsObject();
								FSector CurrSector{ FGuid::NewGuid() };
								TempName = LayerName;
								SectorObject->TryGetStringField( TEXT( "Name" ), TempName );
								CurrSector.Name = FName( TempName );
								SectorObject->TryGetStringField( TEXT( "LinkedMap" ), CurrSector.LinkedMapContentPath );
								SectorProviderLocal::TryGetColorField( SectorObject->GetObjectField( TEXT( "DefaultColor" ) ), CurrSector.DefaultColor );
								SectorProviderLocal::TryGetColorField( SectorObject->GetObjectField( TEXT( "HoveredColor" ) ), CurrSector.HoveredColor );

								CurrSector.LayerGuid = CurrLayer.LayerGuid;
								Sectors.Add( CurrSector.SectorGuid, CurrSector );

								// Get Points list
								const TArray< TSharedPtr< FJsonValue > >* PointsJsonValues;
								if ( SectorObject->TryGetArrayField( TEXT( "Points" ), PointsJsonValues ) )
									if ( PointsJsonValues != nullptr )
										for ( const TSharedPtr< FJsonValue >& PointJsonValue : ( *PointsJsonValues ) )
										{
											// Creat Point Row in db
											FSectorPoint CurrPoint{ FGuid::NewGuid() };
											SectorProviderLocal::TryGetVector2DField( PointJsonValue->AsObject(), CurrPoint.Point );
											CurrPoint.SectorGuid = CurrSector.SectorGuid;
											SectorPoints.Add( CurrPoint.PointGuid, CurrPoint );
										}
							}
				}
	}
}

// TODO: Remove it. It`s unusable debug function. Now this logic implemented in configs files and in "void LoadData()"
void FSectorProvider::LoadDefaultData()
{
	// Fill maps data
	FMap TerraMagna{
		FName( "TerraMagna" ),
		TEXT( "Content/TerraMagna/TerraMagna.umap" ),
		TEXT( "Config/MapSectorsConfig/TerraMagna/default.png" ),
		FVector2D( 2160, 2160 )
	};
	Maps.Add( TerraMagna.ContentPath, TerraMagna );

	// Fill layers data
	FLayer TerrainLayer{
		FGuid::NewGuid(),
		FName( "Terrain" ),
		TEXT( "Config/MapSectorsConfig/TerraMagna/default.png" ),
		FVector2D( 2160, 2160 ),
		FVector2D( -12500, 8500 ),
		FVector2D( 4500, -8500 ),
		TEXT( "Content/TerraMagna/TerraMagna.umap" )
	};
	Layers.Add( TerrainLayer.LayerGuid, TerrainLayer );
	FLayer StuffLayer{
		FGuid::NewGuid(),
		FName( "Zone_Stuff" ),
		TEXT( "Config/MapSectorsConfig/TerraMagna_minimap.png" ),
		FVector2D( 1080, 1080 ),
		FVector2D( 0, 0 ),
		FVector2D( 0, 0 ),
		TEXT( "Content/TerraMagna/TerraMagna.umap" )
	};
	Layers.Add( StuffLayer.LayerGuid, StuffLayer );

	// Fill sectors data
	TSharedPtr< FSectorPoint > CurrPoint;
	{
		FSector S0_0{
			FGuid::NewGuid(),
			FName( "0_0" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_0_0/LA_TerraMagna_0_0.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S0_0.SectorGuid, S0_0 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -12500, 8500 ), S0_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -12500, 4500 ), S0_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, 4500 ), S0_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, 8500 ), S0_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S4_0{
			FGuid::NewGuid(),
			FName( "4_0" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_4_0/LA_TerraMagna_4_0.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S4_0.SectorGuid, S4_0 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -12500, 4500 ), S4_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -12500, 0 ), S4_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, 0 ), S4_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, 4500 ), S4_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S6_0{
			FGuid::NewGuid(),
			FName( "6_0" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_6_0/LA_TerraMagna_6_0.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S6_0.SectorGuid, S6_0 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -12500, 0 ), S6_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -12500, -4500 ), S6_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, -4500 ), S6_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, 0 ), S6_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S10_0{
			FGuid::NewGuid(),
			FName( "10_0" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_10_0/LA_TerraMagna_10_0.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S10_0.SectorGuid, S10_0 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -12500, -4500 ), S10_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -12500, -8500 ), S10_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, -8500 ), S10_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, -4500 ), S10_0.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S0_4{
			FGuid::NewGuid(),
			FName( "0_4" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_0_4/LA_TerraMagna_0_4.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S0_4.SectorGuid, S0_4 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, 8500 ), S0_4.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, 4500 ), S0_4.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -4000, 4500 ), S0_4.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -4000, 8500 ), S0_4.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S0_6{
			FGuid::NewGuid(),
			FName( "0_6" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_0_6/LA_TerraMagna_0_6.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S0_6.SectorGuid, S0_6 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -4000, 8500 ), S0_6.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -4000, 4500 ), S0_6.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, 4500 ), S0_6.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, 8500 ), S0_6.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S0_10{
			FGuid::NewGuid(),
			FName( "0_10" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_0_10/LA_TerraMagna_0_10.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S0_10.SectorGuid, S0_10 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, 8500 ), S0_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, 4500 ), S0_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 4500, 4500 ), S0_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 4500, 8500 ), S0_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S4_10{
			FGuid::NewGuid(),
			FName( "4_10" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_4_10/LA_TerraMagna_4_10.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S4_10.SectorGuid, S4_10 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, 4500 ), S4_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, 0 ), S4_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 4500, 0 ), S4_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 4500, 4500 ), S4_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S6_10{
			FGuid::NewGuid(),
			FName( "6_10" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_6_10/LA_TerraMagna_6_10.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S6_10.SectorGuid, S6_10 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, 0 ), S6_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, -4500 ), S6_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 4500, -4500 ), S6_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 4500, 0 ), S6_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S10_10{
			FGuid::NewGuid(),
			FName( "10_10" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_10_10/LA_TerraMagna_10_10.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S10_10.SectorGuid, S10_10 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, -4500 ), S10_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, -8500 ), S10_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 4500, -8500 ), S10_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 4500, -4500 ), S10_10.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S10_4{
			FGuid::NewGuid(),
			FName( "10_4" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_10_4/LA_TerraMagna_10_4.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S10_4.SectorGuid, S10_4 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, -4500 ), S10_4.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500, -8500 ), S10_4.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -4000, -8500 ), S10_4.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -4000, -4500 ), S10_4.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	{
		FSector S10_6{
			FGuid::NewGuid(),
			FName( "10_6" ),
			TerrainLayer.LayerGuid,
			TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_Background/LA_TerraMagna_10_6/LA_TerraMagna_10_6.umap" ),
			FColor( 0, 255, 0, 50 ),
			FColor( 0, 255, 0, 127 )
		};
		Sectors.Add( S10_6.SectorGuid, S10_6 );

		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -4000, -4500 ), S10_6.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( -4000, -8500 ), S10_6.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, -8500 ), S10_6.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		CurrPoint = MakeShareable( new FSectorPoint{ FGuid::NewGuid(), FVector2D( 500, -4500 ), S10_6.SectorGuid } );
		SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
	}
	for ( int32 i = 1; i <= 9; ++i )
		for ( int32 j = 1; j <= 9; ++j )
		{
			FString CommonSectorName;
			FColor DefaultColor( 255, 255, 255, 50 );
			FColor HoveredColor( 255, 255, 255, 127 );
			{
				if ( ( i >= 1 ) && ( i <= 2 ) && ( j >= 1 ) && ( j <= 2 ) )
				{
					CommonSectorName = TEXT( "EnergyPlant" );
					DefaultColor = FColor( 127, 40, 127, 50 );
				}
				if ( ( i >= 3 ) && ( i <= 4 ) && ( j >= 1 ) && ( j <= 2 ) )
				{
					CommonSectorName = TEXT( "Campus" );
					DefaultColor = FColor( 255, 127, 40, 50 );
				}
				if ( ( i >= 5 ) && ( i <= 6 ) && ( j >= 1 ) && ( j <= 2 ) )
				{
					CommonSectorName = TEXT( "Town" );
					DefaultColor = FColor( 0, 160, 232, 50 );
				}
				if ( ( i >= 7 ) && ( i <= 9 ) && ( j >= 1 ) && ( j <= 3 ) )
				{
					CommonSectorName = TEXT( "Institute" );
					DefaultColor = FColor( 255, 200, 14, 50 );
				}
				if ( ( i >= 1 ) && ( i <= 3 ) && ( j >= 3 ) && ( j <= 4 ) )
				{
					CommonSectorName = TEXT( "River_01" );
					DefaultColor = FColor( 0, 160, 232, 50 );
				}
				if ( ( i >= 4 ) && ( i <= 6 ) && ( j >= 3 ) && ( j <= 3 ) )
				{
					CommonSectorName = TEXT( "River_02" );
					DefaultColor = FColor( 127, 40, 127, 50 );
				}
				if ( ( i >= 4 ) && ( i <= 5 ) && ( j >= 4 ) && ( j <= 5 ) )
				{
					CommonSectorName = TEXT( "Lake" );
					DefaultColor = FColor( 255, 255, 0, 50 );
				}
				if ( ( i >= 1 ) && ( i <= 3 ) && ( j >= 5 ) && ( j <= 6 ) )
				{
					CommonSectorName = TEXT( "Airbase" );
					DefaultColor = FColor( 127, 40, 127, 50 );
				}
				if ( ( ( i >= 6 ) && ( i <= 9 ) && ( j >= 4 ) && ( j <= 5 ) ) || ( ( j == 6 ) && ( i >= 6 ) && ( i <= 7 ) ) )
				{
					CommonSectorName = TEXT( "City" );
					DefaultColor = FColor( 255, 174, 200, 50 );
				}
				if ( ( i >= 1 ) && ( i <= 3 ) && ( j >= 7 ) && ( j <= 9 ) )
				{
					CommonSectorName = TEXT( "Farms" );
					DefaultColor = FColor( 0, 160, 232, 50 );
				}
				if ( ( ( i >= 4 ) && ( i <= 6 ) && ( j >= 7 ) && ( j <= 7 ) ) || ( ( j == 6 ) && ( i >= 4 ) && ( i <= 5 ) ) )
				{
					CommonSectorName = TEXT( "River_03" );
					DefaultColor = FColor( 255, 200, 14, 50 );
				}
				if ( ( i >= 4 ) && ( i <= 6 ) && ( j >= 8 ) && ( j <= 9 ) )
				{
					CommonSectorName = TEXT( "Cargo" );
					DefaultColor = FColor( 255, 255, 0, 50 );
				}
				if ( ( ( i >= 7 ) && ( i <= 9 ) && ( j >= 7 ) && ( j <= 9 ) ) || ( ( j == 6 ) && ( i >= 8 ) && ( i <= 9 ) ) )
				{
					CommonSectorName = TEXT( "Quary" );
					DefaultColor = FColor( 0, 160, 232, 50 );
				}
				HoveredColor = DefaultColor.WithAlpha( 127 );
			}
			FSector CurrSector_ij{
				FGuid::NewGuid(),
				FName( FString::Printf( TEXT( "%d_%d" ), i, j ) ),
				TerrainLayer.LayerGuid,
				FString::Printf(
					TEXT( "Content/TerraMagna/LA_TerraMagna/LA_TerraMagna_%s/LA_TerraMagna_%d_%d/LA_TerraMagna_%d_%d.umap" ),
					*CommonSectorName,
					i,
					j,
					i,
					j ),
				DefaultColor,
				HoveredColor
			};
			Sectors.Add( CurrSector_ij.SectorGuid, CurrSector_ij );

			CurrPoint = MakeShareable(
				new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500 + 1000 * ( j - 1 ), 4500 - 1000 * ( i - 1 ) ), CurrSector_ij.SectorGuid } );
			SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
			CurrPoint = MakeShareable(
				new FSectorPoint{ FGuid::NewGuid(), FVector2D( -8500 + 1000 * ( j - 1 ), 3500 - 1000 * ( i - 1 ) ), CurrSector_ij.SectorGuid } );
			SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
			CurrPoint = MakeShareable(
				new FSectorPoint{ FGuid::NewGuid(), FVector2D( -7500 + 1000 * ( j - 1 ), 3500 - 1000 * ( i - 1 ) ), CurrSector_ij.SectorGuid } );
			SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
			CurrPoint = MakeShareable(
				new FSectorPoint{ FGuid::NewGuid(), FVector2D( -7500 + 1000 * ( j - 1 ), 4500 - 1000 * ( i - 1 ) ), CurrSector_ij.SectorGuid } );
			SectorPoints.Add( CurrPoint->PointGuid, *CurrPoint.Get() );
		}
}

void FSectorProvider::SaveConfig()
{
	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();

	// Check and create Config directory if not exists
	const FString ConfigPath = FPaths::Combine( FPaths::ProjectDir(), TEXT( "Config" ) );
	if ( !PlatformFile.DirectoryExists( *ConfigPath ) )
		PlatformFile.CreateDirectory( *ConfigPath );

	// Check and create MapSectorViewLock directory if not exist
	const FString MSVL_Path = FPaths::Combine( ConfigPath, TEXT( "MapSectorsConfig" ) );
	if ( !PlatformFile.DirectoryExists( *MSVL_Path ) )
		PlatformFile.CreateDirectory( *MSVL_Path );

	// For each map, create a subfolder with config files
	for ( auto& MapModelPair : Maps )
	{
		// Check and create each map directory if not exist
		const FString MapPath = FPaths::Combine( MSVL_Path, MapModelPair.Value.Name.ToString() );
		if ( !PlatformFile.DirectoryExists( *MapPath ) )
			PlatformFile.CreateDirectory( *MapPath );

		// Save map model in json object
		TSharedPtr< FJsonObject > JsonMapObject = MakeShared< FJsonObject >();
		JsonMapObject->SetStringField( TEXT( "Name" ), MapModelPair.Value.Name.ToString() );
		JsonMapObject->SetStringField( TEXT( "ContentPath" ), MapModelPair.Value.ContentPath );
		JsonMapObject->SetStringField( TEXT( "DefaultImage" ), MapModelPair.Value.DefaultImage );
		{
			TSharedPtr< FJsonObject > JsonSizeObject = MakeShared< FJsonObject >();
			JsonSizeObject->SetNumberField( TEXT( "X" ), MapModelPair.Value.DefaultImageSize.X );
			JsonSizeObject->SetNumberField( TEXT( "Y" ), MapModelPair.Value.DefaultImageSize.Y );
			JsonMapObject->SetObjectField( TEXT( "DefaultImageSize" ), JsonSizeObject );
		}

		TArray< TSharedPtr< FJsonValue > > JsonLayerValues;
		for ( auto& LayerModelPair : Layers )
		{
			// Check and create each layer directory if not exist
			FString LayerPath = FPaths::Combine( MapPath, LayerModelPair.Value.Name.ToString() );
			if ( !PlatformFile.DirectoryExists( *LayerPath ) )
				PlatformFile.CreateDirectory( *LayerPath );

			TSharedPtr< FJsonObject > JsonLayerObject = MakeShared< FJsonObject >();
			//JsonLayerObject->SetStringField("Guid", LayerModelPair.Value.LayerGuid.ToString());
			JsonLayerObject->SetStringField( TEXT( "Name" ), LayerModelPair.Value.Name.ToString() );
			JsonLayerObject->SetStringField( TEXT( "Image" ), LayerModelPair.Value.ImageBrushName );
			{
				TSharedPtr< FJsonObject > JsonSizeObject = MakeShared< FJsonObject >();
				JsonSizeObject->SetNumberField( TEXT( "X" ), LayerModelPair.Value.Size.X );
				JsonSizeObject->SetNumberField( TEXT( "Y" ), LayerModelPair.Value.Size.Y );
				JsonLayerObject->SetObjectField( TEXT( "ImageSize" ), JsonSizeObject );
			}
			{
				TSharedPtr< FJsonObject > JsonTopLeftObject = MakeShared< FJsonObject >();
				JsonTopLeftObject->SetNumberField( TEXT( "X" ), LayerModelPair.Value.TopLeft.X );
				JsonTopLeftObject->SetNumberField( TEXT( "Y" ), LayerModelPair.Value.TopLeft.Y );
				JsonLayerObject->SetObjectField( TEXT( "TopLeftReal" ), JsonTopLeftObject );
			}
			{
				TSharedPtr< FJsonObject > JsonBottomRightObject = MakeShared< FJsonObject >();
				JsonBottomRightObject->SetNumberField( TEXT( "X" ), LayerModelPair.Value.BottomRight.X );
				JsonBottomRightObject->SetNumberField( TEXT( "Y" ), LayerModelPair.Value.BottomRight.Y );
				JsonLayerObject->SetObjectField( TEXT( "BottomRightReal" ), JsonBottomRightObject );
			}
			JsonLayerObject->SetStringField( TEXT( "ParentMapContentPath" ), LayerModelPair.Value.ParentMapContentPath );

			TArray< TSharedPtr< FJsonValue > > JsonSectors;
			for ( auto SectorPair : Sectors )
				if ( SectorPair.Value.LayerGuid == LayerModelPair.Key )
				{
					TSharedPtr< FJsonObject > JsonSectorObject = MakeShared< FJsonObject >();

					//JsonSectorObject->SetStringField("SectorGuid", SectorPair.Key.ToString());
					JsonSectorObject->SetStringField( TEXT( "Name" ), SectorPair.Value.Name.ToString() );
					JsonSectorObject->SetStringField( TEXT( "LinkedMap" ), SectorPair.Value.LinkedMapContentPath );
					{
						TSharedPtr< FJsonObject > JsonColorObject = MakeShared< FJsonObject >();
						JsonColorObject->SetNumberField( TEXT( "R" ), SectorPair.Value.DefaultColor.R );
						JsonColorObject->SetNumberField( TEXT( "G" ), SectorPair.Value.DefaultColor.G );
						JsonColorObject->SetNumberField( TEXT( "B" ), SectorPair.Value.DefaultColor.B );
						JsonColorObject->SetNumberField( TEXT( "A" ), SectorPair.Value.DefaultColor.A );

						JsonSectorObject->SetObjectField( TEXT( "DefaultColor" ), JsonColorObject );
					}
					{
						TSharedPtr< FJsonObject > JsonColorObject = MakeShared< FJsonObject >();
						JsonColorObject->SetNumberField( TEXT( "R" ), SectorPair.Value.HoveredColor.R );
						JsonColorObject->SetNumberField( TEXT( "G" ), SectorPair.Value.HoveredColor.G );
						JsonColorObject->SetNumberField( TEXT( "B" ), SectorPair.Value.HoveredColor.B );
						JsonColorObject->SetNumberField( TEXT( "A" ), SectorPair.Value.HoveredColor.A );

						JsonSectorObject->SetObjectField( TEXT( "HoveredColor" ), JsonColorObject );
					}

					TArray< TSharedPtr< FJsonValue > > JsonPoints;
					for ( auto PointPair : SectorPoints )
						if ( SectorPair.Key == PointPair.Value.SectorGuid )
						{
							TSharedPtr< FJsonObject > JsonPointObject = MakeShared< FJsonObject >();
							JsonPointObject->SetNumberField( TEXT( "X" ), PointPair.Value.Point.X );
							JsonPointObject->SetNumberField( TEXT( "Y" ), PointPair.Value.Point.Y );

							JsonPoints.Add( MakeShareable( new FJsonValueObject( JsonPointObject ) ) );
						}
					JsonSectorObject->SetArrayField( TEXT( "Points" ), JsonPoints );

					JsonSectors.Add( MakeShareable( new FJsonValueString( SectorPair.Value.Name.ToString() ) ) );

					// Save sector config file
					FString SerializedJson;
					TSharedRef< TJsonWriter< > > JsonWriter = TJsonWriterFactory< >::Create( &SerializedJson );
					FJsonSerializer::Serialize( JsonSectorObject.ToSharedRef(), TJsonWriterFactory< >::Create( &SerializedJson ) );

					const FString SectorJsonPath = FPaths::Combine(
						LayerPath,
						FString::Printf( TEXT( "%s_SectorConfig.json" ), *SectorPair.Value.Name.ToString() ) );
					if ( !FFileHelper::SaveStringToFile( SerializedJson, *SectorJsonPath ) )
					{
						UE_LOG( LogTemp, Error, TEXT("File \"%s\" was not saved."), *SectorJsonPath );
					}
				}

			JsonLayerObject->SetArrayField( TEXT( "Sectors" ), JsonSectors );

			JsonLayerValues.Add( MakeShareable( new FJsonValueString( LayerModelPair.Value.Name.ToString() ) ) );

			// Save to layer file
			FString SerializedJson;
			TSharedRef< TJsonWriter< > > JsonWriter = TJsonWriterFactory< >::Create( &SerializedJson );
			FJsonSerializer::Serialize( JsonLayerObject.ToSharedRef(), TJsonWriterFactory< >::Create( &SerializedJson ) );

			FString LayerJsonPath = FPaths::Combine( LayerPath, FString::Printf( TEXT( "%s_LayerConfig.json" ), *LayerModelPair.Value.Name.ToString() ) );
			if ( !FFileHelper::SaveStringToFile( SerializedJson, *LayerJsonPath ) )
			{
				UE_LOG( LogTemp, Error, TEXT("File \"%s\" was not saved."), *LayerJsonPath );
			}
		}

		JsonMapObject->SetArrayField( "Layers", JsonLayerValues );

		// Save object to file
		FString SerializedJson;
		TSharedRef< TJsonWriter< > > JsonWriter = TJsonWriterFactory< >::Create( &SerializedJson );
		FJsonSerializer::Serialize( JsonMapObject.ToSharedRef(), TJsonWriterFactory< >::Create( &SerializedJson ) );

		FString MapJsonPath = FPaths::Combine( MapPath, FString::Printf( TEXT( "%s_MapConfig.json" ), *MapModelPair.Value.Name.ToString() ) );
		if ( !FFileHelper::SaveStringToFile( SerializedJson, *MapJsonPath ) )
		{
			UE_LOG( LogTemp, Error, TEXT("File \"%s\" was not saved."), *MapJsonPath );
		}
	}
}
