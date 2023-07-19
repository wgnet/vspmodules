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


#include "TerrainGeneratorHelper.h"

#include "IMaterialBakingModule.h"
#include "MaterialBakingStructures.h"
#include "StaticMeshAttributes.h"

#include "CanvasItem.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Landscape.h"
#include "LandscapeEdit.h"
#include "LandscapeEditorModule.h"
#include "LandscapeInfo.h"
#include "LandscapeProxy.h"
#include "LandscapeStreamingProxy.h"
#include "Logging/MessageLog.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Rendering/Texture2DResource.h"
#include "ScopedTransaction.h"
#include "TerrainGeneratorUtils.h"


#define LOCTEXT_NAMESPACE "UTerrainGeneratorHelper"

bool UTerrainGeneratorHelper::TerrainExportHeightmapToRenderTarget(
	ALandscape* InLandscape,
	UTextureRenderTarget2D* InRenderTarget,
	bool InExportHeightIntoRGChannel,
	bool InExportLandscapeProxies)
{
	if (!IsValid(InLandscape))
	{
		FMessageLog("TerrainGeneratorHelper")
			.Error(FText::FromString("WeightmapFromRenderTarget: Landscape must be non-null."));
		return false;
	}
#if WITH_EDITOR
	UMaterial* HeightmapRenderMaterial = LoadObject<UMaterial>(
		nullptr,
		TEXT(
			"/Engine/EditorLandscapeResources/Landscape_Heightmap_To_RenderTarget2D.Landscape_Heightmap_To_RenderTarget2D"));
	if (HeightmapRenderMaterial == nullptr)
	{
		FMessageLog("UTerrainGeneratorHelper")
			.Error(FText::FromString(
				"LandscapeExportHeightmapToRenderTarget: Material Landscape_Heightmap_To_RenderTarget2D not found in engine content"));
		return false;
	}

	TArray<ULandscapeComponent*> LandscapeComponentsToExport;
	LandscapeComponentsToExport.Append(InLandscape->LandscapeComponents);

	if (InExportLandscapeProxies && InLandscape)
	{
		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
		for (ALandscapeProxy* Proxy : LandscapeInfo->Proxies)
		{
			LandscapeComponentsToExport.Append(Proxy->LandscapeComponents);
		}
	}

	if (LandscapeComponentsToExport.Num() == 0)
	{
		return true;
	}

	UWorld* World = InLandscape->GetWorld(); //GEditor->GetEditorWorldContext().World();
	FTextureRenderTargetResource* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource();

	FCanvas Canvas(RenderTargetResource, nullptr, 0, 0, 0, World->FeatureLevel);
	Canvas.Clear(FLinearColor::Black);

	FIntRect ComponentsExtent(MAX_int32, MAX_int32, MIN_int32, MIN_int32);
	for (ULandscapeComponent* Component : LandscapeComponentsToExport)
	{
		Component->GetComponentExtent(
			ComponentsExtent.Min.X,
			ComponentsExtent.Min.Y,
			ComponentsExtent.Max.X,
			ComponentsExtent.Max.Y);
	}
	FIntPoint ExportBaseOffset = ComponentsExtent.Min;

	struct FTrianglePerMID
	{
		UMaterialInstanceDynamic* HeightmapMID;
		TArray<FCanvasUVTri> TriangleList;
	};

	TMap<UTexture*, FTrianglePerMID> TrianglesPerHeightmap;

	for (const ULandscapeComponent* Component : LandscapeComponentsToExport)
	{
		FTrianglePerMID* TrianglesPerMID = TrianglesPerHeightmap.Find(Component->GetHeightmap());

		if (TrianglesPerMID == nullptr)
		{
			FTrianglePerMID Data;
			Data.HeightmapMID = UMaterialInstanceDynamic::Create(HeightmapRenderMaterial, InLandscape->GetWorld());
			Data.HeightmapMID->SetTextureParameterValue(TEXT("Heightmap"), Component->GetHeightmap());
			Data.HeightmapMID->SetScalarParameterValue(TEXT("ExportHeightIntoRGChannel"), InExportHeightIntoRGChannel);
			TrianglesPerMID = &TrianglesPerHeightmap.Add(Component->GetHeightmap(), Data);
		}

		FIntPoint ComponentSectionBase = Component->GetSectionBase();
		FIntPoint ComponentHeightmapTextureSize(
			Component->GetHeightmap()->Source.GetSizeX(),
			Component->GetHeightmap()->Source.GetSizeY());
		int32 SubsectionSizeVerts = Component->SubsectionSizeQuads + 1;
		float HeightmapSubsectionOffsetU = (float)(SubsectionSizeVerts) / (float)ComponentHeightmapTextureSize.X;
		float HeightmapSubsectionOffsetV = (float)(SubsectionSizeVerts) / (float)ComponentHeightmapTextureSize.Y;

		for (int8 SubY = 0; SubY < InLandscape->NumSubsections; ++SubY)
		{
			for (int8 SubX = 0; SubX < InLandscape->NumSubsections; ++SubX)
			{
				FIntPoint SubSectionSectionBase = ComponentSectionBase - ExportBaseOffset;
				SubSectionSectionBase.X += Component->SubsectionSizeQuads * SubX;
				SubSectionSectionBase.Y += Component->SubsectionSizeQuads * SubY;

				float HeightmapOffsetU = Component->HeightmapScaleBias.Z + HeightmapSubsectionOffsetU * (float)SubX;
				float HeightmapOffsetV = Component->HeightmapScaleBias.W + HeightmapSubsectionOffsetV * (float)SubY;

				FCanvasUVTri Tri1;
				Tri1.V0_Pos = FVector2D(SubSectionSectionBase.X, SubSectionSectionBase.Y);
				Tri1.V1_Pos = FVector2D(SubSectionSectionBase.X + SubsectionSizeVerts, SubSectionSectionBase.Y);
				Tri1.V2_Pos = FVector2D(
					SubSectionSectionBase.X + SubsectionSizeVerts,
					SubSectionSectionBase.Y + SubsectionSizeVerts);

				Tri1.V0_UV = FVector2D(HeightmapOffsetU, HeightmapOffsetV);
				Tri1.V1_UV = FVector2D(HeightmapOffsetU + HeightmapSubsectionOffsetU, HeightmapOffsetV);
				Tri1.V2_UV = FVector2D(
					HeightmapOffsetU + HeightmapSubsectionOffsetU,
					HeightmapOffsetV + HeightmapSubsectionOffsetV);
				TrianglesPerMID->TriangleList.Add(Tri1);

				FCanvasUVTri Tri2;
				Tri2.V0_Pos = FVector2D(
					SubSectionSectionBase.X + SubsectionSizeVerts,
					SubSectionSectionBase.Y + SubsectionSizeVerts);
				Tri2.V1_Pos = FVector2D(SubSectionSectionBase.X, SubSectionSectionBase.Y + SubsectionSizeVerts);
				Tri2.V2_Pos = FVector2D(SubSectionSectionBase.X, SubSectionSectionBase.Y);

				Tri2.V0_UV = FVector2D(
					HeightmapOffsetU + HeightmapSubsectionOffsetU,
					HeightmapOffsetV + HeightmapSubsectionOffsetV);
				Tri2.V1_UV = FVector2D(HeightmapOffsetU, HeightmapOffsetV + HeightmapSubsectionOffsetV);
				Tri2.V2_UV = FVector2D(HeightmapOffsetU, HeightmapOffsetV);

				TrianglesPerMID->TriangleList.Add(Tri2);
			}
		}
	}

	for (auto& TriangleList : TrianglesPerHeightmap)
	{
		FCanvasTriangleItem TriItemList(MoveTemp(TriangleList.Value.TriangleList), nullptr);
		TriItemList.MaterialRenderProxy = TriangleList.Value.HeightmapMID->GetRenderProxy();
		TriItemList.BlendMode = SE_BLEND_Opaque;
		TriItemList.SetColor(FLinearColor::White);

		TriItemList.Draw(&Canvas);
	}

	TrianglesPerHeightmap.Reset();

	Canvas.Flush_GameThread(true);

	ENQUEUE_RENDER_COMMAND(DrawHeightmapRTCommand)
	(
		[RenderTargetResource](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.CopyToResolveTarget(
				RenderTargetResource->GetRenderTargetTexture(),
				// Source texture
				RenderTargetResource->TextureRHI,
				// Dest texture
				FResolveParams()); // Resolve parameters
		});

	FlushRenderingCommands();
#endif
	return true;
}

