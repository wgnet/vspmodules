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


#include "TerraformerBrushManager.h"

#include "Algo/Transform.h"
#include "Async/ParallelFor.h"
#include "Components/SplineComponent.h"
#include "Engine/Canvas.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Landscape.h"
#include "LandscapeDataAccess.h"
#include "LandscapeEdit.h"
#include "Misc/ScopedSlowTask.h"

#include "JumpFlood.h"
#include "ShapeRenderer.h"
#include "Terraformer.h"
#include "TerraformerSplineBrush.h"
#include "TerraformerStampBrush.h"
#include "TerrainGeneratorUtils.h"

#define LOCTEXT_NAMESPACE "TerraformerBrushManager"

ATerraformerBrushManager::ATerraformerBrushManager()
{
	AffectHeightmap = true;
	AffectWeightmap = false;

	if (!FTerraformerModule::Get().TerraformerUpdate.IsBoundToObject(this))
	{
		FTerraformerModule::Get().TerraformerUpdate.BindUObject(this, &ThisClass::ForceUpdate);
	}
}

void ATerraformerBrushManager::ForceUpdate()
{
	if (Landscape.IsValid())
	{
		bNeedUpdate = true;
		RequestLandscapeUpdate();
	}
}

void ATerraformerBrushManager::RecalculateNormals() const
{
	FlushRenderingCommands();

	FLandscapeEditDataInterface LandscapeEdit(Landscape->GetLandscapeInfo());
	LandscapeEdit.RecalculateNormals();
}

void ATerraformerBrushManager::UpdateTargetLandscape()
{
	SetTargetLandscape(Landscape.Get());
}

void ATerraformerBrushManager::RequestUpdate()
{
	if (Landscape.IsValid() && bAutoUpdate)
	{
		bNeedUpdate = true;
		RequestLandscapeUpdate();
	}
}

void ATerraformerBrushManager::UpdateBrushManager(UWorld* InWorld)
{
	// TODO FTerraformerModule::Get().TerraformerUpdate.ExecuteIfBound();
	for (auto It = TActorIterator<ATerraformerBrushManager>(InWorld); It; ++It)
	{
		It->RequestUpdate();
	}
}

void ATerraformerBrushManager::Initialize_Native(
	const FTransform& InLandscapeTransform,
	const FIntPoint& InLandscapeSize,
	const FIntPoint& InLandscapeRenderTargetSize)
{
	Super::Initialize_Native(InLandscapeTransform, InLandscapeSize, InLandscapeRenderTargetSize);
	UE_LOG(LogTerraformerEditor, Verbose, TEXT("Updated Landscape Transform"));

	if (bPadToPowerOfTwo)
	{
		LandscapeRTSize = InLandscapeRenderTargetSize;
	}
	else
	{
		const FIntRect PowerOfTwoSize = Landscape->GetBoundingRect() + 1;
		LandscapeRTSize = PowerOfTwoSize.Max;
	}

	LandscapeTransform = InLandscapeTransform;
	LandscapeQuads = InLandscapeSize;

	FVector Origin;
	FVector Extent;
	int32 Subsections;
	int32 QuadsSize;

	GetLandscapeCenterAndExtent(Landscape.Get(), Origin, Extent, Subsections, QuadsSize);

	LandscapeOrigin = Origin;
	LandscapeExtent = Extent;
	LandscapeSubsections = Subsections;
	LandscapeQuadsSize = QuadsSize;
	LandscapeSize = FVector(
		LandscapeQuads.X * LandscapeTransform.GetScale3D().X,
		LandscapeQuads.Y * LandscapeTransform.GetScale3D().Y,
		0.0f);
}

