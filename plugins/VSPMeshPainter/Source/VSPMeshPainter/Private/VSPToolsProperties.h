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
#include "BaseTools/BaseBrushTool.h"
#include "Materials/MaterialInstanceConstant.h"
#include "VSPMeshPainterSettings.h"
#include "VSPToolsProperties.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FLayerCountChanged, int32, int32);

UENUM()
enum class EBrushFunction : uint8
{
	//Add brush on top of base
	Add,
	//Subtract brush from base
	Sub,
	//Keeps the maximum value between brush and base
	Max,
	//Keeps minimum value between brush and base
	Min,
	//Multiplies brush over base
	Mul,
	//Brush completely overrides base
	Replace
};

UENUM()
enum class EMaterialLayerType : uint8
{
	MaterialLayer,
	AlphaBlendLayer
};

UCLASS()
class UVSPBrushToolProperties : public UBrushBaseProperties
{
	GENERATED_BODY()

public:
	FSimpleMulticastDelegate OnLayerUpdate;
	FSimpleMulticastDelegate OnUVLevelUpdate;
	FSimpleMulticastDelegate OnSelectedTileChanged;

	DECLARE_MULTICAST_DELEGATE_OneParam(FloatCallbackDelegate, float)
	FloatCallbackDelegate OnBrushRadiusChanged;
	FloatCallbackDelegate OnBrushStrengthChanged;
	FloatCallbackDelegate OnSmearApplyModeChanged;

	FSimpleMulticastDelegate OnShutdownDelegate;
	FDelegateHandle ShutdownLambdaHandleForPalette;


	UPROPERTY(VisibleAnywhere)
	TArray<FLinearColor> TintsArray;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	int32 UVChannel;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Brush)
	UTexture2D* BrushAlphaMask = nullptr;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Brush, meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0", DisplayPriority = 2))
	float Rotation = 0;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Brush, meta = (UIMin = "0.0", UIMax = "3.0", ClampMin = "0.0", ClampMax = "3.0", DisplayPriority = 3))
	float Step = 0;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = Brush, meta = (DisplayPriority = 4))
	bool bUseBrushDilation = true;

	UPROPERTY(EditAnywhere, Category = BrushSpreading, meta = (UIMin = "0.0", UIMax = "3.0", ClampMin = "0.0", ClampMax = "3.0"))
	float StepSpread = 0;

	UPROPERTY(EditAnywhere, Category = BrushSpreading, meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float RotationSpread = 0;

	UPROPERTY(EditAnywhere, Category = BrushSpreading, meta = (UIMin = "0.0", UIMax = "1.0", ClampMin = "0.0", ClampMax = "1.0"))
	float ScaleSpread = 0;

	UPROPERTY(EditAnywhere, Category = BrushSpreading, meta = (UIMin = "0.0", UIMax = "3.0", ClampMin = "0.0", ClampMax = "3.0"))
	float TranslationSpread = 0;

	UPROPERTY(EditAnywhere, Category = Dilation, meta = (UIMin = "0", UIMax = "16", ClampMin = "0", ClampMax = "16"))
	int32 TrimMateriansNumber = 0;

	UPROPERTY(EditAnywhere, Category = Dilation)
	bool bUseUpscaledDilation;

	UPROPERTY(EditAnywhere, Category = Brush)
	EBrushFunction Function;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UMaterialInterface* BrushMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UMaterialInterface* AccumulatorMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UMaterialInterface* UnwrapMaterial = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UMaterialInstanceConstant* MaterialToModify = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UTexture2DArray* TextureArray = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UTexture2D* TextureToModify = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UTextureRenderTarget2D* SeamMaskRT = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UTextureRenderTarget2D* AccumulatedRT = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UTextureRenderTarget2D* BrushRT = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	UTextureRenderTarget2D* DragInitialRT = nullptr;

	UPROPERTY(VisibleAnywhere, AdvancedDisplay, Category = TexturePainting)
	int32 FirstTintCustomDataIndex = 5;

	UPROPERTY(EditAnywhere, AdvancedDisplay, Category = TexturePainting)
	int32 UVPreviewTextureSize = 2048;

	UPROPERTY()
	TArray<int32> LayerToTextureIdx {};

	UPROPERTY()
	TArray<FAlphaLayerData> AlphaDataToTexture {};

	FLayerCountChanged OnLayerCountChanged;

	void SetupFor(UMeshComponent* MeshComponent);
	void Reset();
	void SelectTile(int32 TileIndex, EMaterialLayerType MaterialLayerType);
	int32 SetTextureIdxForLayer(int32 LayerIdx, int32 TextureIdx, EMaterialLayerType MaterialLayerType);
	FLinearColor ShiftLayerTintColor(int32 LayerIdx, int32 Offset);
	FLinearColor SetLayerTextureChannel(int32 LayerIdx, FLinearColor TextureChannel);
	float SetLayerRoughness(int32 LayerIdx, float Roughness);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void ShiftRadius(float Value);
	void ShiftFalloff(float Value);
	void ShiftStrength(float Value);
	void ShiftRotation(float Value);

	int32 GetSelectedTileIndex();
	EMaterialLayerType GetSelectedLayerType();
	int32 GetMaterialLayersCount();
	int32 GetAlphaLayersCount();
	int32 GetSelectedLayerID();

	bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	bool InputDelta(
		FEditorViewportClient* InViewportClient,
		FViewport* InViewport,
		FVector& InDrag,
		FRotator& InRot,
		FVector& InScale);
	bool HandleMouseShortcut(int MouseKeyIndex, const FVector2D& MouseDelta);
	bool HandleKeyShortcut(FKey Key);

private:
	int32 SelectedTile = 0;
	EMaterialLayerType SelectedLayerType = EMaterialLayerType::MaterialLayer;
	int32 MaterialLayersCount = 0;
	int32 AlphaLayersCount = 0;
};