bool UTerrainGeneratorHelper::TerrainImportHeightmapFromRenderTarget(
	ALandscape* InLandscape,
	UTextureRenderTarget2D* InRenderTarget,
	bool InImportHeightFromRGChannel)
{
	if (InLandscape == nullptr)
	{
		FMessageLog("Blueprint")
			.Error(LOCTEXT(
				"LandscapeImportHeightmapFromRenderTarget_NullLandscape",
				"LandscapeImportHeightmapFromRenderTarget: Landscape must be non-null."));
		return false;
	}

	if (InLandscape->HasLayersContent())
	{
		//todo: Support an edit layer name input parameter to support import to edit layers.
		FMessageLog("Blueprint")
			.Error(LOCTEXT(
				"LandscapeImportHeightmapFromRenderTarget_LandscapeLayersNotSupported",
				"LandscapeImportHeightmapFromRenderTarget: Cannot import to landscape with Edit Layers enabled."));
		return false;
	}

	int32 MinX, MinY, MaxX, MaxY;
	ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();

	if (!LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
	{
		FMessageLog("Blueprint")
			.Error(LOCTEXT(
				"LandscapeImportHeightmapFromRenderTarget_InvalidLandscapeExtends",
				"LandscapeImportHeightmapFromRenderTarget: The landscape min extends are invalid."));
		return false;
	}

	if (InRenderTarget == nullptr || InRenderTarget->Resource == nullptr)
	{
		FMessageLog("Blueprint")
			.Error(LOCTEXT(
				"LandscapeImportHeightmapFromRenderTarget_InvalidRT",
				"LandscapeImportHeightmapFromRenderTarget: Render Target must be non null and not released."));
		return false;
	}

	FTextureRenderTargetResource* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource();
	FIntRect SampleRect = FIntRect(
		0,
		0,
		FMath::Min(1 + MaxX - MinX, InRenderTarget->SizeX),
		FMath::Min(1 + MaxY - MinY, InRenderTarget->SizeY));

	TArray<uint16> HeightData;

	switch (InRenderTarget->RenderTargetFormat)
	{
	case RTF_RGBA16f:
	case RTF_RGBA32f:
	{
		TArray<FLinearColor> OutputRTHeightmap;
		OutputRTHeightmap.Reserve(SampleRect.Width() * SampleRect.Height());

		RenderTargetResource->ReadLinearColorPixels(
			OutputRTHeightmap,
			FReadSurfaceDataFlags(RCM_MinMax, CubeFace_MAX),
			SampleRect);
		HeightData.Reserve(OutputRTHeightmap.Num());

		for (auto LinearColor : OutputRTHeightmap)
		{
			if (InImportHeightFromRGChannel)
			{
				FColor Color = LinearColor.ToFColor(false);
				uint16 Height = ((Color.R << 8) | Color.G);
				HeightData.Add(Height);
			}
			else
			{
				HeightData.Add((uint16)LinearColor.R);
			}
		}
	}
	break;

	case RTF_RGBA8:
	{
		TArray<FColor> OutputRTHeightmap;
		OutputRTHeightmap.Reserve(SampleRect.Width() * SampleRect.Height());

		RenderTargetResource->ReadPixels(
			OutputRTHeightmap,
			FReadSurfaceDataFlags(RCM_MinMax, CubeFace_MAX),
			SampleRect);
		HeightData.Reserve(OutputRTHeightmap.Num());

		for (FColor Color : OutputRTHeightmap)
		{
			uint16 Height = ((Color.R << 8) | Color.G);
			HeightData.Add(Height);
		}
	}
	break;

	default:
	{
		FMessageLog("Blueprint")
			.Error(LOCTEXT(
				"LandscapeImportHeightmapFromRenderTarget_InvalidRTFormat",
				"LandscapeImportHeightmapFromRenderTarget: The Render Target format is invalid. We only support RTF_RGBA16f, RTF_RGBA32f, RTF_RGBA8"));
		return false;
	}
	}

	FScopedTransaction Transaction(LOCTEXT("Undo_ImportHeightmap", "Importing Landscape Heightmap"));

	FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
	HeightmapAccessor.SetData(MinX, MinY, SampleRect.Width() - 1, SampleRect.Height() - 1, HeightData.GetData());

	return true;
}


bool UTerrainGeneratorHelper::TerrainImportWeightmapFromRenderTarget(
	ALandscape* InLandscape,
	UTextureRenderTarget2D* InRenderTarget,
	FName InLayerName)
{
	if (InLandscape)
	{
		if (InLandscape->HasLayersContent())
		{
			FMessageLog("TerrainGeneratorHelper")
				.Error(FText::FromString(
					"WeightmapFromRenderTarget: Cannot import to landscape with Edit Layers enabled."));
			return false;
		}

		ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();

		//ULandscapeLayerInfoObject* LandscapeLayerInfoObject = LandscapeInfo->Layers[ 0 ].LayerInfoObj;

		int32 MinX, MinY, MaxX, MaxY;
		if (LandscapeInfo && LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
		{
			const uint32 LandscapeWidth = (uint32)(1 + MaxX - MinX);
			const uint32 LandscapeHeight = (uint32)(1 + MaxY - MinY);
			FLinearColor SampleRect = FLinearColor(0, 0, LandscapeWidth, LandscapeHeight);

			const uint32 RTWidth = InRenderTarget->SizeX;
			const uint32 RTHeight = InRenderTarget->SizeY;
			ETextureRenderTargetFormat format = (InRenderTarget->RenderTargetFormat);

			if (RTWidth >= LandscapeWidth && RTHeight >= LandscapeHeight)
			{
				TArray<FLinearColor> RTData = SampleRTData(InRenderTarget, SampleRect);

				TArray<uint8> LayerData;
				LayerData.SetNum(RTData.Num());
				for (int32 i = 0; i < RTData.Num(); i++)
				{
					LayerData[i] = ((uint8)(FMath::Clamp((float)RTData[i].R, 0.0f, 1.0f) * 255));
				}

				FLandscapeInfoLayerSettings CurWeightmapInfo;
				//ALandscapeProxy* LandscapeProxy = LandscapeInfo->GetLandscapeProxy();
				int32 Index = LandscapeInfo->GetLayerInfoIndex(InLayerName /*, LandscapeProxy*/);

				if (ensure(Index != INDEX_NONE))
				{
					CurWeightmapInfo = LandscapeInfo->Layers[Index];
				}

				if (CurWeightmapInfo.LayerInfoObj == nullptr)
				{
					FMessageLog("TerrainGeneratorHelper")
						.Error(FText::FromString(
							"WeightmapFromRenderTarget: Layers must first have Layer Info Objects assigned before importing."));
					return false;
				}

				FScopedTransaction Transaction(FText::FromString("Importing Landscape Layer"));

				FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, CurWeightmapInfo.LayerInfoObj);
				AlphamapAccessor.SetData(
					MinX,
					MinY,
					MaxX,
					MaxY,
					LayerData.GetData(),
					ELandscapeLayerPaintingRestriction::None);

				return true;
			}
			else
			{
				FMessageLog("TerrainGeneratorHelper")
					.Error(FText::FromString(
						"WeightmapFromRenderTarget: Render target must be at least as large as landscape on each axis."));
				return false;
			}
		}
		else
		{
			return false;
		}
	}
	FMessageLog("TerrainGeneratorHelper")
		.Error(FText::FromString("WeightmapFromRenderTarget: Landscape must be non-null."));
	return false;
}

bool UTerrainGeneratorHelper::TerrainExportWeightmapToRenderTarget(
	ALandscape* InLandscape,
	UTextureRenderTarget2D* InRenderTarget,
	FName InLayerName)
{
	if (InLandscape && InRenderTarget)
	{
		if (ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo())
		{
			FLandscapeInfoLayerSettings CurWeightmapInfo;
			int32 Index = LandscapeInfo->GetLayerInfoIndex(InLayerName /*, LandscapeProxy*/);

			if (ensure(Index != INDEX_NONE))
			{
				CurWeightmapInfo = LandscapeInfo->Layers[Index];
			}

			int32 X1 = 0;
			int32 Y1 = 0;
			int32 X2 = InRenderTarget->SizeX - 1;
			int32 Y2 = InRenderTarget->SizeY - 1;
			TArray<uint8> Data;
			Data.AddZeroed(InRenderTarget->SizeX * InRenderTarget->SizeY);
			FLandscapeEditDataInterface LandscapeEdit(LandscapeInfo);
			LandscapeEdit.GetWeightData(CurWeightmapInfo.LayerInfoObj, X1, Y1, X2, Y2, Data.GetData(), 0);

			if (FTextureRenderTargetResource* RTResourceDest = InRenderTarget->GameThread_GetRenderTargetResource())
			{
				ENQUEUE_RENDER_COMMAND(UpdateTexture2D)
				(
					[&RTResourceDest, Data = MoveTemp(Data)](FRHICommandListImmediate& RHICmdList)
					{
						RHICmdList.UpdateTexture2D(
							RTResourceDest->GetTexture2DRHI(),
							0,
							FUpdateTextureRegion2D(0, 0, 0, 0, RTResourceDest->GetSizeX(), RTResourceDest->GetSizeY()),
							RTResourceDest->GetSizeX() * sizeof(uint8),
							Data.GetData());
					});

				FlushRenderingCommands();
				return true;
			}
		}
	}
	return false;
}


void UTerrainGeneratorHelper::InternalImportWheightmapToEditingLayer(
	ULandscapeInfo* LandscapeInfo,
	ULandscapeLayerInfoObject* LandscapeLayerInfoObject,
	UTextureRenderTarget2D* InRenderTarget)
{
	if (!LandscapeInfo || !IsValid(LandscapeLayerInfoObject))
		return;

	int32 MinX, MinY, MaxX, MaxY;
	if (LandscapeInfo && LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY))
	{
		const uint32 LandscapeWidth = (uint32)(1 + MaxX - MinX);
		const uint32 LandscapeHeight = (uint32)(1 + MaxY - MinY);
		FLinearColor SampleRect = FLinearColor(0, 0, LandscapeWidth, LandscapeHeight);

		const uint32 RTWidth = InRenderTarget->SizeX;
		const uint32 RTHeight = InRenderTarget->SizeY;

		if (RTWidth >= LandscapeWidth && RTHeight >= LandscapeHeight)
		{
			TArray<FLinearColor> RTData = SampleRTData(InRenderTarget, SampleRect);

			TArray<uint8> LayerData;

			for (auto i : RTData)
			{
				LayerData.Add((uint8)(FMath::Clamp((float)i.R, 0.0f, 1.0f) * 255));
			}

			FScopedTransaction Transaction(FText::FromString("Importing Landscape Layer"));

			FAlphamapAccessor<false, false> AlphamapAccessor(LandscapeInfo, LandscapeLayerInfoObject);
			AlphamapAccessor.SetData(
				MinX,
				MinY,
				MaxX,
				MaxY,
				LayerData.GetData(),
				ELandscapeLayerPaintingRestriction::None);
		}
	}
}