UTextureRenderTarget2D* ATerraformerBrushManager::Render_Native(
	bool InIsHeightmap,
	UTextureRenderTarget2D* InCombinedResult,
	const FName& InWeightmapLayerName)
{
	Super::Render_Native(InIsHeightmap, InCombinedResult, InWeightmapLayerName);

	if (!InIsHeightmap)
		return nullptr;

	if (!bNeedUpdate && IsValid(OutputHeightRT))
		return OutputHeightRT;

	if (bPadToPowerOfTwo)
	{
		LandscapeRTSize = FIntPoint(InCombinedResult->SizeX, InCombinedResult->SizeY);
	}
	else
	{
		const FIntRect PowerOfTwoSize = Landscape->GetBoundingRect() + 1;
		LandscapeRTSize = PowerOfTwoSize.Max;
	}

	AllocateRTs(LandscapeRTSize);
	CopyRenderTarget(InCombinedResult, SourceHeightRT);
	CopyRenderTarget(InCombinedResult, TargetHeightRT);

	TSet<ATerraformerBrushBase*> Brushes;
	for (TActorIterator<ATerraformerBrushBase> It = TActorIterator<ATerraformerBrushBase>(GetWorld()); It; ++It)
	{
		Brushes.Add(*It);
	}

	if (Brushes.Num() > 1)
	{
		Brushes.Sort(
			[](const ATerraformerBrushBase& A, const ATerraformerBrushBase& B)
			{
				const int32 PriorityA = A.Priority + A.GetGroupPriority();
				const int32 PriorityB = B.Priority + B.GetGroupPriority();
				return PriorityA < PriorityB;
			});
	}

	FScopedSlowTask ExploreFoldersTask(Brushes.Num(), LOCTEXT("Generate Heightmap", "Generate Heightmap"));
	ExploreFoldersTask.MakeDialog();
	for (ATerraformerBrushBase* Brush : Brushes)
	{
		ExploreFoldersTask.EnterProgressFrame();
		if (!IsValid(Brush))
		{
			UE_LOG(LogTerraformerEditor, Error, TEXT("INVALID Brush: Remove or Update"));
			continue;
		}

		if (const ATerraformerSplineBrush* SplineBrush = Cast<ATerraformerSplineBrush>(Brush))
		{
			if (GetDepthAndShapeRT())
			{
				DrawShape(SplineBrush);
			}

			if (JumpFloodRTA && JumpFloodRTB && GetDepthAndShapeRT() && TargetHeightRT && SourceHeightRT)
			{
				GenerateJumpFlood(SplineBrush);
			}
		}

		if (ATerraformerStampBrush* StampBrush = Cast<ATerraformerStampBrush>(Brush))
		{
			if (IsValid(TargetHeightRT) && IsValid(SourceHeightRT))
			{
				CopyRenderTarget(TargetHeightRT, SourceHeightRT);
				FTerraformerBrushRenderData BrushRenderData {
					SourceHeightRT,	 TargetHeightRT,	   Landscape->GetTransform(), LandscapeOrigin,
					LandscapeExtent, LandscapeSubsections, LandscapeQuadsSize
				};

				StampBrush->Render(BrushRenderData);
			}
		}
	}

	{
		const FIntPoint OutCombinedSize = FIntPoint(InCombinedResult->SizeX, InCombinedResult->SizeY);
		const TEnumAsByte<ETextureRenderTargetFormat> OutputFormat = InCombinedResult->RenderTargetFormat;
		const FName OutputRTName = TEXT("OutputHeight");

		OutputHeightRT = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
			OutputHeightRT,
			OutputRTName,
			OutCombinedSize,
			OutputFormat);
		CopyRenderTarget(TargetHeightRT, OutputHeightRT);
	}

	bNeedUpdate = false;
	return OutputHeightRT;
}

void ATerraformerBrushManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	return;
}

void ATerraformerBrushManager::AllocateRTs(const FIntPoint Size)
{
	SourceHeightRT =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(SourceHeightRT, TEXT("SourceHeight"), Size, RTF_RGBA8);
	TargetHeightRT =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(TargetHeightRT, TEXT("TargetHeight"), Size, RTF_RGBA8);
	JumpFloodRTA =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(JumpFloodRTA, TEXT("JumpFloodRTA"), Size, RTF_RGBA32f);
	JumpFloodRTB =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(JumpFloodRTB, TEXT("JumpFloodRTB"), Size, RTF_RGBA32f);
	DepthAndShapeRT =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(DepthAndShapeRT, TEXT("DepthAndShape"), Size, RTF_RGBA32f);
	DistanceFieldCacheRT = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
		DistanceFieldCacheRT,
		TEXT("DistanceFieldCache"),
		Size,
		RTF_RGBA32f);
}

