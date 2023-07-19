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

#include "VSPMeshPaintUVWindow.h"

#include "CoreMinimal.h"
#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Image.h"
#include "Developer/MeshMergeUtilities/Private/MeshMergeUtilities.h"
#include "DrawDebugHelpers.h"
#include "EditorLevelLibrary.h"
#include "EditorUtilitySubsystem.h"
#include "Engine/Canvas.h"
#include "IndexTypes.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshPaintUtilities.h"
#include "VSPMeshPaintRenderItem.h"
#include "UObject/Object.h"

void UVSPMeshPaintUVWindow::SetupUVPreviewMaterial()
{
	if (!IsValid(UVPreviewMaterialPtr) || !IsValid(MeshPaintBrush->VSPPaintProperties->TextureToModify))
		return;

	DragInitialRT_Ptr = MeshPaintBrush->GetDragInitialRT();
	BrushRT_Ptr = MeshPaintBrush->GetBrushRT();
	AccumulatedRT_Ptr = MeshPaintBrush->GetAccumulatedRT();
	UVPreviewMaterialPtr->SetTextureParameterValue(VSPMeshPainterExpressions::BrushMaskParam, AccumulatedRT_Ptr);

	UTextureRenderTarget2D* UVBrushWireframe = MeshPaintBrush->GetUVWireframePreviewRT();
	UVPreviewMaterialPtr->SetTextureParameterValue(
		VSPMeshPainterExpressions::UVBrushWireframePreviewRTParameterName,
		UVBrushWireframe);

	int32 UVPreviewTextureSize = MeshPaintBrush->VSPPaintProperties->UVPreviewTextureSize;
	UTextureRenderTarget2D* UVPreview = MeshPaintBrush->GetUVPreviewRT(UVPreviewTextureSize);
	UVPreviewMaterialPtr->SetTextureParameterValue(VSPMeshPainterExpressions::UVPreviewRTParameterName, UVPreview);

	TextureSizeX = MeshPaintBrush->VSPPaintProperties->TextureToModify->GetSizeX();
	TextureSizeY = MeshPaintBrush->VSPPaintProperties->TextureToModify->GetSizeY();
	float UVPreviewSizeRatio = UVPreviewTextureSize / TextureSizeX;
	UVPreviewMaterialPtr->SetScalarParameterValue(
		VSPMeshPainterExpressions::UV_ResolutionRatioParameterName,
		UVPreviewSizeRatio);

	UTexture* Texture;
	const UMaterialInstanceConstant* InitialMeshMaterial = MeshPaintBrush->GetInitialMeshMaterial();
	InitialMeshMaterial->GetTextureParameterValue(VSPMeshPainterExpressions::MaterialArrayParamName, Texture, true);
	UVPreviewMaterialPtr->SetTextureParameterValue(VSPMeshPainterExpressions::MaterialArrayParamName, Texture);
	InitialMeshMaterial->GetTextureParameterValue(VSPMeshPainterExpressions::NRMXArrayParamName, Texture, true);
	UVPreviewMaterialPtr->SetTextureParameterValue(VSPMeshPainterExpressions::NRMXArrayParamName, Texture);

	UVAccumulateDynamicMaterial = UMaterialInstanceDynamic::Create(
		MeshPaintBrush->VSPPaintProperties->AccumulatorMaterial,
		this,
		TEXT("AccumulatorMaterial"));
	UVAccumulateDynamicMaterial->SetTextureParameterValue(
		VSPMeshPainterExpressions::InitialMaskParam,
		AccumulatedRT_Ptr);
	UVAccumulateDynamicMaterial->SetTextureParameterValue(VSPMeshPainterExpressions::BrushMaskParam, BrushRT_Ptr);
	UVAccumulateDynamicMaterial->SetScalarParameterValue(VSPMeshPainterExpressions::MaskSizeXParam, TextureSizeX);
	UVAccumulateDynamicMaterial->SetScalarParameterValue(
		VSPMeshPainterExpressions::MaskSizeYParam,
		MeshPaintBrush->VSPPaintProperties->TextureToModify->GetSizeY());

	MeshPaintBrush->CommitLayersChanges(UVPreviewMaterialPtr, true);
}