FMeshDescription* UTerrainGeneratorHelper::GetLandscapeDescription(ALandscapeProxy* Landscape)
{
	const int32 LandscapeLOD = 0;
	const FVector LandscapeWorldLocation = Landscape->GetActorLocation();

	//UStaticMesh* StaticMesh = NewObject< UStaticMesh >(GetTransientPackage(),"TempMesh", RF_Transient );
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
	StaticMesh->InitResources();
	StaticMesh->SetLightingGuid(FGuid::NewGuid());
	StaticMesh->SetLightMapResolution(64);
	StaticMesh->SetLightMapCoordinateIndex(1);

	FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();

	SrcModel.BuildSettings.bRecomputeNormals = false;
	SrcModel.BuildSettings.bRecomputeTangents = false;
	SrcModel.BuildSettings.bRemoveDegenerates = false;
	SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SrcModel.BuildSettings.bUseFullPrecisionUVs = false;

	StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

	FMeshDescription* LandscapeRawMesh = StaticMesh->CreateMeshDescription(0);
	FStaticMeshAttributes Attributes(*LandscapeRawMesh);

	Landscape->ExportToRawMesh(LandscapeLOD, *LandscapeRawMesh);

	TVertexAttributesRef<FVector> VertexPositions = Attributes.GetVertexPositions();
	for (const FVertexID VertexID : LandscapeRawMesh->Vertices().GetElementIDs())
	{
		VertexPositions[VertexID] -= LandscapeWorldLocation;
	}

	return LandscapeRawMesh;
}


