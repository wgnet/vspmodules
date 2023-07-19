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
#include "InLayerMaskBakeTool.h"

#include "CanvasItem.h"
#include "Engine/Classes/Materials/MaterialInstanceDynamic.h"
#include "Engine/TextureRenderTarget2D.h"
#include "MeshPaintUtilities.h"
#include "VSPCheck.h"
#include "VSPMeshPainterSettings.h"
#include "VSPToolsProperties.h"
#include "ShaderParameterUtils.h"

using namespace VSPMeshPainterExpressions;

bool UInLayerMaskBakeToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	return UVSPMeshPainterSettings::Get()->IsSupportToPaint(
		UMeshPaintUtilities::GetSingleSelectedStaticMeshComp(SceneState));
}

UInteractiveTool* UInLayerMaskBakeToolBuilder::BuildTool(const FToolBuilderState& SceneState) const
{
	UInLayerMaskBakeTool* NewTool = NewObject<UInLayerMaskBakeTool>();
	NewTool->Init(SceneState);
	return NewTool;
}

void UInLayerMaskBakeTool::Init(const FToolBuilderState& SceneState)
{
	ToolBuilderInitialState = SceneState;
}

void UInLayerMaskBakeTool::Setup()
{
	WhiteSquare = UTexture2D::CreateTransient(64, 64);

	MeshComp = UMeshPaintUtilities::GetSingleSelectedStaticMeshComp(ToolBuilderInitialState);

	BakeProperties = NewObject<UMaskBakeProperties>();
	AddToolPropertySource(BakeProperties);

	BakeProperties->OnUpdate.BindLambda(
		[this]()
		{
			if (BakeProperties->Mask)
				ApplyMaskToLayersMaskRT();
		});
	BakeProperties->OnApplyBake.BindLambda(
		[this]()
		{
			CommitLayersMaskRTToModifyTexture();
		});
	BakeProperties->SetupForMeshComp(MeshComp);
	UTexture2D* TextureToModify = BakeProperties->TextureToModify;

	TextureSize = FIntPoint(TextureToModify->Source.GetSizeX(), TextureToModify->Source.GetSizeY());
	BrushRT = UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize, TA_Clamp, TA_Clamp);
	UMeshPaintUtilities::ClearRT(BrushRT);

	InitialLayersMaskRT = UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize, TA_Clamp, TA_Clamp);
	UMeshPaintUtilities::ClearRT(InitialLayersMaskRT);

	LayersMaskRT = UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize, TA_Clamp, TA_Clamp);
	UMeshPaintUtilities::ClearRT(LayersMaskRT);
	UMeshPaintUtilities::CopyTextureToRT(TextureToModify, LayersMaskRT);
	UMeshPaintUtilities::CopyTextureToRT(TextureToModify, InitialLayersMaskRT);

	InitialMeshMaterial = CastChecked<UMaterialInstanceConstant>(MeshComp->GetMaterial(0));
	MeshDMI = MeshComp->CreateDynamicMaterialInstance(0);
	MeshDMI->SetTextureParameterValue(AccumulatedMaskParam, LayersMaskRT);
	MaskBakeDMI = UMaterialInstanceDynamic::Create(
		UVSPMeshPainterSettings::Get()->GetMaskBakeMaterial(),
		this,
		TEXT("MaskBakeDMI"));
	AccumulateDMI = UMaterialInstanceDynamic::Create(BakeProperties->AccumulateMat, this, TEXT("AccumulateDMI"));

	MaskForMultFromInitialRT = UMeshPaintUtilities::CreateInternalRenderTarget(TextureSize, TA_Clamp, TA_Clamp);
}

void UInLayerMaskBakeTool::Shutdown(EToolShutdownType ShutdownType)
{
	BakeProperties->OnUpdate.Unbind();

	MeshComp->SetMaterial(0, InitialMeshMaterial);

	InitialLayersMaskRT->ConditionalBeginDestroy();
	InitialLayersMaskRT = nullptr;

	LayersMaskRT->ConditionalBeginDestroy();
	LayersMaskRT = nullptr;

	BrushRT->ConditionalBeginDestroy();
	BrushRT = nullptr;

	MeshDMI->ConditionalBeginDestroy();
	MeshDMI = nullptr;

	AccumulateDMI->ConditionalBeginDestroy();
	AccumulateDMI = nullptr;

	MaskBakeDMI->ConditionalBeginDestroy();
	MaskBakeDMI = nullptr;
}