void UVSPMeshPaintUVWindow::FocusUVWindowViaPainterBrushPosition(TArray<FVector2D> UVArray)
{
	if (ActiveDragState != Empty || bIsFocused)
		return;

	FVector2D UVPos = UVArray[0];

	//calc zoom based on triangle size
	float MaxDistance = 0;
	UVArray.Add(FVector2D(UVArray[1]));
	for (int i = 2; i < UVArray.Num(); ++i)
	{
		auto Triangle = UVArray[i - 1];
		auto Triangle2 = UVArray[i];

		const float Distance = FVector2D::Distance(Triangle, Triangle2);
		if (Distance > MaxDistance)
		{
			MaxDistance = Distance;
		}
	}

	ZoomAmount = 1 - (MaxDistance * UVZoomFocusMult);

	SetUVCenterShift(FVector((UVPos - FVector2D(0.5, 0.5)) * 0.5, 0)); // set uv window focus
	UVPos = PanUV(UVPos, 2, 0, true);
	BrushUVPosition = UVPos;
	SetMaterialProperties();
}

void UVSPMeshPaintUVWindow::FocusUVWindowViaPainterBrushPosition(
	const FVector TraceStart,
	const FVector TraceEnd,
	IMeshPaintComponentAdapter* MeshAdapter)
{
	if (ActiveDragState != Empty || bIsFocused)
		return;

	FIndex3i FoundTriangle;
	FVector HitPoint;

	if (MeshAdapter == nullptr || !MeshAdapter->RayIntersectAdapter(FoundTriangle, HitPoint, TraceStart, TraceEnd))
		return;

	FVector2D UV_Vert_1, UV_Vert_2, UV_Vert_3;
	MeshAdapter->GetTextureCoordinate(FoundTriangle.A, 1, UV_Vert_1);
	MeshAdapter->GetTextureCoordinate(FoundTriangle.B, 1, UV_Vert_2);
	MeshAdapter->GetTextureCoordinate(FoundTriangle.C, 1, UV_Vert_3);

	FVector2D UV_Center = (UV_Vert_1 + UV_Vert_2 + UV_Vert_3) / 3;
	TArray<FVector2D> UVArray = { UV_Center, UV_Vert_1, UV_Vert_2, UV_Vert_3 };

	FocusUVWindowViaPainterBrushPosition(UVArray);
}


void UVSPMeshPaintUVWindow::Setup(FName TabIdVal)
{
	this->TabId = TabIdVal;
	SetKeyboardFocus();

	BrushDynamicMaterialInstancePtr = UMaterialInstanceDynamic::Create(BrushMaterialPtr, GetWorld());
	MeshPaintBrush->SetUVWireframeMaterialInstance(UVWireframeMatPtr);

	//setup delegates
	OnLayerUpdateHandle = MeshPaintBrush->VSPPaintProperties->OnLayerUpdate.AddLambda(
		[this]()
		{
			SetupUVPreviewMaterial();
			SetMaterialProperties();
		});
	OnSelectedTileChangedHandle = MeshPaintBrush->VSPPaintProperties->OnSelectedTileChanged.AddLambda(
		[this]()
		{
			SetMaterialProperties();
		});
	OnBrushRadiusChangedHandle = MeshPaintBrush->VSPPaintProperties->OnBrushRadiusChanged.AddLambda(
		[this](float Delta)
		{
			BrushSizeMult += Delta / 100.0f;
			SetMaterialProperties();
		});
	OnBrushStrengthChangedHandle = MeshPaintBrush->VSPPaintProperties->OnBrushStrengthChanged.AddLambda(
		[this](float Delta)
		{
			BrushStrengthModifier = MeshPaintBrush->VSPPaintProperties->BrushStrength;
			SetMaterialProperties();
		});
	MeshPaintBrush->OnDelegateOnBrushMoved.BindUObject(
		this,
		&UVSPMeshPaintUVWindow::FocusUVWindowViaPainterBrushPosition);
	ShutdownLambdaHandle = MeshPaintBrush->VSPPaintProperties->OnShutdownDelegate.AddLambda(
		[this]()
		{
			this->ConditionalBeginDestroy();
		});

	if (IsValid(MainImage))
	{
		UVPreviewMaterialPtr = MainImage->GetDynamicMaterial();
		SetupUVPreviewMaterial();
	}

	SetMaterialProperties();
	MeshPaintBrush->BakeUV_Preview();
}