DECLARE_GPU_STAT_NAMED(Terraformer_DrawShape, TEXT("Terraformer_DrawShape"));
void ATerraformerBrushManager::DrawShape(const ATerraformerSplineBrush* BrushActorRenderContext) const
{
	const FIntPoint RTSize(FIntPoint(GetDepthAndShapeRT()->SizeX, GetDepthAndShapeRT()->SizeY));
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	BrushActorRenderContext->GetTriangulatedShape(LandscapeTransform, RTSize, Vertices, Triangles);

	FTerraformerShapeDrawData DrawData { GetDepthAndShapeRT(), Vertices, Triangles };

	if (BrushActorRenderContext->BrushType == ETerraformerBrushType::Area)
	{
		ENQUEUE_RENDER_COMMAND(Terraformer_DrawShapeArea)
		(
			[Data = DrawData](FRHICommandListImmediate& RHICmdList)
			{
				SCOPED_NAMED_EVENT(Terraformer_DrawShape, FColor::Black);
				SCOPED_GPU_STAT(RHICmdList, Terraformer_DrawShape)
				FDrawShapePassProcessor::DrawAreaShape_RenderThread(RHICmdList, Data);
			});
	}

	if (BrushActorRenderContext->BrushType == ETerraformerBrushType::Path)
	{
		ENQUEUE_RENDER_COMMAND(Terraformer_DrawShapePath)
		(
			[Data = DrawData](FRHICommandListImmediate& RHICmdList)
			{
				SCOPED_NAMED_EVENT(Terraformer_DrawShape, FColor::Black);
				SCOPED_GPU_STAT(RHICmdList, Terraformer_DrawShape)
				FDrawShapePassProcessor::DrawPathShape_RenderThread(RHICmdList, Data);
			});
	}
}

