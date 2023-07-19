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
#include "VSPMeshPaintBrush.h"
#include "BaseMeshPaintComponentAdapter.h"
#include "CanvasItem.h"
#include "Components/SceneCaptureComponent2D.h"
#include "EditorLevelLibrary.h"
#include "InteractiveToolManager.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MeshDescription.h"
#include "MeshPaintUtilities.h"
#include "MeshPaintingToolset/Public/MeshTexturePaintingTool.h"
#include "VSPCheck.h"
#include "VSPMeshPaintMode.h"
#include "VSPMeshPaintRenderItem.h"
#include "VSPMeshPainterSettings.h"
#include "VSPToolsProperties.h"
#include "Rendering/Texture2DResource.h"
#include "ToolDataVisualizer.h"

using namespace VSPMeshPainterExpressions;

bool UVSPMeshPaintBrushBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return UVSPMeshPainterSettings::Get()->IsSupportToPaint(
		UMeshPaintUtilities::GetSingleSelectedStaticMeshComp(SceneState));
}

UInteractiveTool* UVSPMeshPaintBrushBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UVSPMeshPaintBrush* NewTool = NewObject<UVSPMeshPaintBrush>(SceneState.ToolManager);
	NewTool->Init(SceneState);
	return NewTool;
}

void UVSPMeshPaintBrush::UpdateAccumulatedRT()
{
	UMeshPaintUtilities::CopyTextureToRT(AccumulatedRT, DragInitialRT);
	FlushRenderingCommands();

	ApplyBrushRTToAccumulatedRT();
	UMeshPaintUtilities::ClearRT(BrushRT);
}

void UVSPMeshPaintBrush::SetPaintModified(bool Value)
{
	bPaintModified = Value;
}

void UVSPMeshPaintBrush::Setup()
{
	Super::Setup();

	MeshToolManager = Cast<UMeshToolManager>(GetToolManager());
	VSPCheck(MeshToolManager);
	MeshToolManager->ResetState();
	if (IsValid(ToolBuilderInitialState.SelectedActors[0]))
		MeshToolManager->AddSelectedMeshComponents(
			{ ToolBuilderInitialState.SelectedActors[0]->FindComponentByClass<UStaticMeshComponent>() });
	MeshToolManager->CacheSelectionData(0, 0);

	PaintableMeshComp = UMeshPaintUtilities::GetSingleSelectedStaticMeshComp(ToolBuilderInitialState);

	VSPPaintProperties = Cast<UVSPBrushToolProperties>(BrushProperties);
	VSPCheck(BrushProperties);
	VSPPaintProperties->SetupFor(PaintableMeshComp);

	UTexture2D* TextureToModify = VSPPaintProperties->TextureToModify;
	TextureSize = FIntPoint(TextureToModify->GetSizeX(), TextureToModify->GetSizeY());

	BuildRenderTargets();

	UMeshPaintUtilities::CopyTextureToRT(TextureToModify, AccumulatedRT);
	FlushRenderingCommands();

	BuildDynamicMaterials();

	bPaintModified = false;
	OnLayerUpdateHandle = VSPPaintProperties->OnLayerUpdate.AddLambda(
		[this]()
		{
			SetupTextureIdxToLayers();
		});
}

void UVSPMeshPaintBrush::Shutdown(EToolShutdownType ShutdownType)
{
	FlushRenderingCommands();

	if (bPaintModified)
	{
		const EAppReturnType::Type Response = FMessageDialog::Open(
			EAppMsgType::YesNo,
			FText::FromString(TEXT("MATERIAL PAINTER: Commit paint changes before exit?")));

		if (Response == EAppReturnType::Type::Yes)
			CommitChanges();
	}

	DestroyDynamicMaterials();
	DestroyRenderTargets();

	if (OnLayerUpdateHandle.IsValid())
		VSPPaintProperties->OnLayerUpdate.Remove(OnLayerUpdateHandle);

	if (OnUVLevelUpdateHandle.IsValid())
		VSPPaintProperties->OnUVLevelUpdate.Remove(OnUVLevelUpdateHandle);

	if (VSPPaintProperties->OnShutdownDelegate.IsBound())
		VSPPaintProperties->OnShutdownDelegate.Broadcast();

	Super::Shutdown(ShutdownType);
}

UVSPMeshPaintBrush::UVSPMeshPaintBrush()
{
	PropertyClass = UVSPBrushToolProperties::StaticClass();
}

void UVSPMeshPaintBrush::Init(const FToolBuilderState& SceneState)
{
	ToolBuilderInitialState = SceneState;
}

bool UVSPMeshPaintBrush::HitTest(const FRay& Ray, FHitResult& OutHit)
{
	PreviousFrameHit = CurrentFrameHit;
	MeshToolManager->FindHitResult(Ray, OutHit);
	CurrentFrameHit = OutHit;

	return OutHit.bBlockingHit;
}

void UVSPMeshPaintBrush::OnBeginDrag(const FRay& Ray)
{
	Super::OnBeginDrag(Ray);

	UMeshPaintUtilities::CopyTextureToRT(AccumulatedRT, DragInitialRT);
	FlushRenderingCommands();

	FHitResult BestTraceResult;
	MeshToolManager->FindHitResult(Ray, BestTraceResult);

	CalculateBrushSpreadAndPaint(BestTraceResult);
	DistanceFromLastPaint = 0.f;
}

void UVSPMeshPaintBrush::OnUpdateDrag(const FRay& Ray)
{
	Super::OnUpdateDrag(Ray);

	FHitResult BestTraceResult;
	MeshToolManager->FindHitResult(Ray, BestTraceResult);

	DistanceFromLastPaint += FVector::Distance(CurrentFrameHit.Location, PreviousFrameHit.Location);

	if (DistanceFromLastPaint >= DistanceToNextPaint)
	{
		CalculateBrushSpreadAndPaint(BestTraceResult);
		DistanceFromLastPaint = 0.f;
	}
}

void UVSPMeshPaintBrush::OnEndDrag(const FRay& Ray)
{
	Super::OnEndDrag(Ray);

	ApplyBrushRTToAccumulatedRT();
	UMeshPaintUtilities::ClearRT(BrushRT);

	FlushRenderingCommands();
}

inline bool UVSPMeshPaintBrush::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	if (bUpdateUV_WindowBrush)
	{
		UpdateUVBrush(DevicePos);
	}
	return Super::OnUpdateHover(DevicePos);
}


void UVSPMeshPaintBrush::UpdateUVBrush(const FInputDeviceRay& DevicePos)
{
	const FVector TraceStart(DevicePos.WorldRay.Origin);
	const FVector TraceEnd(DevicePos.WorldRay.Origin + DevicePos.WorldRay.Direction * HALF_WORLD_MAX);
	UpdateUVBrush(TraceStart, TraceEnd);
}