void UTerrainGeneratorHelper::MergeRGBAndAlphaData(
	const TArray<FColor>& RGB,
	const TArray<FColor>& Alpha,
	TArray<FColor>& OutColor)
{
	check(RGB.Num() == Alpha.Num()) check(OutColor.Num() == 0)

		for (int32 i = 0; i < RGB.Num(); ++i)
	{
		OutColor.Add(FColor(RGB[i].R, RGB[i].G, RGB[i].B, Alpha[i].A));
	}
}


void UTerrainGeneratorHelper::TerrainImportWeightmapFromRenderTargetToEditingLayer(
	ALandscapeProxy* InLandscapeProxy,
	UTextureRenderTarget2D* InRenderTarget,
	const FString& InEditingLayerName,
	const FString& InMaterialLayerName)
{
	if (!IsValid(InLandscapeProxy) || !IsValid(InRenderTarget))
	{
		return;
	}

	auto IsLayerNameMatch = [InEditingLayerName](const FLandscapeLayer& InLayer)
	{
		return InLayer.Name == *InEditingLayerName;
	};

	FGuid GuidForActivate;

	FLandscapeLayer* FoundLayerPtr { InLandscapeProxy->GetLandscapeActor()->LandscapeLayers.FindByPredicate(
		IsLayerNameMatch) };
	if (FoundLayerPtr)
	{
		const FLandscapeLayer& FoundLayer { *FoundLayerPtr };
		GuidForActivate = FoundLayer.Guid;
	}

	if (GuidForActivate.IsValid())
	{
		InLandscapeProxy->GetLandscapeActor()->SetEditingLayer(GuidForActivate);
	}

	ULandscapeInfo* LandscapeInfo { InLandscapeProxy->GetLandscapeInfo() };

	auto IsMaterialNameMatch = [InMaterialLayerName](const FLandscapeEditorLayerSettings& InEditorLayerSettings)
	{
		return InEditorLayerSettings.LayerInfoObj->LayerName == *InMaterialLayerName;
	};

	FLandscapeEditorLayerSettings* FoundEditorLayerSettingsPtr =
		InLandscapeProxy->EditorLayerSettings.FindByPredicate(IsMaterialNameMatch);
	if (FoundEditorLayerSettingsPtr)
	{
		const FLandscapeEditorLayerSettings& FoundEditorLayerSettings { *FoundEditorLayerSettingsPtr };
		InternalImportWheightmapToEditingLayer(LandscapeInfo, FoundEditorLayerSettings.LayerInfoObj, InRenderTarget);
	}

	//	Clear editing layers
	InLandscapeProxy->GetLandscapeActor()->SetEditingLayer();
}

