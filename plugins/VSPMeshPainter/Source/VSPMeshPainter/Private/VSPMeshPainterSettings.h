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
#include "EditorUtilityWidgetBlueprint.h"
#include "Engine/DeveloperSettings.h"
#include "VSPMeshPainterSettings.generated.h"

class UMaterialInstanceConstant;

namespace VSPMeshPainterExpressions
{
	static const FString LayerToTextureIdxPostfix = TEXT("_LAYER_TO_TEXTURE_IDX");
	static const FString AlphaToTextureIdxPostfix = TEXT("_ALPHA_TO_TEXTURE_IDX");
	static const FString AlphaToTextureChannelPostfix = TEXT("_ALPHA_CHANNEL");
	static const FString AlphaToTintColorPostfix = TEXT("_ALPHA_COLOR");
	static const FString AlphaToRoughnessPostfix = TEXT("_ALPHA_ROUGHNESS");
	static const FName MaterialArrayParamName = TEXT("MATERIAL_ARRAY");
	static const FName NRMXArrayParamName = TEXT("NRMX");
	static const FName BrushLocationParam = TEXT("BRUSH_LOCATION");
	static const FName BrushRadiusParam = TEXT("BRUSH_RADIUS");
	static const FName BrushFalloffParam = TEXT("BRUSH_FALLOFF");
	static const FName BrushStrengthParam = TEXT("BRUSH_STRENGTH");
	static const FName MaskIDParam = TEXT("MASK_ID");
	static const FName PaintTintBackupColorParam = TEXT("PAINT_TINT_BACKUP_COLOR");
	static const FName UseBrushDilationParam = TEXT("USE_BRUSH_DILATION");
	static const FName MaskSizeXParam = TEXT("MASK_SIZE_X");
	static const FName MaskSizeYParam = TEXT("MASK_SIZE_Y");
	static const FName BrushNormalParam = TEXT("BRUSH_NORMAL");
	static const FName BrushAlphaMaskParam = TEXT("BRUSH_ALPHA_MASK");
	static const FName BrushAlphaMaskRotationParam = TEXT("BRUSH_ALPHA_MASK_ROTATION");
	static const FName BUseBrushAlphaMaskParam = TEXT("B_USE_BRUSH_ALPHA_MASK");
	static const FName BrushMaskParam = TEXT("MASK");
	static const FName BrushMask2Param = TEXT("MASK_2");
	static const FName AccumulatedMaskParam = TEXT("MASK");
	static const FName InitialMaskParam = TEXT("INITIAL_MASK");
	static const FName SeamMaskParam = TEXT("SEAM_MASK");
	static const FName SmearApplyModeParam = TEXT("SMEAR_APPLY_MODE");
	static const FName BInvertBrushMaskParam = TEXT("B_INVERT_BRUSH_MASK");
	static const FName BrushMaskContrastParam = TEXT("BRUSH_MASK_CONTRAST");
	static const FName MaskSmearApplyByCurrentDistributionParam = TEXT("MASK_SMEAR_APPLY_BY_CURRENT_DISTRIBUTION");
	static const FName UnwrapCaptureSizeParam = TEXT("UNWRAP_CAPTURE_SIZE");
	static const FName UnwrapLocationParam = TEXT("UNWRAP_LOCATION");

	static const FName UVBrushWireframePreviewRTParameterName = TEXT("Texture_UVBrushWireframePreviewRT");
	static const FName UVPreviewRTParameterName = TEXT("Texture_UVPreviewRT");
	static const FName UV_ResolutionRatioParameterName = TEXT("UV_ResolutionRatio");
	static const FName ColorParameterName = TEXT("Color");
	static const FName Texture0ParameterName = TEXT("Texture_0");
	static const FName ChannelIndexParameterName = TEXT("Channel_Index");
	static const FName UV_ShiftParameterName = TEXT("UV_Shift");
	static const FName BrushPositionUVSpaceParameterName = TEXT("BrushPosition_UVSpace");
	static const FName ZoomParameterName = TEXT("Zoom");
	static const FName RawChannelsPreviewParameterName = TEXT("RawChannelsPreview");
	static const FName BrushRadiusUVSpaceParameterName = TEXT("BrushRadius_UVSpace");
	static const FName BrushStrengthMultParameterName = TEXT("BrushStrengthMult");