void UVSPMeshPaintBrush::UpdateUVBrush(const FVector TraceStart, const FVector TraceEnd)
{
	if (!IsValid(PaintableMeshComp) || !IsValid(MeshToolManager))
		return;

	IMeshPaintComponentAdapter* MeshAdapter = MeshToolManager->GetAdapterForComponent(PaintableMeshComp).Get();
	VSPCheckReturn(MeshAdapter);

	FHitResult TraceHitResult(1.0f);

	if (MeshAdapter == nullptr || !IsValid(UVWireframeMaterialInstancePtr)
		|| !MeshAdapter->LineTraceComponent(
			TraceHitResult,
			TraceStart,
			TraceEnd,
			FCollisionQueryParams(SCENE_QUERY_STAT(Paint), true)))
		return;

	FHitResult BestTraceResult;
	BestTraceResult = TraceHitResult;

	//begin paint of wireframe preview to canvas
	const FBrushSpreadVariation BrushSpreadVariation(VSPPaintProperties);
	TArray<FVSPMPTriangle> Triangles = GenerateTrianglesForPaint(BestTraceResult, BrushSpreadVariation);
	UKismetRenderingLibrary::ClearRenderTarget2D(PaintableMeshComp, GetUVWireframePreviewRT(), FLinearColor::Black);
	PaintToCanvas(UVWireframeMaterialInstancePtr, GetUVWireframePreviewRT(), Triangles);
	//end paint to canvas

	//call delegate
	FTransform ComponentTransform = PaintableMeshComp->GetComponentTransform();
	OnDelegateOnBrushMoved.ExecuteIfBound(
		ComponentTransform.InverseTransformPosition(TraceStart),
		ComponentTransform.InverseTransformPosition(TraceEnd),
		MeshAdapter);
}

void UVSPMeshPaintBrush::CalculateBrushSpreadAndPaint(const FHitResult& Hit)
{
	const FBrushSpreadVariation BrushSpreadVariation(VSPPaintProperties);
	const TArray<FVSPMPTriangle> Fr3MPTriangles = GenerateTrianglesForPaint(Hit, BrushSpreadVariation);

	PaintToCanvas(BrushDynamicMaterial, BrushRT, Fr3MPTriangles);
	FlushRenderingCommands();
	ApplyBrushRTToAccumulatedRT();
	bPaintModified = true;

	DistanceToNextPaint = BrushSpreadVariation.NextPaintDistance;
}

void UVSPMeshPaintBrush::PaintToCanvas(
	const UMaterialInstance* MaterialInstance,
	UTextureRenderTarget2D* TargetRT,
	TArray<FVSPMPTriangle> Triangles) const
{
	FCanvas BrushPaintCanvas(TargetRT->GameThread_GetRenderTargetResource(), nullptr, 0, 0, 0, SupportedFeatureLevel);
	BrushPaintCanvas.SetParentCanvasSize({ TargetRT->SizeX, TargetRT->SizeY });

	FVSPMPRenderItem* TriangleItem = new FVSPMPRenderItem(MaterialInstance->GetRenderProxy());
	TriangleItem->AddTriangles(MoveTemp(Triangles));
	TriangleItem->Draw(BrushPaintCanvas);
	//Triangle Item gets cleared automatically after this command
	BrushPaintCanvas.Flush_GameThread(true);

	{
		ENQUEUE_RENDER_COMMAND(UpdateMeshPaintRTCommand1)
		(
			[this, TargetRT](FRHICommandListImmediate& RHICmdList)
			{
				const FTextureRenderTargetResource* RenderTargetResource = TargetRT->GetRenderTargetResource();
				RHICmdList.CopyToResolveTarget(
					RenderTargetResource->GetRenderTargetTexture(),
					RenderTargetResource->TextureRHI,
					FResolveParams());
			});
	}

	FlushRenderingCommands();
}

TArray<FVSPMPTriangle> UVSPMeshPaintBrush::GenerateTrianglesForPaint(
	const FHitResult& Hit,
	const FBrushSpreadVariation& BrushSpreadVariation)
{
	TArray<FVSPMPTriangle> Triangles;

	if (Cast<UMeshComponent>(Hit.GetComponent()) == nullptr)
		return Triangles;

	auto HoveredComponent = CastChecked<UMeshComponent>(Hit.GetComponent());
	IMeshPaintComponentAdapter* MeshAdapter = MeshToolManager->GetAdapterForComponent(HoveredComponent).Get();

	if (!MeshAdapter || !MeshAdapter->SupportsTexturePaint())
		return Triangles;

	FViewCameraState CameraState;
	GetToolManager()->GetContextQueriesAPI()->GetCurrentViewState(CameraState);

	FSmearProperties SmearProperties = BrushSpreadVariation.GetSmearProperties(Hit);

	BuildTrianglesForPaint(MeshAdapter, CameraState.Position, SmearProperties, TextureSize, bShiftToggle, Triangles);

	if (!Triangles.Num())
		return Triangles;

	BindBrushParams(MeshAdapter, SmearProperties);

	return Triangles;
}

TArray<FVSPMPTriangle> UVSPMeshPaintBrush::GenerateTrianglesForPaint(
	const FVector2D UVPoint,
	const FBrushSpreadVariation& BrushSpreadVariation)
{
	TArray<FVSPMPTriangle> Triangles;
	IMeshPaintComponentAdapter* MeshAdapter = MeshToolManager->GetAdapterForComponent(PaintableMeshComp).Get();

	if (!MeshAdapter || !MeshAdapter->SupportsTexturePaint())
		return Triangles;

	FViewCameraState CameraState;
	GetToolManager()->GetContextQueriesAPI()->GetCurrentViewState(CameraState);


	FVector Normal;
	TArray<FVector> OutTriangleVerts;
	FVector WorldPosition;
	GetWorldDataFromUVPosition(UVPoint, WorldPosition, Normal, OutTriangleVerts);
	FHitResult HitResult = FHitResult();
	HitResult.Location = WorldPosition;
	HitResult.Normal = Normal;
	FSmearProperties SmearProperties = BrushSpreadVariation.GetSmearProperties(HitResult);

	BuildTrianglesForPaint(MeshAdapter, CameraState.Position, SmearProperties, TextureSize, bShiftToggle, Triangles);

	if (!Triangles.Num())
		return Triangles;

	BindBrushParams(MeshAdapter, SmearProperties);

	return Triangles;
}