void UTerrainGeneratorHelper::ExportWeightLayer(ALandscape* InLandscape, FString Dir, FName InLayerName)
{
	ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
	FLandscapeInfoLayerSettings LandscapeInfoLayerSettings;
	int32 Index = LandscapeInfo->GetLayerInfoIndex(InLayerName);

	if (ensure(Index != INDEX_NONE))
	{
		LandscapeInfoLayerSettings = LandscapeInfo->Layers[Index];
	}
	FString TotalFileName = FPaths::Combine(Dir, InLayerName.ToString() + ".PNG");
	LandscapeInfo->ExportLayer(LandscapeInfoLayerSettings.LayerInfoObj, TotalFileName);
}


UStaticMesh* UTerrainGeneratorHelper::ConvertLandscapeToStaticMesh(ALandscapeProxy* Landscape)
{
	const FVector LandscapeWorldLocation = Landscape->GetActorLocation();
	//UStaticMesh* StaticMesh = NewObject< UStaticMesh >(GetTransientPackage(),"TempMesh", RF_Transient );
	UStaticMesh* StaticMesh = NewObject<UStaticMesh>();
	const int32 LandscapeLOD = 0;
	StaticMesh->InitResources();
	StaticMesh->SetLightingGuid(FGuid::NewGuid());
	StaticMesh->SetLightMapResolution(64);
	StaticMesh->SetLightMapCoordinateIndex(1);

	FStaticMeshSourceModel& SrcModel = StaticMesh->AddSourceModel();

	SrcModel.BuildSettings.bRecomputeNormals = false;
	SrcModel.BuildSettings.bRecomputeTangents = false;
	SrcModel.BuildSettings.bRemoveDegenerates = false;
	SrcModel.BuildSettings.bUseHighPrecisionTangentBasis = false;
	SrcModel.BuildSettings.bUseFullPrecisionUVs = false;

	// TODO Assign the proxy material to the static mesh
	//StaticMesh->StaticMaterials.Add(FStaticMaterial(StaticLandscapeMaterial));

	//Set the Imported version before calling the build
	StaticMesh->ImportVersion = EImportStaticMeshVersion::LastVersion;

	FMeshDescription* LandscapeRawMesh = StaticMesh->CreateMeshDescription(0);
	FStaticMeshAttributes Attributes(*LandscapeRawMesh);

	Landscape->ExportToRawMesh(LandscapeLOD, *LandscapeRawMesh);

	TVertexAttributesRef<FVector> VertexPositions = Attributes.GetVertexPositions();
	for (const FVertexID VertexID : LandscapeRawMesh->Vertices().GetElementIDs())
	{
		VertexPositions[VertexID] -= LandscapeWorldLocation;
	}

	//Commit raw mesh and build the StaticMesh
	{
		StaticMesh->CommitMeshDescription(0);
		StaticMesh->Build();
		StaticMesh->PostEditChange();
	}

	return StaticMesh;
}


