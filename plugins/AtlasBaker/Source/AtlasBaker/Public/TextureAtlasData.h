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
#include "AtlasSlot.h"

#include "TextureAtlasData.generated.h"

class UAtlasLayoutBuilder;
class USlateAtlasTile;

UCLASS()
class ATLASBAKER_API UTextureAtlasData : public UObject
{
	GENERATED_BODY()

public:
	UTextureAtlasData();

	UPROPERTY(EditDefaultsOnly, Instanced)
	UAtlasLayoutBuilder* LayoutBuilder;

	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<enum TextureCompressionSettings> CompressionSettings = TC_BC7;

	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<TextureGroup> TextureGroup = TEXTUREGROUP_World;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite)
	TSoftObjectPtr<UTexture2D> DestinationTexture;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay)
	TArray<FAtlasSlot> Slots;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TArray<UTexture2D*> Textures;

	UFUNCTION(BlueprintCallable)
	FVector4 GetUVScaleAndOffset(int32 TextureIdx);

	UFUNCTION(BlueprintCallable)
	int32 GetTileIdxFromUV(const FVector2D& UV);

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Actions")
	void GenerateTextureAtlas();

	UFUNCTION(CallInEditor, Category = "UI")
	void GenerateSlateAtlasTiles();

	static UTextureAtlasData* FindDataFromAtlas(UTexture2D* Atlas);

private:
	UPROPERTY()
	TArray<TSoftObjectPtr<USlateAtlasTile>> SlateAtlasTiles;

	USlateAtlasTile* GetOrCreateSATFor(int32 SlotIdx);
};