void UVSPMeshPaintBrush::ApplyMaskDilation()
{
	if (PaintableMeshComp == nullptr)
		return;

	GenerateSeamMask();

#if WITH_EDITOR
	IMeshPaintComponentAdapter* MeshAdapter = MeshToolManager->GetAdapterForComponent(PaintableMeshComp).Get();

	if (MeshAdapter && MeshAdapter->SupportsTexturePaint())
	{
		int32 SizeX = AccumulatedRT->SizeX;
		int32 SizeY = AccumulatedRT->SizeY;
		int32 DilationSteps;
		switch (FMath::Min(SizeY, SizeX))
		{
		case 256:
			DilationSteps = 2;
			break;
		case 512:
			DilationSteps = 4;
			break;
		case 1024:
			DilationSteps = 8;
			break;
		case 2048:
			DilationSteps = 16;
			break;
		case 4096:
			DilationSteps = 32;
			break;
		default:
			DilationSteps = 1;
			break;
		}

		//TODO: Add support for exclude area auto-adjustment in houdini
		FBox2D ExcludeArea = VSPPaintProperties->TrimMateriansNumber > 0
			? FBox2D(
				FVector2D(FMath::FloorToInt(SizeX * 0.01486f), SizeY * 0.0277f),
				FVector2D(
					FMath::CeilToInt(SizeX * (0.01486f + 0.02546f + 0.03329f * VSPPaintProperties->TrimMateriansNumber)),
					0.f))
			: FBox2D(FVector2D(-1, -1), (FVector2D(-1, -1)));

		FRenderTarget* SeamRenderTarget = SeamMaskRT->GameThread_GetRenderTargetResource();
		TArray<FColor> SeamPixels;
		VSPCheck(SeamRenderTarget);
		SeamRenderTarget->ReadPixels(SeamPixels);

		FRenderTarget* RenderTarget = AccumulatedRT->GameThread_GetRenderTargetResource();
		TArray<FColor> MaskPixels;
		VSPCheck(RenderTarget);
		RenderTarget->ReadPixels(MaskPixels);

		//Generate dilated data
		TArray<FColor> ResultingValues;
		ResultingValues.AddZeroed(SizeX * SizeY);

		auto CheckSeamInCoordinates = [this, SeamPixels, SizeX, SizeY](int32 U, int32 V) -> float
		{
			float SeamValueThreshold = 0.99;
			int32 MaskMultiplier = VSPPaintProperties->bUseUpscaledDilation ? 2 : 1;
			float Average = 0;
			int32 StartU = (U % (SizeX / 2)) * 2 * MaskMultiplier;
			int32 StartV = (V % (SizeY / 2)) * 2 * MaskMultiplier;
			for (int32 SeamU = StartU; SeamU < StartU + MaskMultiplier; SeamU++)
				for (int32 SeamV = StartV; SeamV < StartV + MaskMultiplier; SeamV++)
				{
					FColor SeamSample = SeamPixels[SeamU + (SeamV * SizeX * MaskMultiplier)];
					Average += SeamSample.R;
				}
			Average = Average / (MaskMultiplier * MaskMultiplier);

			return Average > SeamValueThreshold ? true : false;
		};

		for (int32 V = 0; V < SizeY; ++V)
		{
			for (int32 U = 0; U < SizeX; ++U)
			{
				FColor& MaskColor = MaskPixels[(U + (V * SizeX))];

				FColor Result(0, 0, 0, 0);
				//If pixel inside UV island or exclude area
				if (CheckSeamInCoordinates(U, V) || ExcludeArea.IsInside(FVector2D(U, V)))
				{
					Result = FColor(MaskColor.B, MaskColor.G, MaskColor.R, MaskColor.A);
				}
				//if pixel is outside
				else
				{
					FColor CurMinColor(0, 0, 0, 0);

					float MinDist = 100000;
					int32 DilationStep = 0;
					while (DilationStep < DilationSteps)
					{
						DilationStep++;
						int32 OffsetIndex = 0;
						while (OffsetIndex < Offsets.Num())
						{
							int32 OffsetU = U + Offsets[OffsetIndex].X * DilationStep;
							int32 OffsetV = V + Offsets[OffsetIndex].Y * DilationStep;
							if (OffsetU >= 0 && OffsetU < SizeX && OffsetV >= 0 && OffsetV < SizeY)
							{
								if (CheckSeamInCoordinates(OffsetU, OffsetV))
								{
									FColor OffsetColor = MaskPixels[OffsetU + (OffsetV * SizeX)];
									float CurDist = FVector2D::Distance(FVector2D(U, V), FVector2D(OffsetU, OffsetV));
									if (CurDist < MinDist && CurDist)
									{
										MinDist = CurDist;

										int32 DirectionU = FMath::Clamp(
											FMath::FloorToInt(
												OffsetU
												+ Offsets[OffsetIndex].X * DilationStep * DilationExtrapolationPush),
											0,
											SizeX - 1);
										int32 DirectionV = FMath::Clamp(
											FMath::FloorToInt(
												OffsetV
												+ Offsets[OffsetIndex].Y * DilationStep * DilationExtrapolationPush),
											0,
											SizeY - 1);


										if (DirectionU >= 0 && DirectionU < SizeX && DirectionV >= 0
											&& DirectionV < SizeY)
										{
											if (CheckSeamInCoordinates(DirectionU, DirectionV))
											{

												FColor DirectionColor = MaskPixels[DirectionU + (DirectionV * SizeX)];

												FColor Delta(
													OffsetColor.R - DirectionColor.R,
													OffsetColor.G - DirectionColor.G,
													OffsetColor.B - DirectionColor.B,
													OffsetColor.A - DirectionColor.A);

												CurMinColor = FColor(
													OffsetColor.R + Delta.R / DilationExtrapolationPush,
													OffsetColor.G + Delta.G / DilationExtrapolationPush,
													OffsetColor.B + Delta.B / DilationExtrapolationPush,
													OffsetColor.A + Delta.A / DilationExtrapolationPush);
											}
											else
											{
												CurMinColor = OffsetColor;
											}
										}
										else
										{
											CurMinColor = OffsetColor;
										}
									}
								}
							}

							OffsetIndex++;
						}
					}
					Result = FColor(CurMinColor.B, CurMinColor.G, CurMinColor.R, CurMinColor.A);
				}

				ResultingValues[FMath::FloorToInt(U + (V * SizeX))] = Result;
			}
		}
		//Fill new texture with data
		UTexture2D* DilatedTexture = UTexture2D::CreateTransient(SizeX, SizeY, PF_R8G8B8A8);
		DilatedTexture->SRGB = false;

		FColor* NewMipData =
			reinterpret_cast<FColor*>(DilatedTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

		for (int32 V = 0; V < SizeY; ++V)
		{
			for (int32 U = 0; U < SizeX; ++U)
			{
				NewMipData[FMath::FloorToInt(U + (V * SizeX))] = ResultingValues[FMath::FloorToInt(U + (V * SizeX))];
			}
		}

		DilatedTexture->PlatformData->Mips[0].BulkData.Unlock();
		DilatedTexture->UpdateResource();


		UMeshPaintUtilities::CopyTextureToRT(DilatedTexture, AccumulatedRT);

		DilatedTexture->ConditionalBeginDestroy();
	}

	bPaintModified = true;
#endif
}

void UVSPMeshPaintBrush::BindBrushParams(IMeshPaintComponentAdapter* Adapter, const FSmearProperties& SmearProperties)
{
	const FMatrix ComponentToWorldMatrix = Adapter->GetComponentToWorldMatrix();
	const FVector BrushLocation = ComponentToWorldMatrix.InverseTransformPosition(SmearProperties.Location);
	const FVector BrushNormal = ComponentToWorldMatrix.InverseTransformPosition(SmearProperties.Normal);
	const float Radius =
		ComponentToWorldMatrix.InverseTransformVector(FVector(SmearProperties.Size, 0.0f, 0.0f)).Size();

	int32 SelectedTileMaskID = VSPPaintProperties->GetSelectedLayerID();

	BrushDynamicMaterial->SetVectorParameterValue(BrushLocationParam, BrushLocation);
	BrushDynamicMaterial->SetScalarParameterValue(BrushRadiusParam, SmearProperties.Size);
	BrushDynamicMaterial->SetScalarParameterValue(BrushFalloffParam, BrushProperties->BrushFalloffAmount);
	BrushDynamicMaterial->SetScalarParameterValue(BrushStrengthParam, BrushProperties->BrushStrength);
	BrushDynamicMaterial->SetScalarParameterValue(MaskIDParam, SelectedTileMaskID);
	BrushDynamicMaterial->SetScalarParameterValue(BrushAlphaMaskRotationParam, SmearProperties.Rotation);
	BrushDynamicMaterial->SetVectorParameterValue(BrushNormalParam, BrushNormal);
	BrushDynamicMaterial->SetScalarParameterValue(BUseBrushAlphaMaskParam, IsUsingBrushAlphaMask());
	BrushDynamicMaterial->SetTextureParameterValue(BrushAlphaMaskParam, VSPPaintProperties->BrushAlphaMask);
}

void UVSPMeshPaintBrush::GetInfluencedTriangles(
	IMeshPaintComponentAdapter* Adapter,
	const FIntPoint& CanvasSize,
	TArray<FVSPMPTriangle>& OutTriangles,
	const int32 UVChannel,
	TArray<uint32> InfluencedTriangles) const
{
	int32 TriangleVertexIndices[3];
	const TArray<uint32> VertexIndices = Adapter->GetMeshIndices();
	FBox2D UVRect { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
	for (uint32 TriangleIndex : InfluencedTriangles)
	{
		for (int32 TriVertexNum = 0; TriVertexNum < 3; ++TriVertexNum)
		{
			uint32 I = TriangleIndex * 3 + TriVertexNum;
			if ((uint32)VertexIndices.Num() > I)
				TriangleVertexIndices[TriVertexNum] = VertexIndices[I];
		}

		FVSPMPTriangle OutTriangle;

		FVector Pos0;
		FVector Pos1;
		FVector Pos2;

		FVector2D UV0;
		FVector2D UV1;
		FVector2D UV2;


		Adapter->GetVertexPosition(TriangleVertexIndices[0], Pos0);
		Adapter->GetVertexPosition(TriangleVertexIndices[1], Pos1);
		Adapter->GetVertexPosition(TriangleVertexIndices[2], Pos2);

		FVector Normal = FVector::CrossProduct(Pos2 - Pos0, Pos1 - Pos0).GetSafeNormal();

		Adapter->GetTextureCoordinate(TriangleVertexIndices[0], UVChannel, UV0);
		Adapter->GetTextureCoordinate(TriangleVertexIndices[1], UVChannel, UV1);
		Adapter->GetTextureCoordinate(TriangleVertexIndices[2], UVChannel, UV2);

		//check if verts is inside UV rect
		if (!UVRect.IsInside(UV0))
			continue;

		OutTriangle.Vertex0.Position = FVector(UV0 * CanvasSize, 0);
		OutTriangle.Vertex1.Position = FVector(UV1 * CanvasSize, 0);
		OutTriangle.Vertex2.Position = FVector(UV2 * CanvasSize, 0);

		OutTriangle.Vertex0.SetTangents(FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector);
		OutTriangle.Vertex1.SetTangents(FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector);
		OutTriangle.Vertex2.SetTangents(FVector::XAxisVector, FVector::YAxisVector, FVector::ZAxisVector);

		OutTriangle.Vertex0.TextureCoordinate[0] = UV0;
		OutTriangle.Vertex0.TextureCoordinate[1] = FVector2D(Pos0.X, Normal.X);
		OutTriangle.Vertex0.TextureCoordinate[2] = FVector2D(Pos0.Y, Normal.Y);
		OutTriangle.Vertex0.TextureCoordinate[3] = FVector2D(Pos0.Z, Normal.Z);

		OutTriangle.Vertex1.TextureCoordinate[0] = UV1;
		OutTriangle.Vertex1.TextureCoordinate[1] = FVector2D(Pos1.X, Normal.X);
		OutTriangle.Vertex1.TextureCoordinate[2] = FVector2D(Pos1.Y, Normal.Y);
		OutTriangle.Vertex1.TextureCoordinate[3] = FVector2D(Pos1.Z, Normal.Z);

		OutTriangle.Vertex2.TextureCoordinate[0] = UV2;
		OutTriangle.Vertex2.TextureCoordinate[1] = FVector2D(Pos2.X, Normal.X);
		OutTriangle.Vertex2.TextureCoordinate[2] = FVector2D(Pos2.Y, Normal.Y);
		OutTriangle.Vertex2.TextureCoordinate[3] = FVector2D(Pos2.Z, Normal.Z);

		OutTriangles.Add(OutTriangle);
	}
}

void UVSPMeshPaintBrush::BuildTrianglesForPaint(
	IMeshPaintComponentAdapter* Adapter,
	const FVector& CameraPosition,
	const FSmearProperties& SmearProperties,
	const FIntPoint& CanvasSize,
	bool bUseOnlyCentral,
	TArray<FVSPMPTriangle>& OutTriangles) const
{
	const int32 UVChannel = VSPPaintProperties->UVChannel;
	const FMatrix& ComponentToWorldMatrix = Adapter->GetComponentToWorldMatrix();

	const FVector ComponentSpaceCameraPosition(ComponentToWorldMatrix.InverseTransformPosition(CameraPosition));
	const FVector ComponentSpaceBrushPosition(
		ComponentToWorldMatrix.InverseTransformPosition(SmearProperties.Location));

	const float BrushRadius = bUseOnlyCentral
		? 0.001f
		: (IsUsingBrushAlphaMask() ? 2 * SmearProperties.Size / FMath::Sqrt(2) : SmearProperties.Size);
	const float ComponentSpaceBrushRadius =
		ComponentToWorldMatrix.InverseTransformVector(FVector(BrushRadius, 0.0f, 0.0f)).Size();
	const float ComponentSpaceSquaredBrushRadius = ComponentSpaceBrushRadius * ComponentSpaceBrushRadius;

	TArray<uint32> InfluencedTriangles = Adapter->SphereIntersectTriangles(
		ComponentSpaceSquaredBrushRadius,
		ComponentSpaceBrushPosition,
		ComponentSpaceCameraPosition,
		true);

	GetInfluencedTriangles(Adapter, CanvasSize, OutTriangles, UVChannel, InfluencedTriangles);
}

void UVSPMeshPaintBrush::CommitChanges()
{
	CommitTextureChanges();
	CommitLayersChanges(InitialMeshMaterial);
}

void UVSPMeshPaintBrush::UndoLastSmear()
{
	UMeshPaintUtilities::CopyTextureToRT(DragInitialRT, AccumulatedRT);
	FlushRenderingCommands();
}

bool UVSPMeshPaintBrush::InputKey(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	FKey Key,
	EInputEvent Event)
{
	if (Key == EKeys::LeftControl)
	{
		if (Event == IE_Pressed)
		{
			bUpdateUV_WindowBrush = true;
		}
		if (Event == IE_Released)
		{
			bUpdateUV_WindowBrush = false;
		}
	}

	return VSPPaintProperties->InputKey(ViewportClient, Viewport, Key, Event);
}


bool UVSPMeshPaintBrush::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale)
{
	return VSPPaintProperties->InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale);
}