UCLASS()
class UMaskBakeProperties : public UObject
{
	GENERATED_BODY()

public:
	FSimpleDelegate OnUpdate;
	FSimpleDelegate OnApplyBake;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void SetupForMeshComp(UMeshComponent* MeshComp);

	UPROPERTY(EditAnywhere, meta = (UIMin = "0", UIMax = "15", ClampMin = "0", ClampMax = "15"))
	int32 TargetLayer = 0;

	UPROPERTY(EditAnywhere)
	bool UseMaskMultiplication = false;

	UPROPERTY(EditAnywhere, meta = (UIMin = "0", UIMax = "15", ClampMin = "0", ClampMax = "15"))
	int32 TargetLayer_ForMaskMultiplication = 0;

	UPROPERTY(EditAnywhere, meta = (UIMin = "-1.0", UIMax = "1.0", ClampMin = "-1.0", ClampMax = "1.0"))
	float MaskWhiteOverlayValue = 0;

	UPROPERTY(EditAnywhere)
	UTexture2D* Mask;

	UPROPERTY(EditAnywhere)
	bool bInvertMask = false;

	UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", UIMax = "10.0", ClampMin = "0.0", ClampMax = "10.0"))
	float Contrast = 1.f;

	UPROPERTY(EditAnywhere, meta = (UIMin = "0.0", UIMax = "10.0", ClampMin = "0.0", ClampMax = "10.0"))
	float Strength = 1.f;

	UPROPERTY(/*EditAnywhere, AdvancedDisplay*/) //editing temporary disabled
	EBrushFunction Function = EBrushFunction::Add;

	UPROPERTY(/*EditAnywhere, AdvancedDisplay*/) //editing temporary disabled
	bool bMaskSmearApplyByCurrentDistribution = false;

	int32 LayersCount = 0;

	UPROPERTY()
	UTexture2D* TextureToModify = nullptr;

	UPROPERTY()
	UMaterialInstanceConstant* MaterialToModify = nullptr;

	UPROPERTY()
	UMaterial* AccumulateMat = nullptr;
};

struct FSmearProperties
{
	FVector Location = FVector::ZeroVector;
	FVector Normal = FVector::ZeroVector;
	float Size = 0.f;
	float Rotation = 0.f;
};

struct FBrushSpreadVariation
{
	FVector2D Translation = FVector2D::ZeroVector;
	float Rotation = 0.f;
	float Scale = 1.f;
	float NextPaintDistance = 0.f;
	UVSPBrushToolProperties* BrushProperties = nullptr;

	FBrushSpreadVariation() = delete;
	FBrushSpreadVariation(UVSPBrushToolProperties* InBrushProperties);

	FSmearProperties GetSmearProperties(const FHitResult& Hit) const;
};
