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
#include "InteractiveTool.h"
#include "InteractiveToolBuilder.h"
#include "Materials/MaterialInstanceConstant.h"
#include "InLayerMaskBakeTool.generated.h"

class UMaskBakeProperties;

UCLASS()
class VSPMESHPAINTER_API UInLayerMaskBakeToolBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};

UCLASS()
class UInLayerMaskBakeTool : public UInteractiveTool
{
	GENERATED_BODY()

public:
	void Init(const FToolBuilderState& SceneState);
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	void WriteToRenderTarget(
		int TargetLayer,
		UTexture* Mask,
		UTexture* Mask2,
		UMaterialInstanceDynamic* MaterialInstanceDynamic,
		UTextureRenderTarget2D* TargetRT,
		FBox2D BakeArea);

private:
	FToolBuilderState ToolBuilderInitialState {};
	FIntPoint TextureSize = FIntPoint::ZeroValue;

	UPROPERTY()
	UMaskBakeProperties* BakeProperties = nullptr;

	UPROPERTY()
	UMeshComponent* MeshComp = nullptr;

	UPROPERTY()
	UMaterialInstance* InitialMeshMaterial = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* InitialLayersMaskRT = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* LayersMaskRT = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* BrushRT = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* MaskForMultFromInitialRT = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MeshDMI = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* MaskBakeDMI = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* AccumulateDMI = nullptr;

	void ApplyMaskToLayersMaskRT();
	void CommitLayersMaskRTToModifyTexture();
	FBox2D GetBakeArea(int32 LayersCount, int32 TargetLayer);

	const FLinearColor BrushRT_ClearColor = FLinearColor(0, 0, 0, 1);

	UPROPERTY()
	UTexture2D* WhiteSquare;
};