UMaterialInstanceDynamic* UVSPMeshPaintBrush::GetBrushMaterial()
{
	return BrushDynamicMaterial;
}

UMaterialInstanceConstant* UVSPMeshPaintBrush::GetInitialMeshMaterial()
{
	return InitialMeshMaterial;
}

UMaterialInstanceDynamic* UVSPMeshPaintBrush::GetAccumulateDynamicMaterial()
{
	return AccumulateDynamicMaterial;
}

UTextureRenderTarget2D* UVSPMeshPaintBrush::GetAccumulatedRT()
{
	return AccumulatedRT;
}

UTextureRenderTarget2D* UVSPMeshPaintBrush::GetDragInitialRT()
{
	return DragInitialRT;
}

UTextureRenderTarget2D* UVSPMeshPaintBrush::GetSeamMaskRT()
{
	GenerateSeamMask();
	return SeamMaskRT;
}

UTextureRenderTarget2D* UVSPMeshPaintBrush::GetUVWireframePreviewRT()
{
	if (UVWireframePreviewRT == nullptr)
	{
		UVWireframePreviewRT =
			UMeshPaintUtilities::CreateInternalRenderTarget(FIntPoint(4096, 4096), TA_Clamp, TA_Clamp);
		UVWireframePreviewRT->Filter = TextureFilter::TF_Nearest;
	}
	return UVWireframePreviewRT;
}