DECLARE_GPU_STAT_NAMED(Terraformer_JumpFlood, TEXT("Terraformer_JumpFlood"));
DECLARE_GPU_STAT_NAMED(Terraformer_DetectEdges, TEXT("Terraformer_DetectEdges"));
DECLARE_GPU_STAT_NAMED(Terraformer_SmoothEdges, TEXT("Terraformer_BlurEdges"));
DECLARE_GPU_STAT_NAMED(Terraformer_JumpStep, TEXT("Terraformer_JumpStep"));
DECLARE_GPU_STAT_NAMED(Terraformer_CacheDistanceField, TEXT("Terraformer_CacheDistanceField"));
DECLARE_GPU_STAT_NAMED(Terraformer_BlendAngleFalloff, TEXT("Terraformer_BlendAngleFalloff"));
void ATerraformerBrushManager::GenerateJumpFlood(const ATerraformerSplineBrush* BrushActorRenderContext) const
{
	FTextureRenderTargetResource* ShapeResource = GetDepthAndShapeRT()->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* JumpFloodA = JumpFloodRTA->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* JumpFloodB = JumpFloodRTB->GameThread_GetRenderTargetResource();
	const bool bSmoothEdgesPass = (BrushActorRenderContext->SmoothEdges > 0) && bUseSmoothEdges;
	const int32 SmoothEdgesPasses = BrushActorRenderContext->SmoothEdges;
	const int32 RequiredPasses =
		FMath::CeilToInt(FMath::Loge(FMath::Max(JumpFloodRTA->SizeX, JumpFloodRTA->SizeY)) / FMath::Loge(2.0f));
	const float BrushPosition = BrushActorRenderContext->GetActorLocation().Z;
	const bool bClipShape = BrushActorRenderContext->bClipShape;
	const FIntRect RenderRegion = CalculateRenderRegion(BrushActorRenderContext);

	FJumpFloodData JumpFloodData {
		ShapeResource,	JumpFloodA,	   JumpFloodB, bSmoothEdgesPass, SmoothEdgesPasses,
		RequiredPasses, BrushPosition, bClipShape, RenderRegion
	};

	const bool bSmoothShape = (BrushActorRenderContext->SmoothShape > 0) && bUseSmoothShape;
	FCacheDistanceFieldData CacheDistanceFieldData {
		GetJumpFloodRT(BrushActorRenderContext)->GameThread_GetRenderTargetResource(),
		GetDepthAndShapeRT()->GameThread_GetRenderTargetResource(),
		DistanceFieldCacheRT->GameThread_GetRenderTargetResource(),
		(uint32)BrushActorRenderContext->SmoothShape,
		1,
		FVector2D(LandscapeRTSize),
		FVector2D(LandscapeSize),
		FVector2D::ZeroVector,
		LandscapeTransform.GetLocation().Z,
		LandscapeTransform.GetScale3D().Z / LANDSCAPE_INV_ZSCALE,
		BrushActorRenderContext->GetActorLocation().Z,
		BrushActorRenderContext->HeightOffset,
		(uint32)BrushActorRenderContext->FalloffWidth,
		BrushActorRenderContext->InvertShape,
		bSmoothShape,
		CalculateRenderRegion(BrushActorRenderContext)
	};

	constexpr float RawZScale = 1.0f;
	FTextureRenderTargetResource* TerrainHeightCache = SourceHeightRT->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* DistanceFieldCache = GetDistanceFieldCache()->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* TargetTargetHeight = TargetHeightRT->GameThread_GetRenderTargetResource();
	const float FalloffTangent =
		UKismetMathLibrary::DegTan(FMath::Clamp(BrushActorRenderContext->FalloffAngle, 1.f, 89.f)) / RawZScale;

	FBlendAngleFalloffData BlendAngleFalloffData {
		TerrainHeightCache,
		DistanceFieldCache,
		TargetTargetHeight,
		(uint32)BrushActorRenderContext->bCapShape,
		(uint32)BrushActorRenderContext->BlendMode,
		FVector2D::ZeroVector,
		FVector2D(LandscapeSize),
		FVector2D(LandscapeRTSize),
		LandscapeTransform.GetScale3D().Z / LANDSCAPE_INV_ZSCALE,
		FalloffTangent,
		(uint32)BrushActorRenderContext->FalloffWidth,
		BrushActorRenderContext->EdgeWidthOffset,
		BrushActorRenderContext->InnerSmoothThreshold,
		BrushActorRenderContext->OuterSmoothThreshold,
		BrushActorRenderContext->TerraceAlpha,
		BrushActorRenderContext->TerraceSmoothness,
		BrushActorRenderContext->TerraceSpacing,
		BrushActorRenderContext->TerraceMaskLength,
		BrushActorRenderContext->MaskStartOffset
	};

	ENQUEUE_RENDER_COMMAND(JumpFlood)
	(
		[JumpFloodData, CacheDistanceFieldData, BlendAngleFalloffData](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_NAMED_EVENT(Terraformer_JumpFlood, FColor::Black);
			SCOPED_GPU_STAT(RHICmdList, Terraformer_JumpFlood)

			{
				SCOPED_NAMED_EVENT(Terraformer_DetectEdges, FColor::Black);
				SCOPED_GPU_STAT(RHICmdList, Terraformer_DetectEdges)

				FJumpFloodPassProcessor::DetectEdges_RenderThread(
					RHICmdList,
					JumpFloodData.ShapeResource,
					JumpFloodData.JumpFloodA,
					JumpFloodData.BrushPosition,
					JumpFloodData.RenderRegion);
			}

			auto PingPongSourceLocal = [&JumpFloodData](const int32 InOffset, const int32 InCompletedPasses)
			{
				if ((InOffset + InCompletedPasses) % 2)
					return JumpFloodData.JumpFloodB;
				else
					return JumpFloodData.JumpFloodA;
			};

			auto PingPongTargetLocal = [&JumpFloodData](const int32 InOffset, const int32 InCompletedPasses)
			{
				if ((InOffset + InCompletedPasses + 1) % 2)
					return JumpFloodData.JumpFloodB;
				else
					return JumpFloodData.JumpFloodA;
			};

			const bool bSmoothEdges = JumpFloodData.bSmoothEdgesPass;
			const int32 Passes = JumpFloodData.SmoothEdgesPasses;
			const int32 NumBlurPasses = bSmoothEdges ? FMath::Clamp(Passes, 1, 8) : 0;
			for (int32 PassIndex = 0; PassIndex < NumBlurPasses; ++PassIndex)
			{
				SCOPED_NAMED_EVENT(Terraformer_SmoothEdges, FColor::Black);
				SCOPED_GPU_STAT(RHICmdList, Terraformer_SmoothEdges)

				const FTextureRenderTargetResource* Source = PingPongSourceLocal(0, PassIndex);
				const FTextureRenderTargetResource* Target = PingPongTargetLocal(0, PassIndex);
				FJumpFloodPassProcessor::SmoothEdges_RenderThread(
					RHICmdList,
					Source,
					Target,
					JumpFloodData.RenderRegion);
			}

			const int32 CircleBuffer = (bSmoothEdges ? FMath::Min(Passes, 8) : 0) % 2;
			int32 LastCompletedPass = 0;
			for (int32 CompletedPasses = 0; CompletedPasses < JumpFloodData.RequiredPasses; ++CompletedPasses)
			{
				SCOPED_NAMED_EVENT(Terraformer_JumpStep, FColor::Black);
				SCOPED_GPU_STAT(RHICmdList, Terraformer_JumpStep)

				const FTextureRenderTargetResource* Source = PingPongSourceLocal(CircleBuffer, CompletedPasses);
				const FTextureRenderTargetResource* Target = PingPongTargetLocal(CircleBuffer, CompletedPasses);
				FJumpFloodPassProcessor::JumpStep_RenderThread(
					RHICmdList,
					Source,
					Target,
					JumpFloodData.bClipShape,
					CompletedPasses,
					JumpFloodData.RenderRegion);
				LastCompletedPass++;
			}

			FTextureRenderTargetResource* DFCache = CacheDistanceFieldData.DistanceFieldCache;
			{
				SCOPED_NAMED_EVENT(Terraformer_CacheDistanceField, FColor::Black)
				SCOPED_GPU_STAT(RHICmdList, Terraformer_CacheDistanceField)
				FTextureRenderTargetResource* Target = PingPongTargetLocal(CircleBuffer, LastCompletedPass);
				FCacheDistanceFieldPassProcessor::CacheDistanceField_RenderThread(
					RHICmdList,
					CacheDistanceFieldData,
					Target,
					CacheDistanceFieldData.DepthShape,
					CacheDistanceFieldData.DistanceFieldCache,
					CacheDistanceFieldData.RenderRegion);
			}

			{
				SCOPED_NAMED_EVENT(Terraformer_BlendAngleFalloff, FColor::Black);
				SCOPED_GPU_STAT(RHICmdList, Terraformer_BlendAngleFalloff)
				const FTexture2DRHIRef Source = BlendAngleFalloffData.TerrainHeightCache->GetRenderTargetTexture();
				const FTextureRHIRef Target = BlendAngleFalloffData.OutputResource->TextureRHI;
				RHICmdList.CopyToResolveTarget(Target, Source, FResolveParams());

				FBlendAngleFalloffPassProcessor::BlendAngleFalloff_RenderThread(RHICmdList, BlendAngleFalloffData);
			}
			RHICmdList.BlockUntilGPUIdle();
		});
}

