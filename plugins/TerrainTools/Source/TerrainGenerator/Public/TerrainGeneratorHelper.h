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

#include "Kismet/BlueprintFunctionLibrary.h"
#include "TerrainGeneratorHelper.generated.h"

class ALandscape;
class ALandscapeProxy;
class ULandscapeLayerInfoObject;
class ULandscapeInfo;

UCLASS()
class TERRAINGENERATOR_API UTerrainGeneratorHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, Category = "TerrainGeneratorHelper")
	static bool TerrainExportHeightmapToRenderTarget(
		ALandscape* InLandscape,
		UTextureRenderTarget2D* InRenderTarget,
		bool InExportHeightIntoRGChannel = false,
		bool InExportLandscapeProxies = true);

	UFUNCTION(BlueprintCallable, Category = "TerrainGeneratorHelper")
	static bool TerrainImportHeightmapFromRenderTarget(
		ALandscape* InLandscape,
		UTextureRenderTarget2D* InRenderTarget,
		bool InImportHeightFromRGChannel);

	UFUNCTION(BlueprintCallable, Category = "TerrainGeneratorHelper")
	static bool TerrainImportWeightmapFromRenderTarget(
		ALandscape* InLandscape,
		UTextureRenderTarget2D* InRenderTarget,
		FName InLayerName);

	UFUNCTION(BlueprintCallable, Category = "TerrainGeneratorHelper")
	static bool TerrainExportWeightmapToRenderTarget(
		ALandscape* InLandscape,
		UTextureRenderTarget2D* InRenderTarget,
		FName InLayerName);

	UFUNCTION(BlueprintCallable, Category = "TerrainGeneratorHelper")
	static void TerrainImportWeightmapFromRenderTargetToEditingLayer(
		ALandscapeProxy* InLandscapeProxy,
		UTextureRenderTarget2D* InRenderTarget,
		const FString& InEditingLayerName,
		const FString& InMaterialLayerName);

	UFUNCTION(BlueprintCallable, Category = "TerrainGeneratorHelper")
	static void ExportWeightLayer(ALandscape* InLandscape, FString Dir, FName InLayerName);

	UFUNCTION(BlueprintCallable, Category = "TerrainGeneratorHelper")
	static UStaticMesh* ConvertLandscapeToStaticMesh(ALandscapeProxy* Landscape);

	UFUNCTION(BlueprintCallable, Category = "TerrainGeneratorHelper")
	static bool BakeLandscape(
		ALandscapeProxy* Landscape,
		UMaterialInterface* Material,
		FIntPoint Size,
		int32 UVChannel,
		TArray<FColor>& OutData);

	static bool TerrainImportHeightmap(ALandscape* InLandscape, TArray<uint16>& InHeightDada, FIntPoint Size);

	static bool TerrainExportHeightmap(ALandscape* InLandscape, TArray<uint16>& OutHeightDada);

	// Create a dynamic texture intended to be used for passing non-texture data
	// into materials. Defaults to 32-bit RGBA. The texture is not added to the
	// root set, so something else will need to hold a reference to it.
	static UTexture2D* CreateTransientDynamicTexture(
		int32 Width,
		int32 Height,
		EPixelFormat PixelFormat = PF_A32B32G32R32F);

	// Updates a region of a texture with the supplied input data. Does nothing
	// if the pixel formats do not match.
	static void UpdateTextureRegion(
		UTexture2D* Texture,
		int32 MipIndex,
		FUpdateTextureRegion2D Region,
		uint32 SrcPitch,
		uint32 SrcBpp,
		uint8* SrcData,
		bool bFreeData);

	// Convenience wrapper for updating a dynamic texture with an array of FLinearColors.
	static void UpdateDynamicVectorTexture(const TArray<FLinearColor>& Source, UTexture2D* Texture);

	static TArray<FLinearColor> SampleRTData(UTextureRenderTarget2D* InRenderTarget, FLinearColor InRect);

	static TArray<FColor> DownsampleTextureData(TArray<FColor>& Data, int32 Width, int32 Kernel);

	static UTexture2D* CreateTextureFromBGRA(FColor* InData, int32 InWidth, int32 InHeight);

	// Copy the texture to the render target using the GPU
	static void CopyTextureToRenderTargetTexture(
		UTexture* SourceTexture,
		UTextureRenderTarget2D* RenderTargetTexture,
		ERHIFeatureLevel::Type FeatureLevel);

	static void GetLandscapeCenterAndExtent(const ALandscape* Landscape, FVector& OutCenter, FVector& OutExtent);

private:
	static void InternalImportWheightmapToEditingLayer(
		ULandscapeInfo* LandscapeInfo,
		ULandscapeLayerInfoObject* LandscapeLayerInfoObject,
		UTextureRenderTarget2D* InRenderTarget);
	static FMeshDescription* GetLandscapeDescription(ALandscapeProxy* Landscape);
	static void MergeRGBAndAlphaData(const TArray<FColor>& RGB, const TArray<FColor>& Alpha, TArray<FColor>& OutColor);
};