void UVSPMeshPaintUVWindow::SetMaterialProperties()
{
	if (!IsValid(UVPreviewMaterialPtr) || !IsValid(MeshPaintBrush))
		return;

	ChannelToEdit = MeshPaintBrush->VSPPaintProperties->GetSelectedLayerID();

	UVPreviewMaterialPtr->SetScalarParameterValue(VSPMeshPainterExpressions::ChannelIndexParameterName, ChannelToEdit);
	UVPreviewMaterialPtr->SetVectorParameterValue(VSPMeshPainterExpressions::UV_ShiftParameterName, UVCenterShift);
	UVPreviewMaterialPtr->SetVectorParameterValue(
		VSPMeshPainterExpressions::BrushPositionUVSpaceParameterName,
		FLinearColor(FVector(BrushUVPosition, 0)));
	UVPreviewMaterialPtr->SetScalarParameterValue(VSPMeshPainterExpressions::ZoomParameterName, ZoomAmount);
	UVPreviewMaterialPtr->SetScalarParameterValue(
		VSPMeshPainterExpressions::RawChannelsPreviewParameterName,
		RawChannelsPreview);
	UVPreviewMaterialPtr->SetScalarParameterValue(
		VSPMeshPainterExpressions::BrushRadiusUVSpaceParameterName,
		BrushPreviewInitialSize * BrushSizeMult * 2);
	UVPreviewMaterialPtr->SetScalarParameterValue(
		VSPMeshPainterExpressions::BrushStrengthMultParameterName,
		BrushStrengthModifier);
}

//called from BP
void UVSPMeshPaintUVWindow::On_MaskPreviewMode_SliderValueChanged(float Val)
{
	RawChannelsPreview = Val;
	SetMaterialProperties();
}

//called from BP
void UVSPMeshPaintUVWindow::On_BrushSize_SliderValueChanged(float Val)
{
	BrushSizeMult = FMath::Clamp(Val, 0.01f, 10.0f);
	SetMaterialProperties();
}

void UVSPMeshPaintUVWindow::PaintOnUV(FVector2D UVPosition, int Channel)
{
	if (!IsValid(UVBrushAlphaMask) || !IsValid(BrushDynamicMaterialInstancePtr))
		return;

	UWorld* World = GEditor->GetEditorWorldContext().World();
	UCanvas* BrushPaintCanvas;
	FVector2D Size = FVector2D(BrushRT_Ptr->SizeX, BrushRT_Ptr->SizeY);
	FDrawToRenderTargetContext Context;

	const FVector2D BrushPixelSize = FVector2D(64, 64) * (1 - ZoomAmount) * BrushSizeMult;
	const FVector2D ScreenPosition =
		UVPosition * FVector2D(BrushRT_Ptr->SizeX, BrushRT_Ptr->SizeY) - BrushPixelSize / 2;

	const bool bDrawingAlphaChannel = Channel >= 3;
	const EBlendMode BrushBlendModeLocal =
		bDrawingAlphaChannel ? EBlendMode::BLEND_AlphaComposite : static_cast<EBlendMode>(BrushBlendMode);

	UKismetRenderingLibrary::BeginDrawCanvasToRenderTarget(World, BrushRT_Ptr, BrushPaintCanvas, Size, Context);

	BrushDynamicMaterialInstancePtr->SetVectorParameterValue(
		VSPMeshPainterExpressions::ColorParameterName,
		BrushChannelColors[Channel % 4] * BrushStrengthModifier);
	BrushDynamicMaterialInstancePtr->SetTextureParameterValue(
		VSPMeshPainterExpressions::Texture0ParameterName,
		UVBrushAlphaMask);

	//paint to channel
	if (!bDrawingAlphaChannel)
	{
		BrushPaintCanvas->K2_DrawMaterial(
			BrushDynamicMaterialInstancePtr,
			ScreenPosition,
			BrushPixelSize,
			FVector2D::UnitVector,
			FVector2D::UnitVector);
	}
	else
	{
		//'Canvas->K2_DrawMaterial' cant draw alpha on canvas
		BrushPaintCanvas->K2_DrawTexture(
			UVBrushAlphaMask,
			ScreenPosition,
			BrushPixelSize,
			FVector2D::UnitVector,
			FVector2D::UnitVector,
			FLinearColor::White,
			BrushBlendModeLocal);
	}

	UKismetRenderingLibrary::EndDrawCanvasToRenderTarget(World, Context);

	ApplyBrushRTToAccumulatedRT();
	UMeshPaintUtilities::ClearRT(BrushRT_Ptr);
}