void ATerraformerBrushManager::CacheBrushDistanceField(const ATerraformerSplineBrush* BrushActorRenderContext) const
{
	const bool bSmoothShape = (BrushActorRenderContext->SmoothShape > 0) && bUseSmoothShape;
	FCacheDistanceFieldData CacheDistanceFieldData {
		GetJumpFloodRT(BrushActorRenderContext)->GameThread_GetRenderTargetResource(),
		GetDepthAndShapeRT()->GameThread_GetRenderTargetResource(),
		DistanceFieldCacheRT->GameThread_GetRenderTargetResource(),
		(uint32)BrushActorRenderContext->SmoothShape,
		1,
		FVector2D(LandscapeRTSize),
		FVector2D(LandscapeSize),
		FVector2D::ZeroVector,
		LandscapeTransform.GetLocation().Z,
		LandscapeTransform.GetScale3D().Z / LANDSCAPE_INV_ZSCALE,
		BrushActorRenderContext->GetActorLocation().Z,
		BrushActorRenderContext->HeightOffset,
		(uint32)BrushActorRenderContext->FalloffWidth,
		BrushActorRenderContext->InvertShape,
		bSmoothShape,
		CalculateRenderRegion(BrushActorRenderContext)
	};

	ENQUEUE_RENDER_COMMAND(CacheDistanceField)
	(
		[Cache = MoveTemp(CacheDistanceFieldData)](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_NAMED_EVENT(Terraformer_CacheDistanceField, FColor::Black)
			SCOPED_GPU_STAT(RHICmdList, Terraformer_CacheDistanceField)

			FCacheDistanceFieldPassProcessor::CacheDistanceField_RenderThread(
				RHICmdList,
				Cache,
				Cache.JumpFlood,
				Cache.DepthShape,
				Cache.DistanceFieldCache,
				Cache.RenderRegion);
			RHICmdList.BlockUntilGPUIdle();
		});
}

