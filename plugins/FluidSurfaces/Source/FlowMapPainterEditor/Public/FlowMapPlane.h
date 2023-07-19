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

#include "GameFramework/Actor.h"
#include "FlowMapPlane.generated.h"

UCLASS()
class FLOWMAPPAINTEREDITOR_API AFlowMapPlane : public AActor
{
	GENERATED_BODY()

public:
	AFlowMapPlane();

	void DrawTexture(FVector ForceLocation, FVector ForceVelocity, float BrushSize, float Density, bool bErase);
	void SetDrawPlane(UStaticMeshComponent* InComponent);

	UFUNCTION(CallInEditor, Category = Setup)
	void ClearTexture();

	UFUNCTION(CallInEditor, Category = Setup)
	void SaveTexture();

	UFUNCTION(CallInEditor, Category = Setup)
	void FillTexture();

	UFUNCTION(CallInEditor, Category = Setup)
	void CopyTexture();

	UPROPERTY(EditAnywhere, Category = Setup)
	bool bShowPreview = true;

	UPROPERTY(EditAnywhere, Category = Setup)
	FLinearColor FillColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, Category = Setup)
	FIntPoint WorldSize = 204800;

	UPROPERTY(EditAnywhere, Category = Setup)
	FIntPoint TextureSize = 2048;

	UPROPERTY(EditAnywhere, Category = Setup, meta = (DisplayThumbnail = "false"))
	UTexture2D* OutputTexture;

	UPROPERTY(EditAnywhere, Category = Setup, meta = (ContentDir))
	FString OutputTextureName = TEXT("T_Flowmap");

	UPROPERTY(EditAnywhere, Category = Setup, meta = (ContentDir))
	FDirectoryPath OutputDirectory = FDirectoryPath { TEXT("/Game/Environment/Flowmaps") };

	UPROPERTY(VisibleInstanceOnly, Category = Debug)
	UTextureRenderTarget2D* FlowMapRT = nullptr;

private:
	UPROPERTY()
	UMaterialInterface* ForceMaterial = nullptr;

	UPROPERTY()
	UMaterialInterface* CopyTextureMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* ForceMID = nullptr;

	UPROPERTY()
	UStaticMeshComponent* DrawPlane;

	virtual void OnConstruction(const FTransform& Transform) override;
};
