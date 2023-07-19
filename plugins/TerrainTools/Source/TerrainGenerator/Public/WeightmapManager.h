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

#include "GameFramework/Actor.h"
#include "Landscape.h"
#include "WeightmapManager.generated.h"

class UTGBaseLayer;

UENUM()
enum EWeightmapCompositeMode
{
	WBM_Subtractive,
	WBM_Additive,
	WBM_Multiplied
};

USTRUCT()
struct FTGLayerMask
{
	GENERATED_BODY()

	UPROPERTY(EditInstanceOnly)
	FName Layer;

	UPROPERTY(EditInstanceOnly)
	TSubclassOf<UTGBaseLayer> GeneratorType;

	UPROPERTY(EditInstanceOnly)
	UTGBaseLayer* Generator = nullptr;
};

UCLASS(hidecategories = (LOD, Rendering, Replication, Collision, Input, Actor, Cooking))
class TERRAINGENERATOR_API AWeightmapManager : public AActor
{
	GENERATED_BODY()

public:
	AWeightmapManager();

	UFUNCTION(CallInEditor, Category = "Generation")
	void Generate();

	UFUNCTION(CallInEditor, Category = "Generation")
	void Reapply();

	UFUNCTION(CallInEditor, Category = "Generation")
	void InitializeManager();

	UPROPERTY(EditInstanceOnly, Category = "Generation")
	TSoftObjectPtr<ALandscape> Landscape;

	UPROPERTY(VisibleAnywhere, Category = "Generation")
	FIntPoint Resolution;

	UPROPERTY(VisibleAnywhere, Category = "Generation")
	FName EditingLayerName;

	UPROPERTY(EditInstanceOnly, Category = "Generation")
	bool bComposite = false;

	UPROPERTY(EditInstanceOnly, Category = "Generation")
	bool bExportNormals = false;

	UPROPERTY(EditInstanceOnly, Category = "Generation")
	bool bStreamingProxy = false;

	UPROPERTY(EditInstanceOnly, Instanced, Category = "Generators")
	TMap<FName, UTGBaseLayer*> Generators;

	UPROPERTY(VisibleInstanceOnly, AdvancedDisplay, Category = "Layers")
	TMap<FName, UTextureRenderTarget2D*> LayerStack;

	UFUNCTION(CallInEditor, Category = "Layers")
	void ExportOnDisk();

	UPROPERTY(EditInstanceOnly, Category = "Layers")
	FDirectoryPath ExportDirectory = { "D:/TerrainLayers" };

	UPROPERTY(EditInstanceOnly, Category = "Debug")
	bool bCleanMemory = false;

protected:
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	void UpdateSceneCapture() const;

private:
	UPROPERTY(Transient)
	UTextureRenderTarget2D* PackedHeightRT;

	UPROPERTY(Transient)
	UTextureRenderTarget2D* ObjectDepth;

	UPROPERTY(Transient)
	UTextureRenderTarget2D* TerrainDepth;

	UPROPERTY(Transient)
	TArray<float> DecodedHeight;

	UPROPERTY(Transient)
	TArray<uint8> ObjectsDepth;

	UPROPERTY()
	USceneCaptureComponent2D* SceneCapture;

	UTextureRenderTarget2D* SubtractWeightmap(
		UTextureRenderTarget2D* InRenderTarget,
		UTextureRenderTarget2D* InRenderTargetB);
	static UTexture2D* GenerateWeightmapTexture(UTextureRenderTarget2D* InRenderTarget);
	void Composite();
	void GetPackedHeightRT(UTextureRenderTarget2D*& InRenderTarget);
	TArray<uint8> UpdateObjectsDepth(FName Tag);
	TArray<ULandscapeComponent*> GetLandscapeComponents() const;
	TArray<uint16> GetPackedHeight() const;
	TArray<float> GetUnpackedHeight() const;
	FName GetBaseLayerName() const;
};