void UInLayerMaskBakeTool::WriteToRenderTarget(
	int TargetLayer,
	UTexture* Mask,
	UTexture* Mask2,
	UMaterialInstanceDynamic* MaterialInstanceDynamic,
	UTextureRenderTarget2D* TargetRT,
	FBox2D BakeArea)
{
	FCanvas BrushPaintCanvas(TargetRT->GameThread_GetRenderTargetResource(), nullptr, 0, 0, 0, SupportedFeatureLevel);
	BrushPaintCanvas.SetParentCanvasSize(TextureSize);

	MaterialInstanceDynamic->SetTextureParameterValue(BrushMaskParam, Mask);
	MaterialInstanceDynamic->SetTextureParameterValue(BrushMask2Param, Mask2);
	MaterialInstanceDynamic->SetScalarParameterValue(MaskIDParam, TargetLayer);
	MaterialInstanceDynamic->SetScalarParameterValue(BInvertBrushMaskParam, BakeProperties->bInvertMask);
	MaterialInstanceDynamic->SetScalarParameterValue(BrushMaskContrastParam, BakeProperties->Contrast);
	MaterialInstanceDynamic->SetScalarParameterValue(BrushStrengthParam, BakeProperties->Strength);

	FCanvasTileItem Item(
		BakeArea.Min * TextureSize,
		MaterialInstanceDynamic->GetRenderProxy(),
		BakeArea.GetSize() * TextureSize,
		FVector2D::ZeroVector,
		FVector2D::UnitVector);
	Item.BlendMode = ESimpleElementBlendMode::SE_BLEND_AlphaBlend;
	Item.Draw(&BrushPaintCanvas);

	BrushPaintCanvas.Flush_GameThread(true);

	{
		ENQUEUE_RENDER_COMMAND(UpdateMeshPaintRTCommand1)
		(
			[this, TargetRT](FRHICommandListImmediate& RHICmdList)
			{
				FTextureRenderTargetResource* RenderTargetResource = TargetRT->GetRenderTargetResource();
				RHICmdList.CopyToResolveTarget(
					RenderTargetResource->GetRenderTargetTexture(),
					RenderTargetResource->TextureRHI,
					FResolveParams());
			});
	}

	FlushRenderingCommands();
}

