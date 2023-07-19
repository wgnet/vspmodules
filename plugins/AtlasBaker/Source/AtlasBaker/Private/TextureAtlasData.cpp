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

#include "TextureAtlasData.h"

#include "Algo/Copy.h"
#include "AssetRegistryModule.h"
#include "Engine/Canvas.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "LayoutBuilders/UniformAtlasLayoutBuilder.h"
#include "SlateAtlasTile.h"

#include "Algo/MinElement.h"
#include "Editor.h"
#include "FileHelpers.h"
#include "UnrealEd/Public/ObjectTools.h"

UTextureAtlasData::UTextureAtlasData()
{
	LayoutBuilder = CreateDefaultSubobject<UUniformAtlasLayoutBuilder>(TEXT("LayoutBuilder"));
}

FVector4 UTextureAtlasData::GetUVScaleAndOffset(int32 TextureIdx)
{
	if (!Slots.IsValidIndex(TextureIdx))
		return FVector4();

	const FAtlasSlot& Slot = Slots[TextureIdx];
	return FVector4(Slot.UVScale.X, Slot.UVScale.Y, Slot.UVMin.X, Slot.UVMin.Y);
}

int32 UTextureAtlasData::GetTileIdxFromUV(const FVector2D& UV)
{
	int32 NearestTile = INDEX_NONE;
	int32 NearestSquaredDistance = MAX_int32;

	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		const FVector2D SlotOffset = Slots[i].UVMin;
		const float SquaredDistance = (UV - SlotOffset).SizeSquared();
		if (UV > SlotOffset && SquaredDistance < NearestSquaredDistance)
		{
			NearestTile = i;
			NearestSquaredDistance = SquaredDistance;
		}
	}

	return NearestTile;
}

void UTextureAtlasData::GenerateTextureAtlas()
{
	if (!LayoutBuilder)
		return;

	FlushRenderingCommands();
	if (DestinationTexture)
	{
		ObjectTools::ForceDeleteObjects({ DestinationTexture.LoadSynchronous() }, false);
		DestinationTexture = nullptr;
	}

	Textures.RemoveAll(
		[](UTexture2D* Texture)
		{
			return !IsValid(Texture);
		});

	const FAtlasLayout Layout = LayoutBuilder->Build(Textures);
	Slots = Layout.Slots;

	UWorld* World = GEditor->GetEditorWorldContext().World();
	UTextureRenderTarget2D* RenderTarget2D =
		UKismetRenderingLibrary::CreateRenderTarget2D(World, Layout.SizeX, Layout.SizeY, RTF_RGBA8_SRGB);
	UKismetRenderingLibrary::ClearRenderTarget2D(World, RenderTarget2D, FLinearColor { 0, 0, 0, 0 });

	UCanvas* Canvas;
	FVector2D Size;
	FDrawToRenderTargetContext Context;
	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(World, RenderTarget2D, Canvas, Size, Context);

	for (const FAtlasSlot& Slot : Layout.Slots)
	{
		Canvas->K2_DrawTexture(
			Slot.Texture,
			Slot.Region.Min,
			Slot.Region.GetSize(),
			FVector2D::UnitVector,
			FVector2D::UnitVector,
			FLinearColor::White,
			BLEND_AlphaComposite);
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);

	FString PackageName;
	FString FilenamePart;
	FString ExtensionPart;
	FPaths::Split(*GetPathName(), PackageName, FilenamePart, ExtensionPart);

	FString TextureName = TEXT("T_") + FilenamePart;
	PackageName = PackageName + TEXT("/");

	PackageName += TextureName;
	UPackage* Package = CreatePackage(*PackageName);
	Package->FullyLoad();

	UTexture2D* NewTexture = RenderTarget2D->ConstructTexture2D(
		Package,
		*TextureName,
		RF_Public | RF_Standalone,
		CTF_AllowMips | CTF_SRGB,
		nullptr);

	NewTexture->CompressionSettings = CompressionSettings;
	NewTexture->LODGroup = TextureGroup;
	NewTexture->SRGB = true;
	NewTexture->PostEditChange();
	Package->MarkPackageDirty();
	FAssetRegistryModule::AssetCreated(NewTexture);

	const FString PackageFileName =
		FPackageName::LongPackageNameToFilename(PackageName, FPackageName::GetAssetPackageExtension());
	bool bSaved = UPackage::SavePackage(
		Package,
		NewTexture,
		RF_Public | RF_Standalone,
		*PackageFileName,
		GError,
		nullptr,
		true,
		true,
		SAVE_NoError);

	DestinationTexture = NewTexture;
}