UTextureRenderTarget2D* UVSPMeshPaintBrush::GetUVPreviewRT(int32 UVPreviewTextureSize)
{
	if (UVPreviewRT == nullptr)
	{
		UVPreviewRT = UMeshPaintUtilities::CreateInternalRenderTarget(
			FIntPoint(UVPreviewTextureSize, UVPreviewTextureSize),
			TA_Clamp,
			TA_Clamp);
		UVPreviewRT->Filter = TextureFilter::TF_Nearest;
	}
	return UVPreviewRT;
}

UTextureRenderTarget2D* UVSPMeshPaintBrush::GetBrushRT()
{
	return BrushRT;
}

UStaticMeshComponent* UVSPMeshPaintBrush::GetPaintableMeshComp()
{
	return Cast<UStaticMeshComponent>(PaintableMeshComp);
}


void UVSPMeshPaintBrush::SetUVWireframeMaterialInstance(UMaterialInstance* MaterialInstance)
{
	UVWireframeMaterialInstancePtr = MaterialInstance;
}

void UVSPMeshPaintBrush::BakeUV_Preview()
{
	VSPCheckReturn(MeshToolManager || PaintableMeshComp);

	IMeshPaintComponentAdapter* MeshAdapter = MeshToolManager->GetAdapterForComponent(PaintableMeshComp).Get();
	VSPCheckReturn(MeshAdapter);

	TArray<uint32> InfluencedTriangles = MeshAdapter->GetMeshIndices();
	TArray<FVSPMPTriangle> Triangles;
	GetInfluencedTriangles(MeshAdapter, TextureSize, Triangles, 1, InfluencedTriangles);

	PaintToCanvas(UVWireframeMaterialInstancePtr, GetUVPreviewRT(VSPPaintProperties->UVPreviewTextureSize), Triangles);
}