void UInLayerMaskBakeTool::ApplyMaskToLayersMaskRT()
{
	//reset LayersMaskRT to prevent accumulation of results and clear BrushRT
	UTexture2D* TextureToModify = BakeProperties->TextureToModify;
	UMeshPaintUtilities::CopyTextureToRT(TextureToModify, LayersMaskRT);
	UMeshPaintUtilities::ClearRT(BrushRT, BrushRT_ClearColor);
	UMeshPaintUtilities::ClearRT(MaskForMultFromInitialRT, BrushRT_ClearColor);
	FlushRenderingCommands();

	int TargetLayer = BakeProperties->TargetLayer;
	FBox2D BakeArea;

	//get mask2 for from existing channel for multiplication with ExternalMask
	if (BakeProperties->UseMaskMultiplication)
	{
		BakeArea = FBox2D(FVector2D(0, 0), FVector2D(1, 1));
		MaskBakeDMI->SetScalarParameterValue("UseMask_2", 1);
		MaskBakeDMI->SetScalarParameterValue("Invert", 0);
		MaskBakeDMI->SetScalarParameterValue("ChannelIndex", BakeProperties->TargetLayer_ForMaskMultiplication);
		MaskBakeDMI->SetScalarParameterValue("MaskWhiteOverlay", 0);
		WriteToRenderTarget(
			TargetLayer,
			LayersMaskRT,
			LayersMaskRT,
			MaskBakeDMI,
			MaskForMultFromInitialRT,
			BakeArea); //used only mask2, mask1 discarded
	}

	//paint external mask to BrushRT (optionally mult )
	UTexture* MaskForMult = MaskForMultFromInitialRT;
	if (!BakeProperties->UseMaskMultiplication)
	{
		MaskForMult = WhiteSquare;
	}
	MaskBakeDMI->SetScalarParameterValue("Invert", TargetLayer % 4 == 3);
	MaskBakeDMI->SetScalarParameterValue("UseMask_2", 0);
	MaskBakeDMI->SetScalarParameterValue("MaskWhiteOverlay", BakeProperties->MaskWhiteOverlayValue);
	BakeArea = GetBakeArea(BakeProperties->LayersCount, TargetLayer);
	WriteToRenderTarget(TargetLayer, BakeProperties->Mask, MaskForMult, MaskBakeDMI, BrushRT, BakeArea);


	//write brushRT to accumulatedRT
	FCanvas
		BrushPaintCanvas2(LayersMaskRT->GameThread_GetRenderTargetResource(), nullptr, 0, 0, 0, SupportedFeatureLevel);
	BrushPaintCanvas2.SetParentCanvasSize(TextureSize);

	AccumulateDMI->SetScalarParameterValue(MaskIDParam, TargetLayer);
	AccumulateDMI->SetTextureParameterValue(InitialMaskParam, InitialLayersMaskRT);
	AccumulateDMI->SetTextureParameterValue(BrushMaskParam, BrushRT);
	AccumulateDMI->SetScalarParameterValue(MaskSizeXParam, TextureSize.X);
	AccumulateDMI->SetScalarParameterValue(MaskSizeYParam, TextureSize.Y);
	AccumulateDMI->SetScalarParameterValue(SmearApplyModeParam, static_cast<float>(BakeProperties->Function));
	AccumulateDMI->SetScalarParameterValue(
		MaskSmearApplyByCurrentDistributionParam,
		BakeProperties->bMaskSmearApplyByCurrentDistribution);

	FCanvasTileItem Item2(FVector2D(0), AccumulateDMI->GetRenderProxy(), TextureSize);
	BrushPaintCanvas2.DrawItem(Item2);
	BrushPaintCanvas2.Flush_GameThread(true);

	{
		ENQUEUE_RENDER_COMMAND(UpdateMeshPaintRTCommand1)
		(
			[this](FRHICommandListImmediate& RHICmdList)
			{
				FTextureRenderTargetResource* RenderTargetResource = LayersMaskRT->GetRenderTargetResource();
				RHICmdList.CopyToResolveTarget(
					RenderTargetResource->GetRenderTargetTexture(),
					RenderTargetResource->TextureRHI,
					FResolveParams());
			});
	}

	FlushRenderingCommands();
}

void UInLayerMaskBakeTool::CommitLayersMaskRTToModifyTexture()
{
	UMeshPaintUtilities::WriteRTDataToTexture(LayersMaskRT, BakeProperties->TextureToModify);
	UMeshPaintUtilities::CopyTextureToRT(BakeProperties->TextureToModify, InitialLayersMaskRT);
}

FBox2D UInLayerMaskBakeTool::GetBakeArea(int32 LayersCount, int32 TargetLayer)
{
	FBox2D Out;

	VSPCheckReturn(LayersCount >= 0, Out);

	if (LayersCount <= 4)
	{
		Out.Min = FVector2D::ZeroVector;
		Out.Max = FVector2D::UnitVector;
	}

	if (4 < LayersCount && LayersCount <= 8)
	{
		Out.Min = TargetLayer < 4 ? FVector2D(0, 0) : FVector2D(0.5, 0);
		Out.Max = TargetLayer < 4 ? FVector2D(0.5, 1) : FVector2D(1, 1);
	}

	if (8 < LayersCount && LayersCount <= 16)
	{
		float MinX = TargetLayer < 4 || (8 <= TargetLayer && TargetLayer <= 12) ? 0 : 0.5;
		float MinY = TargetLayer < 8 ? 0 : 0.5;

		float MaxX = TargetLayer < 4 || (8 <= TargetLayer && TargetLayer <= 12) ? 0.5 : 1;
		float MaxY = TargetLayer < 8 ? 0.5 : 1;

		Out.Min = FVector2D(MinX, MinY);
		Out.Max = FVector2D(MaxX, MaxY);
	}

	check(LayersCount <= 16);

	return Out;
}
