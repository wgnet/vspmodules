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
#include "MeshPaintingToolset/Public/MeshTexturePaintingTool.h"
#include "VSPToolsProperties.h"
#include "VSPMeshPaintBrush.generated.h"

class UVSPBrushToolProperties;
struct FVSPMPTriangle;
struct FBrushSpreadVariation;

UCLASS()
class VSPMESHPAINTER_API UVSPMeshPaintBrushBuilder : public UInteractiveToolBuilder
{
	GENERATED_BODY()

public:
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual UInteractiveTool* BuildTool(const FToolBuilderState& SceneState) const override;
};


UCLASS()
class VSPMESHPAINTER_API UVSPMeshPaintBrush : public UBaseBrushTool
{
	GENERATED_BODY()

public:
	DECLARE_DELEGATE_ThreeParams(DelegateOnBrushMoved, FVector, FVector, IMeshPaintComponentAdapter*)
	DelegateOnBrushMoved OnDelegateOnBrushMoved;
	FDelegateHandle OnLayerUpdateHandle;
	FDelegateHandle OnUVLevelUpdateHandle;

	UPROPERTY()
	UVSPBrushToolProperties* VSPPaintProperties;

	UVSPMeshPaintBrush();

	void Init(const FToolBuilderState& SceneState);
	void UpdateAccumulatedRT();
	void SetPaintModified(bool Value = true);
	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;

	virtual bool HitTest(const FRay& Ray, FHitResult& OutHit) override;
	virtual void OnBeginDrag(const FRay& Ray) override;
	virtual void OnUpdateDrag(const FRay& Ray) override;
	virtual void OnEndDrag(const FRay& Ray) override;
	void UpdateUVBrush(const FVector TraceStart, const FVector TraceEnd);
	void UpdateUVBrush(const FInputDeviceRay& DevicePos);
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;

	void CommitChanges();

	void UndoLastSmear();

	void ApplyMaskDilation();

	bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event);
	bool InputDelta(
		FEditorViewportClient* InViewportClient,
		FViewport* InViewport,
		FVector& InDrag,
		FRotator& InRot,
		FVector& InScale);

	UMaterialInstanceDynamic* GetBrushMaterial();
	UMaterialInstanceConstant* GetInitialMeshMaterial();
	UMaterialInstanceDynamic* GetAccumulateDynamicMaterial();
	UTextureRenderTarget2D* GetAccumulatedRT();
	UTextureRenderTarget2D* GetDragInitialRT();
	UTextureRenderTarget2D* GetSeamMaskRT();
	UTextureRenderTarget2D* GetUVWireframePreviewRT();
	UTextureRenderTarget2D* GetUVPreviewRT(int32 UVPreviewTextureSize);
	UTextureRenderTarget2D* GetBrushRT();
	UStaticMeshComponent* GetPaintableMeshComp();
	void SetUVWireframeMaterialInstance(UMaterialInstance* MaterialInstance);
	void BakeUV_Preview();
	void GetWorldDataFromUVPosition(
		FVector2D UVPosition,
		FVector& OutWorldPosition,
		FVector& OutNormal,
		TArray<FVector>& OutTriangleVerts) const;

private:
	FToolBuilderState ToolBuilderInitialState {};
	bool bPaintModified = false;
	bool bUpdateUV_WindowBrush = false;

	UPROPERTY()
	UMeshToolManager* MeshToolManager = nullptr;

	UPROPERTY()
	UMeshComponent* PaintableMeshComp = nullptr;

	UPROPERTY()
	UMaterialInstanceConstant* InitialMeshMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* HelpPaintMeshMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* BrushDynamicMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* AccumulateDynamicMaterial = nullptr;

	UPROPERTY()
	UMaterialInstanceDynamic* UnwrapDynamicMaterial = nullptr;

	UPROPERTY()
	UMaterialInstance* UVWireframeMaterialInstancePtr = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* BrushRT = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* AccumulatedRT = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* DragInitialRT = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* SeamMaskRT = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* UVWireframePreviewRT = nullptr;

	UPROPERTY()
	UTextureRenderTarget2D* UVPreviewRT = nullptr;

	UPROPERTY()
	USceneCaptureComponent2D* SceneCaptureComponentPtr;

	FIntPoint TextureSize = FIntPoint::ZeroValue;
	FHitResult CurrentFrameHit;
	FHitResult PreviousFrameHit;
	float DistanceFromLastPaint = 0.f;
	float DistanceToNextPaint = 0.f;

	int32 UnwrapCaptureSize = 1024;
	FVector UnwrapLocation = FVector::ZeroVector;

	float DilationExtrapolationPush = 0.25f;

	TArray<FVector2D> Offsets {
		FVector2D(-1, 0), FVector2D(1, 0), FVector2D(0, 1),	 FVector2D(0, -1),
		FVector2D(-1, 1), FVector2D(1, 1), FVector2D(1, -1), FVector2D(-1, -1)
	};

	void GenerateSeamMask();

public:
	void CalculateBrushSpreadAndPaint(const FHitResult& Hit);
	void ApplyBrushRTToAccumulatedRT();

private:
	//void Paint(const FHitResult& Hit, const FBrushSpreadVariation& BrushSpreadVariation, UTextureRenderTarget2D* TargetRT, UMaterialInstance* MaterialInstance, bool bApplyToAccumulatedRT = true);
	TArray<FVSPMPTriangle> GenerateTrianglesForPaint(
		const FHitResult& Hit,
		const FBrushSpreadVariation& BrushSpreadVariation);
	TArray<FVSPMPTriangle> GenerateTrianglesForPaint(
		const FVector2D UVPoint,
		const FBrushSpreadVariation& BrushSpreadVariation);
	void PaintToCanvas(
		const UMaterialInstance* MaterialInstance,
		UTextureRenderTarget2D* TargetRT,
		TArray<FVSPMPTriangle> Triangles) const;
	void GetInfluencedTriangles(
		IMeshPaintComponentAdapter* Adapter,
		const FIntPoint& CanvasSize,
		TArray<FVSPMPTriangle>& OutTriangles,
		const int32 UVChannel,
		TArray<uint32> InfluencedTriangles) const;
	void BindBrushParams(IMeshPaintComponentAdapter* Adapter, const FSmearProperties& SmearProperties);
	void BuildTrianglesForPaint(
		IMeshPaintComponentAdapter* Adapter,
		const FVector& CameraPosition,
		const FSmearProperties& SmearProperties,
		const FIntPoint& CanvasSize,
		bool bUseOnlyCentral,
		TArray<FVSPMPTriangle>&) const;


	bool IsUsingBrushAlphaMask() const;

	void SetupTextureIdxToLayers();

	void BuildDynamicMaterials();
	void DestroyDynamicMaterials();
	void BuildRenderTargets();
	void DestroyRenderTargets();
	void CommitTextureChanges();

public:
	void CommitLayersChanges(UMaterialInstanceConstant* MaterialInstance) const;
	void CommitLayersChanges(UMaterialInstanceDynamic* MaterialInstance, bool bUVWindowMaterial) const;
};