void ATerraformerBrushManager::BlendAngleFalloff(
	const ATerraformerSplineBrush* BrushActorRenderContext,
	UTextureRenderTarget2D* InSourceHeight,
	UTextureRenderTarget2D* InTargetHeight) const
{
	constexpr float RawZScale = 1.0f;
	FTextureRenderTargetResource* TerrainHeightCache = InSourceHeight->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* DistanceFieldCache = GetDistanceFieldCache()->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* Target = InTargetHeight->GameThread_GetRenderTargetResource();

	FBlendAngleFalloffData BlendAngleFalloffData {
		TerrainHeightCache,
		DistanceFieldCache,
		Target,
		(uint32)BrushActorRenderContext->bCapShape,
		(uint32)BrushActorRenderContext->BlendMode,
		FVector2D::ZeroVector,
		FVector2D(LandscapeSize),
		FVector2D(LandscapeRTSize),
		LandscapeTransform.GetScale3D().Z / LANDSCAPE_INV_ZSCALE,
		UKismetMathLibrary::DegTan(FMath::Clamp(BrushActorRenderContext->FalloffAngle, 1.f, 89.f)) / RawZScale,
		(uint32)BrushActorRenderContext->FalloffWidth,
		BrushActorRenderContext->EdgeWidthOffset,
		BrushActorRenderContext->InnerSmoothThreshold,
		BrushActorRenderContext->OuterSmoothThreshold,
		BrushActorRenderContext->TerraceAlpha,
		BrushActorRenderContext->TerraceSmoothness,
		BrushActorRenderContext->TerraceSpacing,
		BrushActorRenderContext->TerraceMaskLength,
		BrushActorRenderContext->MaskStartOffset
	};

	ENQUEUE_RENDER_COMMAND(BlendAngleFalloff)
	(
		[this, BlendAngleFalloffData](FRHICommandListImmediate& RHICmdList)
		{
			SCOPED_NAMED_EVENT(Terraformer_BlendAngleFalloff, FColor::Black);
			SCOPED_GPU_STAT(RHICmdList, Terraformer_BlendAngleFalloff)
			FBlendAngleFalloffPassProcessor::BlendAngleFalloff_RenderThread(RHICmdList, BlendAngleFalloffData);
			RHICmdList.BlockUntilGPUIdle();
		});
}