void UVSPMeshPaintUVWindow::ApplyBrushRTToAccumulatedRT()
{
	if (!AccumulatedRT_Ptr || !MeshPaintBrush || !UVAccumulateDynamicMaterial)
		return;

	FCanvas BrushPaintCanvas(
		AccumulatedRT_Ptr->GameThread_GetRenderTargetResource(),
		nullptr,
		0,
		0,
		0,
		VSPMeshPainterExpressions::SupportedFeatureLevel);
	BrushPaintCanvas.SetParentCanvasSize({ TextureSizeX, TextureSizeY });
	BrushPaintCanvas.Clear(FLinearColor::White);

	UVAccumulateDynamicMaterial->SetScalarParameterValue(VSPMeshPainterExpressions::MaskIDParam, ChannelToEdit);
	UVAccumulateDynamicMaterial->SetScalarParameterValue(
		VSPMeshPainterExpressions::SmearApplyModeParam,
		static_cast<float>(MeshPaintBrush->VSPPaintProperties->Function));
	UVAccumulateDynamicMaterial->SetScalarParameterValue(VSPMeshPainterExpressions::UseBrushDilationParam, 0);

	const FMaterialRenderProxy* MaterialRenderProxy = UVAccumulateDynamicMaterial->GetRenderProxy();
	FCanvasTileItem Item(FVector2D(0), MaterialRenderProxy, FVector2D(TextureSizeX, TextureSizeY));
	Item.BlendMode = SE_BLEND_AlphaHoldout;
	BrushPaintCanvas.DrawItem(Item);
	BrushPaintCanvas.Flush_GameThread(true);

	{
		FTextureRenderTargetResource* RenderTargetResource = AccumulatedRT_Ptr->GameThread_GetRenderTargetResource();
		ENQUEUE_RENDER_COMMAND(UpdateMeshPaintRTCommand5)
		(
			[RenderTargetResource](FRHICommandListImmediate& RHICmdList)
			{
				// Copy (resolve) the rendered image from the frame buffer to its render target texture
				RHICmdList.CopyToResolveTarget(
					RenderTargetResource->GetRenderTargetTexture(),
					RenderTargetResource->TextureRHI,
					FResolveParams());
			});
	}
}

void UVSPMeshPaintUVWindow::On_UndoLastSmear_ButtonPressed()
{
	UMeshPaintUtilities::CopyTextureToRT(DragInitialRT_Ptr, AccumulatedRT_Ptr);
	FlushRenderingCommands();
	bPaintModified = false;
}

FVector2D UVSPMeshPaintUVWindow::PanUV(
	FVector2D UV,
	const float StepMult,
	const int UVChunkIndexOverride,
	const bool bReverse) const
{
	const float Step = 0.25 * StepMult;
	const TArray<FVector2D> UVChunkCenters = { FVector2D(1, 1), FVector2D(3, 1), FVector2D(1, 3), FVector2D(3, 3) };

	const float OneMinusZoom = 1.0 - ZoomAmount;
	const int UVChunkIndex = UVChunkIndexOverride < 0 ? ChannelToEdit / 4 : UVChunkIndexOverride;
	const float zoomRange = Step * OneMinusZoom;
	FVector2D UVShiftLocal = FVector2D(UVCenterShift.X, UVCenterShift.Y) * StepMult;

	const FVector2D UVChunkCenter = UVChunkCenters[UVChunkIndex] * Step;
	const float UVCenterX = UVChunkCenter.X;
	const float UVCenterY = UVChunkCenter.Y;

	UVShiftLocal.X =
		FMath::Clamp(UVCenterX + UVShiftLocal.X, UVCenterX - Step + zoomRange, UVCenterX + Step - zoomRange);
	UVShiftLocal.Y =
		FMath::Clamp(UVCenterY + UVShiftLocal.Y, UVCenterY - Step + zoomRange, UVCenterY + Step - zoomRange);


	const FLinearColor UVRange = FLinearColor(
		UVShiftLocal.X - zoomRange,
		UVShiftLocal.X + zoomRange,
		UVShiftLocal.Y - zoomRange,
		UVShiftLocal.Y + zoomRange);

	if (!bReverse)
	{
		UV.X = FMath::GetMappedRangeValueUnclamped(FVector2D(0, 1), FVector2D(UVRange.R, UVRange.G), UV.X);
		UV.Y = FMath::GetMappedRangeValueUnclamped(FVector2D(0, 1), FVector2D(UVRange.B, UVRange.A), UV.Y);
	}
	else
	{
		//reverse
		UV.X = FMath::GetMappedRangeValueUnclamped(FVector2D(UVRange.R, UVRange.G), FVector2D(0, 1), UV.X);
		UV.Y = FMath::GetMappedRangeValueUnclamped(FVector2D(UVRange.B, UVRange.A), FVector2D(0, 1), UV.Y);
	}

	return UV;
}