bool UTerrainGeneratorHelper::BakeLandscape(
	ALandscapeProxy* Landscape,
	UMaterialInterface* Material,
	FIntPoint Size,
	int32 UVChannel,
	TArray<FColor>& OutData)
{
	TArray<FBakeOutput> BakeOutputs;
	FMaterialData MaterialSetting;
	MaterialSetting.Material = Material;
	MaterialSetting.PropertySizes.Add(MP_EmissiveColor, Size);
	MaterialSetting.PropertySizes.Add(MP_Opacity, Size);
	MaterialSetting.bPerformBorderSmear = false;

	FMeshDescription MeshDescription = *GetLandscapeDescription(Landscape);
	FStaticMeshAttributes(MeshDescription).Register();

	FMeshData MeshSetting;
	MeshSetting.RawMeshDescription = &MeshDescription;
	MeshSetting.TextureCoordinateIndex = UVChannel;

	int32 MaterialIndex = 0;
	while (MaterialIndex >= 0)
	{
		MeshSetting.MaterialIndices.Add(MaterialIndex--);
	}

	MeshSetting.TextureCoordinateBox = FBox2D(FVector2D(0, 0), FVector2D(1, 1));

	IMaterialBakingModule& Module = FModuleManager::Get().LoadModuleChecked<IMaterialBakingModule>("MaterialBaking");
	Module.BakeMaterials({ &MaterialSetting }, { &MeshSetting }, BakeOutputs);

	const TArray<FColor>& RGB = *BakeOutputs[0].PropertyData.Find(MP_EmissiveColor);
	const TArray<FColor>& Alpha = *BakeOutputs[0].PropertyData.Find(MP_Opacity);
	MergeRGBAndAlphaData(RGB, Alpha, OutData);

	return true;
}

bool UTerrainGeneratorHelper::TerrainImportHeightmap(
	ALandscape* InLandscape,
	TArray<uint16>& InHeightDada,
	FIntPoint Size)
{
	int32 MinX, MinY, MaxX, MaxY;
	ULandscapeInfo* LandscapeInfo = InLandscape->GetLandscapeInfo();
	LandscapeInfo->GetLandscapeExtent(MinX, MinY, MaxX, MaxY);
	const FIntRect SampleRect =
		FIntRect(0, 0, FMath::Min(1 + MaxX - MinX, Size.X), FMath::Min(1 + MaxY - MinY, Size.Y));
	FHeightmapAccessor<false> HeightmapAccessor(LandscapeInfo);
	HeightmapAccessor.SetData(MinX, MinY, SampleRect.Width() - 1, SampleRect.Height() - 1, InHeightDada.GetData());

	return true;
}

bool UTerrainGeneratorHelper::TerrainExportHeightmap(ALandscape* InLandscape, TArray<uint16>& OutHeightDada)
{
	int32 MinX, MinY, MaxX, MaxY;
	FLandscapeEditDataInterface LandscapeEdit(InLandscape->GetLandscapeInfo());
	LandscapeEdit.GetHeightData(MinX, MinY, MaxX, MaxY, OutHeightDada.GetData(), 0);

	return true;
}

UTexture2D* UTerrainGeneratorHelper::CreateTransientDynamicTexture(
	const int32 Width,
	const int32 Height,
	const EPixelFormat PixelFormat)
{
	UTexture2D* Texture = UTexture2D::CreateTransient(Width, Height, PixelFormat);
	if (Texture)
	{
		Texture->CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
		Texture->SRGB = 0;
		Texture->UpdateResource();
	}
	return Texture;
}


void UTerrainGeneratorHelper::UpdateTextureRegion(
	UTexture2D* Texture,
	int32 MipIndex,
	FUpdateTextureRegion2D Region,
	uint32 SrcPitch,
	uint32 SrcBpp,
	uint8* SrcData,
	bool bFreeData)
{
	if (Texture->Resource)
	{
		struct FUpdateTextureRegionsData
		{
			FTexture2DResource* Texture2DResource;
			int32 MipIndex;
			FUpdateTextureRegion2D Region;
			uint32 SrcPitch;
			uint32 SrcBpp;
			uint8* SrcData;
		};

		FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;

		RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
		RegionData->MipIndex = MipIndex;
		RegionData->Region = Region;
		RegionData->SrcPitch = SrcPitch;
		RegionData->SrcBpp = SrcBpp;
		RegionData->SrcData = SrcData;
		{
			ENQUEUE_RENDER_COMMAND(UpdateTextureRegionsData)
			(
				[RegionData, bFreeData](FRHICommandListImmediate& RHICmdList)
				{
					const int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
					if (RegionData->MipIndex >= CurrentFirstMip)
					{
						RHIUpdateTexture2D(
							RegionData->Texture2DResource->GetTexture2DRHI(),
							RegionData->MipIndex - CurrentFirstMip,
							RegionData->Region,
							RegionData->SrcPitch,
							RegionData->SrcData + RegionData->Region.SrcY * RegionData->SrcPitch
								+ RegionData->Region.SrcX * RegionData->SrcBpp);
					}
					// TODO is this leaking if we never set this to true??
					if (bFreeData)
					{
						FMemory::Free(RegionData->SrcData);
					}
					delete RegionData;
				});
		}
	}
}