void ATerraformerBrushManager::CopyRenderTarget(UTextureRenderTarget2D* Source, UTextureRenderTarget2D* Dest) const
{
	FTextureRenderTargetResource* SourceResource = Source->GameThread_GetRenderTargetResource();
	FTextureRenderTargetResource* DestResource = Dest->GameThread_GetRenderTargetResource();

	if (SourceResource && DestResource)
	{
		ENQUEUE_RENDER_COMMAND(UpdateDrawTextureToRTCommand)
		(
			[SourceResource, DestResource](FRHICommandListImmediate& RHICmdList)
			{
				const int32 X = FMath::Min(SourceResource->GetSizeX(), DestResource->GetSizeX());
				const int32 Y = FMath::Min(SourceResource->GetSizeY(), DestResource->GetSizeY());
				const FResolveRect Rect(0, 0, X, Y);

				RHICmdList.CopyToResolveTarget(
					SourceResource->GetRenderTargetTexture(),
					DestResource->TextureRHI,
					FResolveParams(Rect));
			});
	}
}

void ATerraformerBrushManager::SetTargetLandscape(ALandscape* InTargetLandscape)
{
	if (OwningLandscape != InTargetLandscape)
	{
		if (OwningLandscape)
		{
			OwningLandscape->RemoveBrush(this);
		}

		if (InTargetLandscape && InTargetLandscape->CanHaveLayersContent())
		{
			FName TerraformerLayerName = TEXT("Terraformer");
			int32 ExistingTerraformerLayerIndex = InTargetLandscape->GetLayerIndex(TerraformerLayerName);
			if (ExistingTerraformerLayerIndex == INDEX_NONE)
			{
				ExistingTerraformerLayerIndex = InTargetLandscape->CreateLayer(TerraformerLayerName);
			}
			InTargetLandscape->AddBrushToLayer(ExistingTerraformerLayerIndex, this);
			InTargetLandscape->SetLayerLocked(ExistingTerraformerLayerIndex, true);
		}
	}
}

FIntRect ATerraformerBrushManager::CalculateRenderRegion(const ATerraformerSplineBrush* BrushActorRenderContext) const
{
	const float FalloffWidth = BrushActorRenderContext->FalloffWidth;
	const FBoxSphereBounds Bounds = BrushActorRenderContext->GetSplineComponent()->Bounds;
	const FVector PosMin =
		LandscapeTransform.InverseTransformPosition(Bounds.Origin - (Bounds.BoxExtent + FalloffWidth));
	const FVector PosMax =
		LandscapeTransform.InverseTransformPosition(Bounds.Origin + (Bounds.BoxExtent + FalloffWidth));

	const FIntPoint RecMin = FIntPoint(
		FMath::Max(FMath::Min(LandscapeRTSize.X, FVector2D(PosMin).IntPoint().X), 0),
		FMath::Max(FMath::Min(LandscapeRTSize.Y, FVector2D(PosMin).IntPoint().Y), 0));

	const FIntPoint RecMax = FIntPoint(
		FMath::Max(FMath::Min(LandscapeRTSize.X, FVector2D(PosMax).IntPoint().X), 0),
		FMath::Max(FMath::Min(LandscapeRTSize.Y, FVector2D(PosMax).IntPoint().Y), 0));

	FIntRect RenderRegion =
		bRenderRegions ? FIntRect(RecMin, RecMax) : FIntRect(0, 0, LandscapeRTSize.X, LandscapeRTSize.Y);

	return RenderRegion;
}

UTextureRenderTarget2D* ATerraformerBrushManager::GetDistanceFieldCache() const
{
	return DistanceFieldCacheRT;
}

UTextureRenderTarget2D* ATerraformerBrushManager::GetJumpFloodRT(
	const ATerraformerSplineBrush* BrushActorRenderContext) const
{
	const bool bUseBlurPass = (BrushActorRenderContext->SmoothEdges > 0) && bUseSmoothEdges;
	const int32 BlurPasses = BrushActorRenderContext->SmoothEdges;
	const int32 RequiredPasses =
		FMath::CeilToInt(FMath::Loge((float)FMath::Max(JumpFloodRTA->SizeX, JumpFloodRTA->SizeY)) / FMath::Loge(2.0f));
	const int32 Offset = (bUseBlurPass ? FMath::Min(BlurPasses, 9) : 0) % 2;
	return PingPongTarget(Offset, RequiredPasses);
}