void UVSPMeshPaintBrush::GetWorldDataFromUVPosition(
	FVector2D UVPosition,
	FVector& OutWorldPosition,
	FVector& Normal,
	TArray<FVector>& OutTriangleVerts) const
{
	VSPCheckReturn(MeshToolManager);

	IMeshPaintComponentAdapter* Adapter = MeshToolManager->GetAdapterForComponent(PaintableMeshComp).Get();
	VSPCheckReturn(Adapter);

	TArray<uint32> Indices = Adapter->GetMeshIndices();
	Normal = FVector::UpVector;
	const int ChannelIndex = 1;
	OutWorldPosition = FVector::ZeroVector;

	int32 TriangleVertexIndices[3];
	for (uint32 TriangleIndex : Indices)
	{
		for (int32 TriVertexNum = 0; TriVertexNum < 3; ++TriVertexNum)
		{
			uint32 I = TriangleIndex * 3 + TriVertexNum;
			if ((uint32)Indices.Num() > I)
				TriangleVertexIndices[TriVertexNum] = Indices[I];
		}

		FVector2D UV0;
		FVector2D UV1;
		FVector2D UV2;

		Adapter->GetTextureCoordinate(TriangleVertexIndices[0], ChannelIndex, UV0);
		Adapter->GetTextureCoordinate(TriangleVertexIndices[1], ChannelIndex, UV1);
		Adapter->GetTextureCoordinate(TriangleVertexIndices[2], ChannelIndex, UV2);

		const FBox2D UVRect { { 0.0f, 0.0f }, { 1.0f, 1.0f } };
		if (!UVRect.IsInside(UV0) || !UVRect.IsInside(UV1) || !UVRect.IsInside(UV2))
			continue;

		const FVector& A = FVector(UV0, 0);
		const FVector& B = FVector(UV1, 0);
		;
		const FVector& C = FVector(UV2, 0);
		;
		const FVector BarycentricWeights = FMath::GetBaryCentric2D(FVector(UVPosition, 0), A, B, C);

		if ((BarycentricWeights.X >= 0.0f) && (BarycentricWeights.Y >= 0.0f) && (BarycentricWeights.Z >= 0.0f)
			&& FMath::IsNearlyEqual(BarycentricWeights.X + BarycentricWeights.Y + BarycentricWeights.Z, 1.0f))
		{
			FVector P0;
			FVector P1;
			FVector P2;

			Adapter->GetVertexPosition(TriangleVertexIndices[0], P0);
			Adapter->GetVertexPosition(TriangleVertexIndices[1], P1);
			Adapter->GetVertexPosition(TriangleVertexIndices[2], P2);
			Normal = FVector::CrossProduct(P2 - P0, P1 - P0).GetSafeNormal();

			OutTriangleVerts.Add(PaintableMeshComp->GetComponentTransform().TransformPosition(P0));
			OutTriangleVerts.Add(PaintableMeshComp->GetComponentTransform().TransformPosition(P1));
			OutTriangleVerts.Add(PaintableMeshComp->GetComponentTransform().TransformPosition(P2));

			OutWorldPosition += P0 * BarycentricWeights.X;
			OutWorldPosition += P1 * BarycentricWeights.Y;
			OutWorldPosition += P2 * BarycentricWeights.Z;
			OutWorldPosition = PaintableMeshComp->GetComponentTransform().TransformPosition(OutWorldPosition);

			break;
		}
	}
}

void UVSPMeshPaintBrush::CommitTextureChanges()
{
	if (bPaintModified)
	{
		UMeshPaintUtilities::WriteRTDataToTexture(AccumulatedRT, VSPPaintProperties->TextureToModify);
		bPaintModified = false;
	}
}

bool UVSPMeshPaintBrush::IsUsingBrushAlphaMask() const
{
	return VSPPaintProperties->BrushAlphaMask != nullptr;
}

void UVSPMeshPaintBrush::GenerateSeamMask()
{
	if (SeamMaskRT)
		SeamMaskRT->ConditionalBeginDestroy();
	int32 MaskSizeMultiplier = VSPPaintProperties->bUseUpscaledDilation ? 2 : 1;
	SeamMaskRT = UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize * MaskSizeMultiplier, TA_Clamp, TA_Clamp);
	UMaterialInterface* OriginalMeshMaterial = PaintableMeshComp->GetMaterial(0);
	VSPCheck(OriginalMeshMaterial);
	PaintableMeshComp->SetMaterial(0, UnwrapDynamicMaterial);

	UnwrapLocation = PaintableMeshComp->GetComponentLocation();
	AActor* SelectedActor = ToolBuilderInitialState.SelectedActors[0];
	USceneCaptureComponent2D* SceneCaptureComponent = Cast<USceneCaptureComponent2D>(SelectedActor->AddComponentByClass(
		USceneCaptureComponent2D::StaticClass(),
		false,
		FTransform::Identity,
		false));
	SceneCaptureComponent->SetWorldLocation(UnwrapLocation + FVector(0, 0, 10));
	SceneCaptureComponent->SetWorldRotation(FRotator(-90, -90, 0));
	SceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCaptureComponent->OrthoWidth = UnwrapCaptureSize;
	SceneCaptureComponent->ShowOnlyComponent(PaintableMeshComp);
	SceneCaptureComponent->TextureTarget = SeamMaskRT;
	SceneCaptureComponent->CaptureScene();

	SceneCaptureComponent->DestroyComponent();

	PaintableMeshComp->SetMaterial(0, OriginalMeshMaterial);

	//Remove edge pixels
	TArray<FColor> ColorData;
	FRenderTarget* RenderTarget = SeamMaskRT->GameThread_GetRenderTargetResource();
	RenderTarget->ReadPixels(ColorData);

	TArray<FColor> CorrectedColors = ColorData;

	for (int32 V = 0; V < SeamMaskRT->SizeY; ++V)
	{
		for (int32 U = 0; U < SeamMaskRT->SizeX; ++U)
		{
			FColor CurrentColor = ColorData[U + (V * SeamMaskRT->SizeX)];
			if (CurrentColor.A < 1)
			{
				bool bEdgePixel = false;
				for (FVector2D Offset : Offsets)
				{
					int32 OffsetU = FMath::Clamp(U + FMath::FloorToInt(Offset.X), 0, SeamMaskRT->SizeX - 1);
					int32 OffsetV = FMath::Clamp(V + FMath::FloorToInt(Offset.Y), 0, SeamMaskRT->SizeY - 1);
					FColor OffsetColor = ColorData[OffsetU + (OffsetV * SeamMaskRT->SizeX)];
					if (OffsetColor.A == 1)
					{
						bEdgePixel = true;
						break;
					}
				}
				if (bEdgePixel)
					CorrectedColors[U + (V * SeamMaskRT->SizeX)] = FColor::Transparent;
			}
		}
	}

	UTexture2D* SeamTexture = UTexture2D::CreateTransient(SeamMaskRT->SizeX, SeamMaskRT->SizeY, PF_R8G8B8A8);
	FColor* NewMipData = reinterpret_cast<FColor*>(SeamTexture->PlatformData->Mips[0].BulkData.Lock(LOCK_READ_WRITE));

	for (int32 V = 0; V < SeamMaskRT->SizeY; ++V)
	{
		for (int32 U = 0; U < SeamMaskRT->SizeX; ++U)
		{
			NewMipData[U + (V * SeamMaskRT->SizeX)] = CorrectedColors[U + (V * SeamMaskRT->SizeX)];
		}
	}
	SeamTexture->PlatformData->Mips[0].BulkData.Unlock();
	SeamTexture->UpdateResource();

	UMeshPaintUtilities::CopyTextureToRT(SeamTexture, SeamMaskRT);

	SeamTexture->ConditionalBeginDestroy();
}