void UTerrainGeneratorHelper::UpdateDynamicVectorTexture(const TArray<FLinearColor>& Source, UTexture2D* Texture)
{
	// Only handles 32-bit float textures
	if (!Texture || Texture->GetPixelFormat() != PF_A32B32G32R32F)
		return;
	// Shouldn't do anything if there's no data
	if (Source.Num() < 1)
		return;

	UpdateTextureRegion(
		Texture,
		0,
		FUpdateTextureRegion2D(0, 0, 0, 0, Texture->GetSizeX(), Texture->GetSizeY()),
		Texture->GetSizeX() * sizeof(FLinearColor),
		sizeof(FLinearColor),
		(uint8*)Source.GetData(),
		false);
}


TArray<FColor> UTerrainGeneratorHelper::DownsampleTextureData(TArray<FColor>& Data, int32 Width, int32 Kernel)
{
	TArray<FColor> Downsampled;
	Downsampled.Init(FColor(), Data.Num() / (Kernel * Kernel));

	int32 Row = 0;
	for (int32 i = 0; i < Downsampled.Num(); i++)
	{
		TArray<FColor> KernelColors;
		KernelColors.Init(FColor(0, 0, 0, 255), Kernel * Kernel);

		int32 KernelIndex = i * Kernel;
		if (KernelIndex % Width == 0 && i != 0)
		{
			Row++;
		}

		KernelIndex += Row * Width * (Kernel - 1);

		for (int32 k = 0; k < Kernel; ++k)
		{
			for (int32 kr = 0; kr < Kernel; ++kr)
			{
				int32 c = KernelIndex + k + (Width * kr);
				if (c > 0 && c < Data.Num())
				{
					KernelColors[k * Kernel + kr] = Data[c];
				}
			}
		}

		int32 AvR = 0;
		int32 AvG = 0;
		int32 AvB = 0;
		int32 AvA = 0;
		for (int32 j = 0; j < KernelColors.Num(); ++j)
		{
			AvR += KernelColors[j].R;
			AvG += KernelColors[j].G;
			AvB += KernelColors[j].B;
			AvA += KernelColors[j].A;
		}

		FColor KernelAvarage;
		KernelAvarage.R = (uint8)(AvR / KernelColors.Num());
		KernelAvarage.G = (uint8)(AvG / KernelColors.Num());
		KernelAvarage.B = (uint8)(AvB / KernelColors.Num());
		KernelAvarage.A = (uint8)(AvA / KernelColors.Num());
		Downsampled[i] = KernelAvarage;
	}

	return Downsampled;
}

UTexture2D* UTerrainGeneratorHelper::CreateTextureFromBGRA(FColor* InData, int32 InWidth, int32 InHeight)
{
	UTexture2D* Texture;

	Texture = UTexture2D::CreateTransient(InWidth, InHeight, PF_B8G8R8A8);
	if (!Texture)
	{
		return nullptr;
	}

#if WITH_EDITORONLY_DATA
	Texture->MipGenSettings = TMGS_NoMipmaps;
#endif
	Texture->NeverStream = true;

	Texture->SRGB = 0;

	FTexture2DMipMap& Mip = Texture->PlatformData->Mips[0];
	void* Data = Mip.BulkData.Lock(LOCK_READ_WRITE);

	FMemory::Memcpy(Data, InData, InWidth * InHeight * 4);
	Mip.BulkData.Unlock();
	Texture->UpdateResource();

	return Texture;
}

void UTerrainGeneratorHelper::CopyTextureToRenderTargetTexture(
	UTexture* SourceTexture,
	UTextureRenderTarget2D* RenderTargetTexture,
	ERHIFeatureLevel::Type FeatureLevel)
{
	check(SourceTexture != nullptr);
	check(RenderTargetTexture != nullptr);

	FTextureRenderTargetResource* RenderTargetResource = RenderTargetTexture->GameThread_GetRenderTargetResource();
	check(RenderTargetResource != nullptr);

	FCanvas Canvas(RenderTargetResource, nullptr, 0, 0, 0, FeatureLevel);

	const uint32 Width = RenderTargetTexture->GetSurfaceWidth();
	const uint32 Height = RenderTargetTexture->GetSurfaceHeight();

	FTexture* TextureResource = nullptr;
	UTexture2D* Texture2D = Cast<UTexture2D>(SourceTexture);
	if (Texture2D != nullptr)
	{
		TextureResource = Texture2D->Resource;
	}
	else
	{
		UTextureRenderTarget2D* TextureRenderTarget2D = Cast<UTextureRenderTarget2D>(SourceTexture);
		TextureResource = TextureRenderTarget2D->GameThread_GetRenderTargetResource();
	}
	check(TextureResource != nullptr);

	// Draw a quad to copy the texture over to the render target
	{
		const float MinU = 0.0f;
		const float MinV = 0.0f;
		const float MaxU = 1.0f;
		const float MaxV = 1.0f;
		const float MinX = 0.0f;
		const float MinY = 0.0f;
		const float MaxX = Width;
		const float MaxY = Height;

		FCanvasUVTri Tri1;
		FCanvasUVTri Tri2;
		Tri1.V0_Pos = FVector2D(MinX, MinY);
		Tri1.V0_UV = FVector2D(MinU, MinV);
		Tri1.V1_Pos = FVector2D(MaxX, MinY);
		Tri1.V1_UV = FVector2D(MaxU, MinV);
		Tri1.V2_Pos = FVector2D(MaxX, MaxY);
		Tri1.V2_UV = FVector2D(MaxU, MaxV);

		Tri2.V0_Pos = FVector2D(MaxX, MaxY);
		Tri2.V0_UV = FVector2D(MaxU, MaxV);
		Tri2.V1_Pos = FVector2D(MinX, MaxY);
		Tri2.V1_UV = FVector2D(MinU, MaxV);
		Tri2.V2_Pos = FVector2D(MinX, MinY);
		Tri2.V2_UV = FVector2D(MinU, MinV);
		Tri1.V0_Color = Tri1.V1_Color = Tri1.V2_Color = Tri2.V0_Color = Tri2.V1_Color = Tri2.V2_Color =
			FLinearColor::White;
		TArray<FCanvasUVTri> List;
		List.Add(Tri1);
		List.Add(Tri2);
		FCanvasTriangleItem TriItem(List, TextureResource);
		TriItem.BlendMode = SE_BLEND_Opaque;
		Canvas.DrawItem(TriItem);
	}

	// Tell the rendering thread to draw any remaining batched elements
	Canvas.Flush_GameThread(true);

	ENQUEUE_RENDER_COMMAND(UpdateDrawTextureToRTCommand)
	(
		[RenderTargetResource](FRHICommandListImmediate& RHICmdList)
		{
			// Copy (resolve) the rendered image from the frame buffer to its render target texture
			RHICmdList.CopyToResolveTarget(
				RenderTargetResource->GetRenderTargetTexture(), // Source texture
				RenderTargetResource->TextureRHI, // Dest texture
				FResolveParams()); // Resolve parameters
		});
}