FInputDeviceRay UVSPMeshPaintUVWindow::DeviceRayFromUvPosition(FVector2D UVPosition) const
{
	FVector OutNormal;
	TArray<FVector> OutTriangle;
	FVector WorldPosition;
	MeshPaintBrush->GetWorldDataFromUVPosition(UVPosition, WorldPosition, OutNormal, OutTriangle);

	const FRay WorldRayIn = FRay(WorldPosition + OutNormal * 10, -OutNormal);
	const FInputDeviceRay InputDeviceRay = FInputDeviceRay(WorldRayIn);
	return InputDeviceRay;
}

void UVSPMeshPaintUVWindow::FakeBrushMovement(FVector2D UVPosition) const
{
	const FInputDeviceRay DeviceRay = DeviceRayFromUvPosition(UVPosition);
	MeshPaintBrush->OnUpdateHover(DeviceRay); //fake scene brush movement
}

bool UVSPMeshPaintUVWindow::FocusEditorCamera() const
{
	FVector OutNormal;
	const FVector2D UVPos = PanUV(FVector2D(BrushUVPosition.X, BrushUVPosition.Y), 2, 0);
	TArray<FVector> OutTriangle;
	FVector WorldPosition;
	MeshPaintBrush->GetWorldDataFromUVPosition(UVPos, WorldPosition, OutNormal, OutTriangle);
	OutNormal = OutNormal.GetSafeNormal();

	if (WorldPosition.Size() <= 1)
		return false;

	//calc distance to camera
	float MaxDistance = 0;
	if (OutTriangle.Num() == 3)
	{
		OutTriangle.Add(FVector(OutTriangle[0]));
		for (int i = 1; i < OutTriangle.Num(); ++i)
		{
			auto Triangle = OutTriangle[i - 1];
			auto Triangle2 = OutTriangle[i];

			DrawDebugLine(
				GetWorld(),
				Triangle,
				Triangle2,
				FColor::Yellow,
				false,
				50,
				ESceneDepthPriorityGroup::SDPG_MAX);

			const float Distance = FVector::Distance(Triangle, Triangle2);
			if (Distance > MaxDistance)
			{
				MaxDistance = Distance;
			}
		}
	}
	else
	{
		MaxDistance = 1000;
	}

	FVector CameraLocation = WorldPosition + OutNormal * MaxDistance;
	FHitResult HitResult;
	if (GetWorld()->LineTraceSingleByChannel(
			HitResult,
			WorldPosition,
			WorldPosition + OutNormal * MaxDistance,
			ECollisionChannel::ECC_WorldStatic))
	{
		CameraLocation = HitResult.Location - OutNormal * 25;
	}
	const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(CameraLocation, WorldPosition);

	UEditorLevelLibrary::SetLevelViewportCameraInfo(CameraLocation, LookAtRotation);

	return true;
}

void UVSPMeshPaintUVWindow::SetUVCenterShift(FVector NewUVPos)
{
	constexpr float UVShiftRange = 0.25;
	NewUVPos.X = FMath::Clamp(NewUVPos.X, -UVShiftRange, UVShiftRange);
	NewUVPos.Y = FMath::Clamp(NewUVPos.Y, -UVShiftRange, UVShiftRange);

	UVCenterShift = NewUVPos;
}


void UVSPMeshPaintUVWindow::BeginDestroy()
{
	if (IsValid(MeshPaintBrush))
	{
		MeshPaintBrush->VSPPaintProperties->OnLayerUpdate.Remove(OnLayerUpdateHandle);
		MeshPaintBrush->VSPPaintProperties->OnSelectedTileChanged.Remove(OnSelectedTileChangedHandle);
		MeshPaintBrush->OnDelegateOnBrushMoved.Unbind();
		MeshPaintBrush->VSPPaintProperties->OnShutdownDelegate.Remove(ShutdownLambdaHandle);
		MeshPaintBrush->VSPPaintProperties->OnBrushStrengthChanged.Remove(OnBrushStrengthChangedHandle);
		MeshPaintBrush->VSPPaintProperties->OnBrushRadiusChanged.Remove(OnBrushRadiusChangedHandle);
		MeshPaintBrush->VSPPaintProperties->OnSmearApplyModeChanged.Remove(OnSmearApplyModeChangedHandle);
	}

	UEditorUtilitySubsystem* EditorUtilitySubsystem = GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
	if (IsValid(EditorUtilitySubsystem))
	{
		EditorUtilitySubsystem->CloseTabByID(this->TabId);
	}

	Super::BeginDestroy();
}