void UVSPMeshPaintBrush::ApplyBrushRTToAccumulatedRT()
{
	FCanvas
		BrushPaintCanvas(AccumulatedRT->GameThread_GetRenderTargetResource(), nullptr, 0, 0, 0, SupportedFeatureLevel);
	BrushPaintCanvas.SetParentCanvasSize({ BrushRT->SizeX, BrushRT->SizeY });
	BrushPaintCanvas.Clear(FLinearColor::White);


	int32 SelectedTileMaskID = VSPPaintProperties->GetSelectedLayerID();

	AccumulateDynamicMaterial->SetScalarParameterValue(MaskIDParam, SelectedTileMaskID);
	AccumulateDynamicMaterial->SetScalarParameterValue(BrushStrengthParam, VSPPaintProperties->BrushStrength);
	AccumulateDynamicMaterial->SetScalarParameterValue(
		SmearApplyModeParam,
		static_cast<float>(VSPPaintProperties->Function));
	AccumulateDynamicMaterial->SetScalarParameterValue(
		UseBrushDilationParam,
		VSPPaintProperties->bUseBrushDilation ? 1 : 0);

	FMaterialRenderProxy* MaterialRenderProxy = AccumulateDynamicMaterial->GetRenderProxy();
	FCanvasTileItem Item(FVector2D(0), MaterialRenderProxy, TextureSize);
	Item.BlendMode = ESimpleElementBlendMode::SE_BLEND_AlphaHoldout;
	BrushPaintCanvas.DrawItem(Item);
	BrushPaintCanvas.Flush_GameThread(true);

	{
		FTextureRenderTargetResource* RenderTargetResource = AccumulatedRT->GameThread_GetRenderTargetResource();
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

void UVSPMeshPaintBrush::SetupTextureIdxToLayers()
{
	for (int32 LayerIdx = 0; LayerIdx < VSPPaintProperties->GetMaterialLayersCount(); ++LayerIdx)
	{
		const int32 TextureIdx = VSPPaintProperties->LayerToTextureIdx[LayerIdx];
		const FName MaterialParam = UVSPMeshPainterSettings::GetLayerToTextureIdxParamName(LayerIdx + 1);
		HelpPaintMeshMaterial->SetScalarParameterValue(MaterialParam, TextureIdx);
	}

	for (int32 LayerIdx = 0; LayerIdx < VSPPaintProperties->GetAlphaLayersCount(); ++LayerIdx)
	{
		const FName TextureIndexParam = UVSPMeshPainterSettings::GetAlphaToTextureIdxParamName(LayerIdx + 1);
		const FName TintColorParam = UVSPMeshPainterSettings::GetAlphaToTintColorParamName(LayerIdx + 1);
		const FName RoughnessParam = UVSPMeshPainterSettings::GetAlphaToRoughnessParamName(LayerIdx + 1);
		const FName TextureChannelParam = UVSPMeshPainterSettings::GetAlphaToTextureChannelParamName(LayerIdx + 1);
		HelpPaintMeshMaterial->SetScalarParameterValue(
			TextureIndexParam,
			VSPPaintProperties->AlphaDataToTexture[LayerIdx].TextureIndex);
		HelpPaintMeshMaterial->SetScalarParameterValue(
			RoughnessParam,
			VSPPaintProperties->AlphaDataToTexture[LayerIdx].Roughness);
		HelpPaintMeshMaterial->SetVectorParameterValue(
			TextureChannelParam,
			VSPPaintProperties->AlphaDataToTexture[LayerIdx].TextureChannel);
		if ((VSPPaintProperties->GetAlphaLayersCount() - LayerIdx) == 1)
		{
			PaintableMeshComp->SetDefaultCustomPrimitiveDataVector4(
				VSPPaintProperties->FirstTintCustomDataIndex,
				VSPPaintProperties->AlphaDataToTexture[LayerIdx].TintColor);
		}
		else
		{
			HelpPaintMeshMaterial->SetVectorParameterValue(
				TintColorParam,
				VSPPaintProperties->AlphaDataToTexture[LayerIdx].TintColor);
		}
	}

	bPaintModified = true;
}

void UVSPMeshPaintBrush::BuildDynamicMaterials()
{
	InitialMeshMaterial = CastChecked<UMaterialInstanceConstant>(PaintableMeshComp->GetMaterial(0));
	HelpPaintMeshMaterial = PaintableMeshComp->CreateDynamicMaterialInstance(0);
	HelpPaintMeshMaterial->SetTextureParameterValue(AccumulatedMaskParam, AccumulatedRT);

	BrushDynamicMaterial =
		UMaterialInstanceDynamic::Create(VSPPaintProperties->BrushMaterial, this, TEXT("BrushMaterial"));
	BrushDynamicMaterial->SetTextureParameterValue(BrushMaskParam, BrushRT);
	BrushDynamicMaterial->SetScalarParameterValue(MaskSizeXParam, TextureSize.X);
	BrushDynamicMaterial->SetScalarParameterValue(MaskSizeYParam, TextureSize.Y);

	AccumulateDynamicMaterial =
		UMaterialInstanceDynamic::Create(VSPPaintProperties->AccumulatorMaterial, this, TEXT("AccumulatMaterial"));
	AccumulateDynamicMaterial->SetTextureParameterValue(InitialMaskParam, DragInitialRT);
	AccumulateDynamicMaterial->SetTextureParameterValue(BrushMaskParam, BrushRT);
	AccumulateDynamicMaterial->SetScalarParameterValue(MaskSizeXParam, TextureSize.X);
	AccumulateDynamicMaterial->SetScalarParameterValue(MaskSizeYParam, TextureSize.Y);

	UnwrapDynamicMaterial =
		UMaterialInstanceDynamic::Create(VSPPaintProperties->UnwrapMaterial, this, TEXT("UnwrapMaterial"));
	UnwrapLocation = PaintableMeshComp->GetComponentLocation();
	UnwrapDynamicMaterial->SetScalarParameterValue(UnwrapCaptureSizeParam, UnwrapCaptureSize);
	UnwrapDynamicMaterial->SetVectorParameterValue(UnwrapLocationParam, FLinearColor(UnwrapLocation));

	FlushRenderingCommands();
}

void UVSPMeshPaintBrush::DestroyDynamicMaterials()
{
	PaintableMeshComp->SetMaterial(0, InitialMeshMaterial);
	InitialMeshMaterial = nullptr;
	HelpPaintMeshMaterial->ConditionalBeginDestroy();
	HelpPaintMeshMaterial = nullptr;
	BrushDynamicMaterial->ConditionalBeginDestroy();
	BrushDynamicMaterial = nullptr;
	AccumulateDynamicMaterial->ConditionalBeginDestroy();
	AccumulateDynamicMaterial = nullptr;
}

void UVSPMeshPaintBrush::BuildRenderTargets()
{
	BrushRT = UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize, TA_Clamp, TA_Clamp);
	UMeshPaintUtilities::ClearRT(BrushRT); //clearRT for a bug with 1st alpha stroke

	AccumulatedRT = UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize, TA_Clamp, TA_Clamp);
	UMeshPaintUtilities::ClearRT(AccumulatedRT);

	DragInitialRT = UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize, TA_Clamp, TA_Clamp);
	UMeshPaintUtilities::ClearRT(DragInitialRT);

	SeamMaskRT = UMeshPaintUtilities::CreateInternalRenderTarget(
		TextureSize * (VSPPaintProperties->bUseUpscaledDilation ? 2 : 1),
		TA_Clamp,
		TA_Clamp);
	UMeshPaintUtilities::ClearRT(SeamMaskRT);

	VSPPaintProperties->SeamMaskRT = SeamMaskRT;
	VSPPaintProperties->AccumulatedRT = AccumulatedRT;
	VSPPaintProperties->BrushRT = BrushRT;
	VSPPaintProperties->DragInitialRT = DragInitialRT;
}