void UTerrainGeneratorHelper::GetLandscapeCenterAndExtent(
	const ALandscape* Landscape,
	FVector& OutCenter,
	FVector& OutExtent)
{
	const FIntRect LandscapeRect = Landscape->GetBoundingRect();
	const FVector MidPoint = FVector(LandscapeRect.Min, 0.f) + FVector(LandscapeRect.Size(), 0.f) * 0.5f;

	OutCenter = Landscape->GetTransform().TransformPosition(MidPoint);
	OutExtent = FVector(LandscapeRect.Size(), 0.f) * Landscape->GetActorScale() * 0.5f;
}

TArray<FLinearColor> UTerrainGeneratorHelper::SampleRTData(UTextureRenderTarget2D* InRenderTarget, FLinearColor InRect)
{
	if (!InRenderTarget)
	{
		FMessageLog("TerrainGeneratorHelper")
			.Warning(LOCTEXT("UTerrainGeneratorHelper", "SampleRTData: Render Target must be non-null."));
		return { FLinearColor(0, 0, 0, 0) };
	}
	if (!InRenderTarget->Resource)
	{
		FMessageLog("TerrainGeneratorHelper")
			.Warning(LOCTEXT("UTerrainGeneratorHelper", "SampleRTData: Render Target has been released."));
		return { FLinearColor(0, 0, 0, 0) };
	}

	FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();

	InRect.R = FMath::Clamp(int(InRect.R), 0, InRenderTarget->SizeX - 1);
	InRect.G = FMath::Clamp(int(InRect.G), 0, InRenderTarget->SizeY - 1);
	InRect.B = FMath::Clamp(int(InRect.B), int(InRect.R + 1), InRenderTarget->SizeX);
	InRect.A = FMath::Clamp(int(InRect.A), int(InRect.G + 1), InRenderTarget->SizeY);
	const FIntRect Rect = FIntRect(InRect.R, InRect.G, InRect.B, InRect.A);
	FReadSurfaceDataFlags ReadPixelFlags(RCM_MinMax);

	ETextureRenderTargetFormat Format = (InRenderTarget->RenderTargetFormat);

	if ((Format == (RTF_RGBA8)) || (Format == RTF_R8) || (Format == RTF_RG8))
	{
		TArray<FColor> OutLDR;
		RTResource->ReadPixels(OutLDR, ReadPixelFlags, Rect);
		TArray<FLinearColor> OutResult;
		OutResult.SetNum(OutLDR.Num());
		for (int32 i = 0; i < OutLDR.Num(); i++)
		{
			OutResult[i] =
				(FLinearColor(float(OutLDR[i].R), float(OutLDR[i].G), float(OutLDR[i].B), float(OutLDR[i].A)) / 255.0f);
		}
		return OutResult;
	}

	if ((Format == RTF_R16f) || (Format == RTF_RG16f) || (Format == RTF_RGBA16f) || (Format == RTF_R32f)
		|| (Format == RTF_RG32f) || (Format == RTF_RGBA32f))
	{
		TArray<FLinearColor> OutResult;
		RTResource->ReadLinearColorPixels(OutResult, ReadPixelFlags, Rect);
		return OutResult;
	}

	FMessageLog("TerrainGeneratorHelper")
		.Warning(LOCTEXT(
			"UTerrainGeneratorHelper",
			"SampleRTData: Currently only 4 channel formats are supported: RTF_RGBA8, RTF_RGBA16f, and RTF_RGBA32f."));

	return { FLinearColor(0, 0, 0, 0) };
}


#undef LOCTEXT_NAMESPACE
