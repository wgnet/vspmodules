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
#include "Components/Image.h"
#include "EditorUtilityWidget.h"
#include "VSPMeshPaintBrush.h"
#include "Tools/UEdMode.h"
#include "VSPMeshPaintUVWindow.generated.h"

enum DragButton
{
	Empty = -1,
	Left = 0,
	Right = 1,
	Middle = 2
};

/**
 * 
 */
UCLASS()
class VSPMESHPAINTER_API UVSPMeshPaintUVWindow : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	UPROPERTY()
	UVSPMeshPaintBrush* MeshPaintBrush;

	UPROPERTY()
	UMaterialInstanceDynamic* UVPreviewMaterialPtr;

	UPROPERTY()
	UMaterialInstanceDynamic* UVAccumulateDynamicMaterial;

	UPROPERTY()
	UTextureRenderTarget2D* DragInitialRT_Ptr;

	UPROPERTY()
	UTextureRenderTarget2D* BrushRT_Ptr;

	UPROPERTY()
	UTextureRenderTarget2D* AccumulatedRT_Ptr;

	UPROPERTY(meta = (BindWidget))
	UImage* MainImage;

	UPROPERTY(EditAnywhere)
	UTexture2D* UVBrushAlphaMask;

	UPROPERTY(EditAnywhere)
	UMaterial* BrushMaterialPtr;

	UPROPERTY(EditAnywhere)
	UMaterialInstance* UVWireframeMatPtr;

	UPROPERTY()
	UMaterialInstanceDynamic* BrushDynamicMaterialInstancePtr;

	UPROPERTY(EditAnywhere)
	int32 ChannelToEdit = 0;

	UPROPERTY(EditAnywhere)
	float ZoomSpeed = 0.01;

	UPROPERTY(EditAnywhere)
	float UVPanSpeed = 0.01;

	UPROPERTY(EditAnywhere)
	float UVZoomFocusMult = 1;

	UPROPERTY(EditAnywhere)
	double BrushPreviewInitialSize = 0.05;

	UPROPERTY(EditAnywhere)
	float BrushStrengthModifier = 1.f;

	UPROPERTY(EditAnywhere)
	TEnumAsByte<EBlendMode> BrushBlendMode = EBlendMode::BLEND_Translucent;

private:
	DragButton ActiveDragState = Empty;
	bool bIsFocused;
	FVector UVCenterShift;
	FVector2D BrushUVPosition;
	float ZoomAmount = 0.f;
	float RawChannelsPreview = 0.f;
	float BrushSizeMult = 1.f;
	float SmearApplyMode = 0.f;
	int32 TextureSizeX = 0;
	int32 TextureSizeY = 0;
	FName TabId;
	bool bPaintModified = false;
	bool bDragFinished = true;

	FDelegateHandle ShutdownLambdaHandle;
	FDelegateHandle OnLayerUpdateHandle;
	FDelegateHandle OnSelectedTileChangedHandle;
	FDelegateHandle OnBrushRadiusChangedHandle;
	FDelegateHandle OnBrushStrengthChangedHandle;
	FDelegateHandle OnSmearApplyModeChangedHandle;

	const FLinearColor BrushChannelColors[5] = {
		FLinearColor::Red, FLinearColor::Green, FLinearColor::Blue, FLinearColor(0, 0, 0, 1), FLinearColor(0, 0, 0, 0),
	};


public:
	void SetupUVPreviewMaterial();
	void Setup(FName TabIdVal);
	void SetMaterialProperties();

	UFUNCTION(BlueprintCallable)
	void On_MaskPreviewMode_SliderValueChanged(float Val);

	UFUNCTION(BlueprintCallable)
	void On_BrushSize_SliderValueChanged(float Val);

	UFUNCTION(BlueprintCallable)
	void On_UndoLastSmear_ButtonPressed();

	void PaintOnUV(FVector2D UVPosition, int Channel);
	void ApplyBrushRTToAccumulatedRT();

	FVector2D PanUV(
		FVector2D UV,
		const float StepMult = 1,
		const int UVChunkIndexOverride = -1,
		const bool bReverse = false) const;
	FInputDeviceRay DeviceRayFromUvPosition(FVector2D UVPosition) const;
	void FakeBrushMovement(FVector2D UVPosition) const;
	void SetUVCenterShift(FVector NewUVPosition);
	bool FocusEditorCamera() const;
	void FocusUVWindowViaPainterBrushPosition(TArray<FVector2D> UVArray);
	void FocusUVWindowViaPainterBrushPosition(
		FVector TraceStart,
		FVector TraceEnd,
		IMeshPaintComponentAdapter* MeshAdapter);

	virtual void BeginDestroy() override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnFocusLost(const FFocusEvent& InFocusEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
};