void UVSPMeshPaintBrush::DestroyRenderTargets()
{
	BrushRT->ConditionalBeginDestroy();
	BrushRT = nullptr;
	DragInitialRT->ConditionalBeginDestroy();
	DragInitialRT = nullptr;
	AccumulatedRT->ConditionalBeginDestroy();
	AccumulatedRT = nullptr;
	SeamMaskRT->ConditionalBeginDestroy();
	SeamMaskRT = nullptr;
}

void UVSPMeshPaintBrush::CommitLayersChanges(UMaterialInstanceConstant* MaterialInstance) const
{
	for (int32 LayerIdx = 0; LayerIdx < VSPPaintProperties->GetMaterialLayersCount(); ++LayerIdx)
	{
		const int32 TextureIdx = VSPPaintProperties->LayerToTextureIdx[LayerIdx];
		const FName MaterialParam = UVSPMeshPainterSettings::GetLayerToTextureIdxParamName(LayerIdx + 1);
		MaterialInstance->SetScalarParameterValueEditorOnly(MaterialParam, TextureIdx);
	}

	for (int32 LayerIdx = 0; LayerIdx < VSPPaintProperties->GetAlphaLayersCount(); ++LayerIdx)
	{
		const int32 TextureIdx = VSPPaintProperties->AlphaDataToTexture[LayerIdx].TextureIndex;
		const FName MaterialTextureIdxParam = UVSPMeshPainterSettings::GetAlphaToTextureIdxParamName(LayerIdx + 1);
		const FName MaterialTintColorParam = UVSPMeshPainterSettings::GetAlphaToTintColorParamName(LayerIdx + 1);
		const FName MaterialTextureChannelParam =
			UVSPMeshPainterSettings::GetAlphaToTextureChannelParamName(LayerIdx + 1);
		const FName MaterialRoughnessParam = UVSPMeshPainterSettings::GetAlphaToRoughnessParamName(LayerIdx + 1);

		MaterialInstance->SetScalarParameterValueEditorOnly(MaterialTextureIdxParam, TextureIdx);
		MaterialInstance->SetScalarParameterValueEditorOnly(
			MaterialRoughnessParam,
			VSPPaintProperties->AlphaDataToTexture[LayerIdx].Roughness);
		MaterialInstance->SetVectorParameterValueEditorOnly(
			MaterialTextureChannelParam,
			VSPPaintProperties->AlphaDataToTexture[LayerIdx].TextureChannel);
		if (VSPPaintProperties->GetAlphaLayersCount() - LayerIdx > 1)
			MaterialInstance->SetVectorParameterValueEditorOnly(
				MaterialTintColorParam,
				VSPPaintProperties->AlphaDataToTexture[LayerIdx].TintColor);
		else
			MaterialInstance->SetVectorParameterValueEditorOnly(
				UVSPMeshPainterSettings::GetPaintTintBackupParamName(),
				VSPPaintProperties->AlphaDataToTexture[LayerIdx].TintColor);
	}

	MaterialInstance->MarkPackageDirty();
}

void UVSPMeshPaintBrush::CommitLayersChanges(UMaterialInstanceDynamic* MaterialInstance, bool bUVWindowMaterial) const
{
	for (int32 LayerIdx = 0; LayerIdx < VSPPaintProperties->GetMaterialLayersCount(); ++LayerIdx)
	{
		const int32 TextureIdx = VSPPaintProperties->LayerToTextureIdx[LayerIdx];
		const FName MaterialParam = UVSPMeshPainterSettings::GetLayerToTextureIdxParamName(LayerIdx + 1);
		MaterialInstance->SetScalarParameterValue(MaterialParam, TextureIdx);
	}

	for (int32 LayerIdx = 0; LayerIdx < VSPPaintProperties->GetAlphaLayersCount(); ++LayerIdx)
	{
		const int32 TextureIdx = VSPPaintProperties->AlphaDataToTexture[LayerIdx].TextureIndex;
		const FName MaterialTextureIdxParam = UVSPMeshPainterSettings::GetAlphaToTextureIdxParamName(LayerIdx + 1);
		const FName MaterialTintColorParam = UVSPMeshPainterSettings::GetAlphaToTintColorParamName(LayerIdx + 1);
		const FName MaterialTextureChannelParam =
			UVSPMeshPainterSettings::GetAlphaToTextureChannelParamName(LayerIdx + 1);
		const FName MaterialRoughnessParam = UVSPMeshPainterSettings::GetAlphaToRoughnessParamName(LayerIdx + 1);

		MaterialInstance->SetScalarParameterValue(MaterialTextureIdxParam, TextureIdx);
		MaterialInstance->SetScalarParameterValue(
			MaterialRoughnessParam,
			VSPPaintProperties->AlphaDataToTexture[LayerIdx].Roughness);
		MaterialInstance->SetVectorParameterValue(
			MaterialTextureChannelParam,
			VSPPaintProperties->AlphaDataToTexture[LayerIdx].TextureChannel);

		if (bUVWindowMaterial)
		{
			MaterialInstance->SetVectorParameterValue(
				MaterialTintColorParam,
				VSPPaintProperties->AlphaDataToTexture[LayerIdx].TintColor);
			continue;
			;
		}

		if (VSPPaintProperties->GetAlphaLayersCount() - LayerIdx > 1)
			MaterialInstance->SetVectorParameterValue(
				MaterialTintColorParam,
				VSPPaintProperties->AlphaDataToTexture[LayerIdx].TintColor);
		else
			MaterialInstance->SetVectorParameterValue(
				UVSPMeshPainterSettings::GetPaintTintBackupParamName(),
				VSPPaintProperties->AlphaDataToTexture[LayerIdx].TintColor);
	}

	MaterialInstance->MarkPackageDirty();
}