bool ATerraformerBrushManager::IsClockwise(const USplineComponent* InSpline)
{
	double PolySignedArea = 0.f;
	const int32 PointsNum = InSpline->GetNumberOfSplinePoints();
	for (int32 Idx = 0, LastIdx = PointsNum - 1; Idx < PointsNum; LastIdx = Idx++)
	{
		const FVector& V1 = InSpline->GetLocationAtSplinePoint(LastIdx, ESplineCoordinateSpace::World);
		const FVector& V2 = InSpline->GetLocationAtSplinePoint(Idx, ESplineCoordinateSpace::World);
		PolySignedArea += V1.X * V2.Y - V1.Y * V2.X;
	}
	PolySignedArea *= 0.5f;
	return PolySignedArea < 0;
}

FLinearColor ATerraformerBrushManager::PackFloatToRG8(const float InValue)
{
	FLinearColor Result;
	float Remap = FMath::Clamp(InValue, -32768.f, 32767.f) + 32768.f;
	Remap = FMath::Floor(Remap);
	Result.R = FMath::Floor(Remap / 256.f) / 255.f;
	Result.G = FMath::Floor(FMath::Fmod(Remap, 256.f)) / 255.f;
	Result.B = 1;
	Result.A = 1;
	return Result / 65536.f;
}

FLinearColor ATerraformerBrushManager::PackFloatToRGB8(float InValue)
{
	const float Max24int = 256 * 256 * 256 - 1;
	float Remap = FMath::Clamp(InValue, -32768.f, 32767.f) + 32768.f;
	Remap /= 65536.f;
	FVector Decomposition = FVector((Max24int / (256 * 256)) * Remap, (Max24int / 256) * Remap, Max24int * Remap);
	Decomposition =
		FVector(FMath::Floor(Decomposition.X), FMath::Floor(Decomposition.Y), FMath::Floor(Decomposition.Z)) / 255.f;
	Decomposition.Z -= Decomposition.Y * 256;
	Decomposition.Y -= Decomposition.X * 256;
	return FLinearColor(Decomposition.X, Decomposition.Y, Decomposition.Z, 1);
}

void ATerraformerBrushManager::GetLandscapeCenterAndExtent(
	const ALandscape* InLandscape,
	FVector& OutCenter,
	FVector& OutExtent,
	int32& OutSubsections,
	int32& OutQuadsSize)
{
	const FIntRect LandscapeRect = InLandscape->GetBoundingRect();
	const FVector MidPoint = FVector(LandscapeRect.Min, 0.f) + FVector(LandscapeRect.Size(), 0.f) * 0.5f;
	const int32 NumSubsections = InLandscape->NumSubsections;
	const int32 ComponentSizeQuads = InLandscape->ComponentSizeQuads;

	OutCenter = InLandscape->GetTransform().TransformPosition(MidPoint);
	OutExtent = FVector(LandscapeRect.Size(), 0.f) * InLandscape->GetActorScale() * 0.5f;
	OutSubsections = NumSubsections;
	OutQuadsSize = ComponentSizeQuads;
}

UTextureRenderTarget2D* ATerraformerBrushManager::PingPongTarget(int32 Offset, int32 CompletedPasses) const
{
	if ((Offset + CompletedPasses + 1) % 2)
	{
		return JumpFloodRTB;
	}
	else
	{
		return JumpFloodRTA;
	}
}

UTextureRenderTarget2D* ATerraformerBrushManager::PingPongSource(int32 Offset, int32 CompletedPasses) const
{
	if ((Offset + CompletedPasses) % 2)
	{
		return JumpFloodRTB;
	}
	else
	{
		return JumpFloodRTA;
	}
}

#undef LOCTEXT_NAMESPACE