FReply UVSPMeshPaintUVWindow::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (!IsValid(MeshPaintBrush))
		return Super::NativeOnMouseMove(InGeometry, InMouseEvent);

	//brush hotkeys. also discarding drag
	if (ActiveDragState > -1 && InMouseEvent.GetModifierKeys().IsLeftAltDown())
	{
		const FVector2D MouseDelta = InMouseEvent.GetCursorDelta();
		MeshPaintBrush->VSPPaintProperties->HandleMouseShortcut((int)ActiveDragState, MouseDelta);
		return FReply::Handled();
	}

	const FVector2D ScreenSpacePosition = InMouseEvent.GetScreenSpacePosition();
	const FGeometry Geometry = MainImage->GetCachedGeometry();
	BrushUVPosition = Geometry.AbsoluteToLocal(ScreenSpacePosition) / Geometry.GetLocalSize();


	if (ActiveDragState == Empty)
	{
		FakeBrushMovement(PanUV(BrushUVPosition, 2, 0));

		if (InMouseEvent.GetModifierKeys().IsLeftControlDown())
		{
			const FInputDeviceRay DeviceRay = DeviceRayFromUvPosition(PanUV(BrushUVPosition, 2, 0));
			const FVector TraceStart(DeviceRay.WorldRay.Origin);
			const FVector TraceEnd(DeviceRay.WorldRay.Origin + DeviceRay.WorldRay.Direction * HALF_WORLD_MAX);

			MeshPaintBrush->UpdateUVBrush(TraceStart, TraceEnd);
		}
	}

	if (ActiveDragState == Left)
	{
		if (InMouseEvent.GetCursorDelta().Size() > 1.f || bDragFinished == true)
		{
			bDragFinished = false;
			FVector2D UVPosition = PanUV(BrushUVPosition);
			PaintOnUV(UVPosition, ChannelToEdit);
		}
	}

	if (ActiveDragState == Right)
	{
		const FVector2D Delta = InMouseEvent.GetCursorDelta() * UVPanSpeed * (1 - ZoomAmount);

		FVector NewUVPos = UVCenterShift;
		NewUVPos.X = NewUVPos.X + Delta.X;
		NewUVPos.Y = NewUVPos.Y + Delta.Y;

		SetUVCenterShift(NewUVPos);
	}
	SetMaterialProperties();

	return FReply::Handled();
}

FReply UVSPMeshPaintUVWindow::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
	{
		ActiveDragState = Right;
	}
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		ActiveDragState = Left;

		if (bDragFinished)
		{
			bPaintModified = true;
			UMeshPaintUtilities::CopyTextureToRT(AccumulatedRT_Ptr, DragInitialRT_Ptr);
			FlushRenderingCommands();
		}
	}
	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}

FReply UVSPMeshPaintUVWindow::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	ActiveDragState = Empty;

	bDragFinished = true;
	//place editor camera to face triangle selected on UV window
	if (InMouseEvent.GetEffectingButton() == EKeys::MiddleMouseButton)
	{
		if (FocusEditorCamera())
		{
			return FReply::Handled();
		}
	}

	return Super::NativeOnMouseButtonUp(InGeometry, InMouseEvent);
}

void UVSPMeshPaintUVWindow::NativeOnFocusLost(const FFocusEvent& InFocusEvent)
{
	Super::NativeOnFocusLost(InFocusEvent);
	ActiveDragState = Empty;
	bIsFocused = false;
}

void UVSPMeshPaintUVWindow::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
	ActiveDragState = Empty;
	bIsFocused = false;
}

void UVSPMeshPaintUVWindow::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	bIsFocused = true;
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

FReply UVSPMeshPaintUVWindow::NativeOnMouseWheel(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	if (IsValid(MeshPaintBrush))
	{
		ZoomAmount += InMouseEvent.GetWheelDelta() * ZoomSpeed;
		ZoomAmount = FMath::Clamp(ZoomAmount, 0.0f, 1.0f);

		FVector UVCenterFromBrushPosition = FVector(BrushUVPosition - FVector2D(0.5, 0.5), 1);
		UVCenterFromBrushPosition = FMath::Lerp(UVCenterShift, UVCenterFromBrushPosition, 0.05 * (1 - ZoomAmount));
		SetUVCenterShift(UVCenterFromBrushPosition);

		SetMaterialProperties();
	}
	return Super::NativeOnMouseWheel(InGeometry, InMouseEvent);
}