USlateAtlasTile* UTextureAtlasData::GetOrCreateSATFor(int32 SlotIdx)
{
	static const FString SlateAtlasTilePrefix = TEXT("SAT");

	USlateAtlasTile* SlateAtlasTile = nullptr;
	if (Slots.IsValidIndex(SlotIdx))
	{
		if (SlateAtlasTiles.IsValidIndex(SlotIdx))
		{
			SlateAtlasTile = SlateAtlasTiles[SlotIdx].LoadSynchronous();
		}
		else
		{
			const FAtlasSlot& Slot = Slots[SlotIdx];

			FString TextureName = FPaths::GetBaseFilename(Slot.Texture->GetPathName());
			TextureName.RemoveFromStart(TEXT("T_"), ESearchCase::CaseSensitive);
			const FString AssetName =
				FString::Format(TEXT("{0}_{1}"), { SlateAtlasTilePrefix, FPaths::GetBaseFilename(TextureName) });

			const FString PackageName = FString::Format(TEXT("{0}/{1}"), { FPaths::GetPath(GetPathName()), AssetName });

			UPackage* Package = CreatePackage(*PackageName);
			SlateAtlasTile = NewObject<USlateAtlasTile>(Package, *AssetName, RF_Standalone | RF_Public);
			Package->MarkPackageDirty();

			FAssetRegistryModule::AssetCreated(SlateAtlasTile);
		}
	}

	return SlateAtlasTile;
}

void UTextureAtlasData::GenerateSlateAtlasTiles()
{
	TArray<TSoftObjectPtr<USlateAtlasTile>> NewSlateAtlasTile;
	for (int32 i = 0; i < Slots.Num(); ++i)
	{
		USlateAtlasTile* SlateAtlasTile = GetOrCreateSATFor(i);
		SlateAtlasTile->Texture = DestinationTexture.Get();
		SlateAtlasTile->Region = FBox2D(Slots[i].UVMin, Slots[i].UVMax);

		SlateAtlasTile->MarkPackageDirty();

		NewSlateAtlasTile.Add(SlateAtlasTile);
		SlateAtlasTiles.Remove(SlateAtlasTile);
	}

	for (int32 i = SlateAtlasTiles.Num() - 1; i >= 0; --i)
	{
		USlateAtlasTile* SlateAtlasTile = SlateAtlasTiles[i].LoadSynchronous();
		if (SlateAtlasTile)
		{
			ObjectTools::ForceDeleteObjects({ SlateAtlasTile }, false);
		}
	}

	SlateAtlasTiles = NewSlateAtlasTile;
}

UTextureAtlasData* UTextureAtlasData::FindDataFromAtlas(UTexture2D* Atlas)
{
	if (!Atlas)
		return nullptr;

	TArray<FAssetData> AllAssets;
	IAssetRegistry::GetChecked().GetAllAssets(AllAssets);

	TArray<FName> OutNames;
	IAssetRegistry::GetChecked().GetDependencies(Atlas->GetPackage()->FileName, OutNames);
	for (FAssetData AssetData : AllAssets)
	{
		if (AssetData.GetClass() == StaticClass())
		{
			auto AtlasData = CastChecked<UTextureAtlasData>(AssetData.GetAsset());
			if (AtlasData->DestinationTexture == Atlas)
			{
				return AtlasData;
			}
		}
	}

	return nullptr;
}