	const ERHIFeatureLevel::Type SupportedFeatureLevel = ERHIFeatureLevel::SM5;
}

USTRUCT(BlueprintType)
struct FSupportedMaterialParam
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterialInterface> SupportedToPaint;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterial> Brush;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterial> Accumulator;

	UPROPERTY(EditAnywhere)
	TSoftObjectPtr<UMaterial> UnwrapMaterial;
};

USTRUCT()
struct FAlphaLayerData
{
	GENERATED_BODY()

	int32 TextureIndex;
	FLinearColor TintColor;
	FLinearColor TextureChannel;
	float Roughness;
};

//TODO: Refactor with using VSPCheck
UCLASS(config = Engine, defaultconfig)
class VSPMESHPAINTER_API UVSPMeshPainterSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Config, Category = ShortcutSettings)
	float RadiusChangeSpeed;

	UPROPERTY(EditAnywhere, Config, Category = ShortcutSettings)
	float FalloffChangeSpeed;

	UPROPERTY(EditAnywhere, Config, Category = ShortcutSettings)
	float StrengthChangeSpeed;

	UPROPERTY(EditAnywhere, Config, Category = ShortcutSettings)
	float RotationChangeSpeed;

	UPROPERTY(EditAnywhere, Config, Category = ShortcutSettings)
	float RadiusChangeDelta;


	static UVSPMeshPainterSettings* Get();
	static FName GetLayerToTextureIdxParamName(int32 LayerIdx);
	static FName GetAlphaToTextureIdxParamName(int32 LayerIdx);
	static FName GetAlphaToTextureChannelParamName(int32 LayerIdx);
	static FName GetAlphaToTintColorParamName(int32 LayerIdx);
	static FName GetAlphaToRoughnessParamName(int32 LayerIdx);
	static FName GetPaintTintBackupParamName();

	bool IsSupportToPaint(UMeshComponent* MeshComponent);
	UMaterialInstanceConstant* GetSupportedMaterialFor(UMeshComponent* MeshComponent);
	UTexture2D* GetToPaintTextureFor(UMeshComponent* MeshComponent) const;
	UMaterial* GetBrushFor(UMeshComponent* MeshComponent);
	UMaterial* GetAccumulatorFor(UMeshComponent* MeshComponent);
	UMaterial* GetUnwrapMaterialFor(UMeshComponent* MeshComponent);
	UTexture2DArray* GetMaterialArrayFor(UMeshComponent* MeshComponent) const;
	TArray<int32> GetLayerToTextureMapFor(UMaterialInstanceConstant* Material) const;
	TArray<FAlphaLayerData> GetAlphaToTextureMapFor(UMaterialInstanceConstant* Material) const;

	UMaterial* GetMaskBakeMaterial() const;

	UEditorUtilityWidgetBlueprint* GetUVWindowWidgetBlueprint();

private:
	UPROPERTY(EditAnywhere, Config)
	TArray<FSupportedMaterialParam> SupportedMaterials;

	UPROPERTY(EditAnywhere, Config)
	TSoftObjectPtr<UMaterial> MaskBakeMaterial;

	UPROPERTY(EditAnywhere, Config /*AdvancedDisplay, Category = TexturePainting*/)
	TSoftObjectPtr<UEditorUtilityWidgetBlueprint> UVWindowWidgetBlueprint;

	UMaterialInstanceConstant* GetMaterialInstanceConstant(UMeshComponent* MeshComponent) const;
	FSupportedMaterialParam* GetSupportedMaterialParamFor(UMeshComponent* MeshComponent);
};

UCLASS()
class VSPMESHPAINTER_API UVSPMeshPainterModeDummySettings : public UObject
{
	GENERATED_BODY()
};
