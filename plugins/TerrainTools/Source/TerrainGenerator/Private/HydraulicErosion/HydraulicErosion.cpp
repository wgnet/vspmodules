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


#include "HydraulicErosion.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Engine/CollisionProfile.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Landscape.h"
#include "LevelEditor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "TerrainGeneratorHelper.h"
#include "TerrainGeneratorUtils.h"

namespace TerrainLayerStackLocal
{
	const FStringAssetReference PreviewMesh128Q_AssetReference(
		"StaticMesh'/TerrainTools/HydraulicErosion/Meshes/S_Preview_LS_128x128.S_Preview_LS_128x128'");
	const FStringAssetReference PreviewMesh256Q_AssetReference(
		"StaticMesh'/TerrainTools/HydraulicErosion/Meshes/S_Preview_LS_256x256.S_Preview_LS_256x256'");
	const FStringAssetReference Scatter127Q_AssetReference(
		"StaticMesh'/TerrainTools/HydraulicErosion/Meshes/S_Scatter_127x127_quads.S_Scatter_127x127_quads'");
	const FStringAssetReference Scatter255Q_AssetReference(
		"StaticMesh'/TerrainTools/HydraulicErosion/Meshes/S_Scatter_255x255_quads.S_Scatter_255x255_quads'");
	const FStringAssetReference MPC_Erosion_AssetReference(
		"MaterialParameterCollection'/TerrainTools/HydraulicErosion/MPC/MPC_Erosion.MPC_Erosion'");
	const FStringAssetReference ComputeNormal_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Seed/M_ComputeNormal.M_ComputeNormal'");
	const FStringAssetReference MeshPreview_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Preview/M_MeshPreview.M_MeshPreview'");
	const FStringAssetReference TerrainSeed_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Seed/M_NoiseGeneration.M_NoiseGeneration'");
	const FStringAssetReference UnpackRG8Height_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Seed/M_UnpackRG8Height.M_UnpackRG8Height'");
	const FStringAssetReference NormalizeHeight_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Seed/M_UnpackRG8Height.M_UnpackRG8Height'");
	const FStringAssetReference DebugDrawing_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Debug/M_DebugDrawing.M_DebugDrawing'");
	const FStringAssetReference PrepRTforLS_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Capture/M_PrepRTforLS.M_PrepRTforLS'");
	const FStringAssetReference PrepWMforLS_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Capture/M_PrepWMforLS.M_PrepWMforLS'");

	// By default, the max Z range is 0.65536 kilometers or 65536 centimeters.
	// Since height is stored as an integer with a range from 0 to 65536, this allows one centimeter precision.
	// Higher ranges are possible by sacrificing some precision. Note that the range is split in half so the default 65536 cm is split into -32k to +32k range.
	static constexpr float MaxTerrainZRangeKilometers = 0.65536f;
	static constexpr float MaxTerrainZRangeCentimeters = 65536.f;
	static constexpr uint16 MaxTerrainZRangeVoxels = 512;
	static constexpr int32 TerrainQuadSize = 100; // Terrain Quad = 100 unreal units or 100 centimeters
}

AHydraulicErosion::AHydraulicErosion()
{
	TerrainISMC = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("TerrainISMC"));
	TerrainISMC->SetupAttachment(RootComponent);
	TerrainISMC->CreationMethod = EComponentCreationMethod::Native;

	ParticlesScatterISMC = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ParticlesScatterISMC"));
	ParticlesScatterISMC->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	ParticlesScatterISMC->SetupAttachment(RootComponent);
	ParticlesScatterISMC->CreationMethod = EComponentCreationMethod::Native;

	ScatterSceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("DepthSceneCapture"));
	ScatterSceneCapture->SetupAttachment(RootComponent);
	ScatterSceneCapture->SetRelativeLocation(FVector(0.f, 0.f, 50000.f));
	ScatterSceneCapture->SetRelativeRotation(FRotator(-90.f, 0.f, -90.f));
	ScatterSceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	ScatterSceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	ScatterSceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneColorHDR;
	ScatterSceneCapture->bCaptureEveryFrame = false;
	ScatterSceneCapture->bCaptureOnMovement = false;
	ScatterSceneCapture->CreationMethod = EComponentCreationMethod::Native;
	// General Show Flags
	ScatterSceneCapture->ShowFlags.SetAntiAliasing(false);
	ScatterSceneCapture->ShowFlags.SetAtmosphere(true);
	ScatterSceneCapture->ShowFlags.SetBSP(false);
	ScatterSceneCapture->ShowFlags.SetDecals(false);
	ScatterSceneCapture->ShowFlags.SetFog(false);
	ScatterSceneCapture->ShowFlags.SetLandscape(false);
	ScatterSceneCapture->ShowFlags.SetParticles(false);
	ScatterSceneCapture->ShowFlags.SetSkeletalMeshes(false);
	ScatterSceneCapture->ShowFlags.SetStaticMeshes(true);
	ScatterSceneCapture->ShowFlags.SetTranslucency(true);
	// Advanced Show Flags
	ScatterSceneCapture->ShowFlags.SetDeferredLighting(false);
	ScatterSceneCapture->ShowFlags.SetInstancedFoliage(false);
	ScatterSceneCapture->ShowFlags.SetInstancedGrass(false);
	ScatterSceneCapture->ShowFlags.SetInstancedStaticMeshes(true);
	ScatterSceneCapture->ShowFlags.SetPaper2DSprites(false);
	ScatterSceneCapture->ShowFlags.SetTextRender(false);
	ScatterSceneCapture->ShowFlags.SetTemporalAA(true);
	// Post Processing Show Flags
	ScatterSceneCapture->ShowFlags.SetBloom(false);
	ScatterSceneCapture->ShowFlags.SetEyeAdaptation(false);
	ScatterSceneCapture->ShowFlags.SetMotionBlur(false);
	ScatterSceneCapture->ShowFlags.SetToneCurve(true);
	// LightType Show Flags
	ScatterSceneCapture->ShowFlags.SetSkyLighting(false);
	// Lighting Components Show Flags
	ScatterSceneCapture->ShowFlags.SetAmbientOcclusion(false);
	ScatterSceneCapture->ShowFlags.SetDynamicShadows(false);
	// Lighting Features Show Flags
	ScatterSceneCapture->ShowFlags.SetAmbientCubemap(false);
	ScatterSceneCapture->ShowFlags.SetDistanceFieldAO(false);
	ScatterSceneCapture->ShowFlags.SetLightFunctions(false);
	ScatterSceneCapture->ShowFlags.SetLightShafts(false);
	ScatterSceneCapture->ShowFlags.SetReflectionEnvironment(false);
	ScatterSceneCapture->ShowFlags.SetScreenSpaceReflections(false);
	ScatterSceneCapture->ShowFlags.SetTexturedLightProfiles(false);
	ScatterSceneCapture->ShowFlags.SetVolumetricFog(false);
	// Hiden Show Flags
	ScatterSceneCapture->ShowFlags.SetGame(false);
	ScatterSceneCapture->ShowFlags.SetLighting(false);
	ScatterSceneCapture->ShowFlags.SetPostProcessing(false);

	HydraulicErosion = CreateDefaultSubobject<UHydraulicErosionComponent>(TEXT("HydraulicErosion"));
	HydraulicErosion->CreationMethod = EComponentCreationMethod::Native;
}

void AHydraulicErosion::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (bTickingErosion)
		return;

	HydraulicErosion->HydraulicErosionMode = HydraulicErosionMode;
	ConfigResolution();
	AllocateRenderTargets();
	//ReadLandscapeHeight();
	LoadMaterials();
	CreateMIDs();
	SetMPCParams();

	SetActorLocation(FVector::ZeroVector);

	// Setup ISMC
	if (!PreviewMesh128Q)
	{
		PreviewMesh128Q = CastChecked<UStaticMesh>(TerrainLayerStackLocal::PreviewMesh128Q_AssetReference.TryLoad());
	}
	if (!PreviewMesh256Q)
	{
		PreviewMesh256Q = CastChecked<UStaticMesh>(TerrainLayerStackLocal::PreviewMesh256Q_AssetReference.TryLoad());
	}
	if (!Scatter127Q)
	{
		Scatter127Q = CastChecked<UStaticMesh>(TerrainLayerStackLocal::Scatter127Q_AssetReference.TryLoad());
	}
	if (!Scatter255Q)
	{
		Scatter255Q = CastChecked<UStaticMesh>(TerrainLayerStackLocal::Scatter255Q_AssetReference.TryLoad());
	}

	if (ComponentSize == ESS_127x127_Quads)
	{
		TerrainISMC->SetStaticMesh(PreviewMesh128Q);
		ParticlesScatterISMC->SetStaticMesh(Scatter127Q);
	}

	if (ComponentSize == ESS_255x255_Quads)
	{
		TerrainISMC->SetStaticMesh(PreviewMesh256Q);
		ParticlesScatterISMC->SetStaticMesh(Scatter255Q);
	}

	TerrainISMC->SetMaterial(0, MeshPreviewMID);
	TerrainISMC->SetVisibility(ShowPreviewMesh);
	TerrainISMC->SetForcedLodModel(ForcedLOD);
	ParticlesScatterISMC->SetVisibility(bShowWater);

	HydraulicErosion->Constructor(FHydraulicErosionDada { HeightRTA,
														  HeightRTB,
														  JumpFloodA,
														  JumpFloodB,
														  PositionA,
														  PositionB,
														  MeshDepthRT,
														  NormalRT });

	ScatterSceneCapture->OrthoWidth = TotalSize.GetMax() + SizePerQuad;
	ScatterSceneCapture->TextureTarget = MeshDepthRT;
	ScatterSceneCapture->ClearShowOnlyComponents();
	ScatterSceneCapture->ShowOnlyComponent(ParticlesScatterISMC);
	ScatterSceneCapture->CaptureScene();

	if (CheckSizeParamsChanged())
	{
		SetEditorTickEnabled(true);
		RenderSeed();
		UpdateBrushesAndNormals();
	}
	else
	{
		if (CheckSeedParamsChanged())
		{
			RenderSeed();
			UpdateBrushesAndNormals();
		}
		else
		{
			UpdateNormalAndPreviewRTAssignments(JumpFloodA);
		}
	}

	CacheSizeParams();
	CacheSeedParams();
}

void AHydraulicErosion::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
}

void AHydraulicErosion::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bTickingErosion)
	{
		if (HydraulicErosion->StackHeightIdx < MaxIterations || MaxIterations == 0)
		{
			HydraulicErosion->TickErosion(PerTickIterations);
			Iterations = HydraulicErosion->StackHeightIdx;
		}
	}
	else
	{
		SpawnInstanceMeshes(TerrainISMC);

		if (HydraulicErosionMode == FreeParticles)
		{
			SpawnInstanceMeshes(ParticlesScatterISMC);
		}
		else
		{
			ParticlesScatterISMC->ClearInstances();
		}

		SetEditorTickEnabled(false);
	}
}

void AHydraulicErosion::StarSimulation()
{
	SetEditorTickEnabled(true);
	bTickingErosion = true;
}

void AHydraulicErosion::StopSimulation()
{
	SetEditorTickEnabled(false);
	bTickingErosion = false;
}

void AHydraulicErosion::ResetSimulation()
{
	UpdateHeight();
}

void AHydraulicErosion::WriteLandscapeWeightmap() const
{
	UTerrainGeneratorHelper::TerrainImportWeightmapFromRenderTarget(Landscape.Get(), HeightRTA, LayerWeightName);
}

void AHydraulicErosion::ExportHeightToLandscape()
{
	// Write a temporary copy adjusted into LS range
	PrepRTforLSMID = FTerrainUtils::GetOrCreateTransientMID(PrepRTforLSMID, TEXT("PrepRTforLSMID"), PrepHMforLS);
	PrepRTforLSMID->SetTextureParameterValue("RT", HeightPingPong_Source());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, HeightPingPong_Target(), PrepRTforLSMID);
	UTerrainGeneratorHelper::TerrainImportHeightmapFromRenderTarget(Landscape.Get(), HeightPingPong_Target(), false);
	UKismetRenderingLibrary::ClearRenderTarget2D(this, HeightPingPong_Target());
}

void AHydraulicErosion::ReadLandscapeHeight()
{
	if (Landscape)
	{
		UMaterialParameterCollectionInstance* MaterialParamInstance =
			GetWorld()->GetParameterCollectionInstance(MPC_Erosion);
		check(MaterialParamInstance) MaterialParamInstance->SetScalarParameterValue("ShowProcedural", 0.0f);
		UKismetRenderingLibrary::ClearRenderTarget2D(this, LandscapeHeightRT);
		UTerrainGeneratorHelper::TerrainExportHeightmapToRenderTarget(Landscape.Get(), LandscapeHeightRT, true, true);
		UpdateHeight();
		MaterialParamInstance->SetScalarParameterValue("ShowProcedural", bShowProceduralImpact);
	}
}

void AHydraulicErosion::CopyLandscapeRange()
{
	if (Landscape)
	{
		TotalZRangeKilometers =
			Landscape->GetActorScale().Z / 128.f * TerrainLayerStackLocal::MaxTerrainZRangeKilometers;
	}
}

void AHydraulicErosion::ExportWeightmapsToLandscape()
{
	if (Landscape)
	{
		PrepWeightmapforLSMID =
			FTerrainUtils::GetOrCreateTransientMID(PrepWeightmapforLSMID, TEXT("PrepWeightmapforLSMID"), PrepWMforLS);
		TArray<FName> WMNamesLocal;
		TArray<FLinearColor> MaskChannels {
			FLinearColor(1, 0, 0, 0),
			FLinearColor(0, 1, 0, 0),
			FLinearColor(0, 0, 1, 0),
			FLinearColor(0, 0, 0, 1)
		};

		for (auto& Iter : Weightmaps)
		{
			WMNamesLocal.Empty();

			WMNamesLocal.Add(Iter.Value.R);
			WMNamesLocal.Add(Iter.Value.G);
			WMNamesLocal.Add(Iter.Value.B);
			WMNamesLocal.Add(Iter.Value.A);

			for (int32 i = 0; i < WMNamesLocal.Num(); i++)
			{
				PrepWeightmapforLSMID->SetTextureParameterValue("RT", Iter.Value.RenderTarget);
				PrepWeightmapforLSMID->SetVectorParameterValue("MaskChannel", MaskChannels[i]);
				UKismetRenderingLibrary::ClearRenderTarget2D(this, HeightPingPong_Target());
				UKismetRenderingLibrary::DrawMaterialToRenderTarget(
					this,
					HeightPingPong_Target(),
					PrepWeightmapforLSMID);
				UTerrainGeneratorHelper::TerrainImportWeightmapFromRenderTarget(
					Landscape.Get(),
					HeightPingPong_Target(),
					WMNamesLocal[i]);
				UKismetRenderingLibrary::ClearRenderTarget2D(this, HeightPingPong_Target());
			}
		}
	}
}

void AHydraulicErosion::PerformIterations() const
{
	HydraulicErosion->TickErosionOnce(ButtonPressIterations);
}

void AHydraulicErosion::SpawnInstanceMeshes(UInstancedStaticMeshComponent* ISMC)
{
	ISMC->ClearInstances();
	StoredInstances = SectionsX * SectionsY;

	const float InSectorSize = TotalSize.GetMax() / FMath::Max(SectionsX, SectionsY);
	const FVector InCenterLocation { 0, 0, 0 };
	const float InX = SectionsX;
	const float InY = SectionsY;
	// 2D Grid

	for (int32 i = 0; i < InY; i++)
	{
		for (int32 k = 0; k < InX; k++)
		{
			//Track Rows and Columns
			FVector Offsets = FVector(k - (float(InX) / 2), i - (float(InY) / 2), 0.f);

			FVector PolyCenter;
			PolyCenter = Offsets * InSectorSize;
			PolyCenter += FVector(InSectorSize * 0.5f, InSectorSize * 0.5f, 0.f);
			PolyCenter += InCenterLocation;
			PolyCenter *= FVector { 1, 1, 0 };
			PolyCenter += GetActorLocation();

			FVector PolyScale;
			PolyScale.X = (TotalSize.X / SectionsX) / (ISMC->GetStaticMesh()->GetBounds().BoxExtent.X * 2);
			PolyScale.Y = (TotalSize.Y / SectionsY) / (ISMC->GetStaticMesh()->GetBounds().BoxExtent.Y * 2);
			PolyScale.Z = 1.f;

			ISMC->AddInstanceWorldSpace(FTransform { FRotator::ZeroRotator, PolyCenter, PolyScale });
		}
	}
}

void AHydraulicErosion::ConfigResolution()
{
	SectionsX = FMath::Min(SectionsX, 16);
	SectionsY = FMath::Min(SectionsY, 16);

	int32 QSizeX = 0;
	int32 QSizeY = 0;

	if (ComponentSize == ESectionSizeOptions::ESS_127x127_Quads)
	{
		QSizeX = SectionsX * 127 + 1;
		QSizeY = SectionsY * 127 + 1;
	}
	else if (ComponentSize == ESectionSizeOptions::ESS_255x255_Quads)
	{
		QSizeX = SectionsX * 255 + 1;
		QSizeY = SectionsY * 255 + 1;
	}

	// TODO OverallResolution = bForcePowerOfTwo ? FTerrainUtils::RoundUpPowerOfTwo( FVector2D( SectionsX, SectionsY ) ) : FVector2D( SectionsX, SectionsY );
	if (bForcePowerOfTwo)
	{
		OverallResolution = FTerrainUtils::RoundUpPowerOfTwo(FVector2D(QSizeX, QSizeY));
	}
	else
	{
		OverallResolution = FVector2D(QSizeX, QSizeY);
	}

	if (OverrideSize == 0)
	{
		SizePerQuad = TerrainLayerStackLocal::TerrainQuadSize;
	}
	else
	{
		SizePerQuad = OverrideSize / (FMath::Max(OverallResolution.X, OverallResolution.Y) - 1.f); // Size Per Quad
	}
	// Dont adjust for power of 2
	TotalSize = FVector2D(QSizeX - 1, QSizeY - 1) * SizePerQuad;
	LandscapeActorZScale =
		TerrainLayerStackLocal::MaxTerrainZRangeCentimeters / TerrainLayerStackLocal::MaxTerrainZRangeVoxels;
	RawZScale = TotalZRangeKilometers / TerrainLayerStackLocal::MaxTerrainZRangeKilometers;
}

void AHydraulicErosion::AllocateRenderTargets()
{
	const FIntPoint SizeLocal =
		FIntPoint(FMath::TruncToInt(OverallResolution.X), FMath::TruncToInt(OverallResolution.Y));
	const FLinearColor RG8ClearColor = FLinearColor(0.501961f, 0.f, 0.f, 0.f);

	HeightRTA =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(HeightRTA, TEXT("RenderTargetA"), SizeLocal, RTF_RGBA32f);
	HeightRTB =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(HeightRTB, TEXT("RenderTargetB"), SizeLocal, RTF_RGBA32f);
	NormalRT = FTerrainUtils::GetOrCreateTransientRenderTarget2D(NormalRT, TEXT("Normal"), SizeLocal, RTF_RGBA16f);
	JumpFloodA = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
		JumpFloodA,
		TEXT("JumpFloodTargetA_Vel"),
		SizeLocal,
		RTF_RGBA32f);
	JumpFloodB = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
		JumpFloodB,
		TEXT("JumpFloodTargetB_Vel"),
		SizeLocal,
		RTF_RGBA32f);
	NoiseSeedRT = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
		NoiseSeedRT,
		TEXT("NoiseSeedRT"),
		SizeLocal,
		RTF_RG8,
		RG8ClearColor);
	LandscapeHeightRT = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
		LandscapeHeightRT,
		TEXT("LandscapeHeightRT"),
		SizeLocal,
		RTF_RG8);
	MeshDepthRT =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(MeshDepthRT, TEXT("MeshDepthRT"), SizeLocal, RTF_RGBA32f);

	if (HydraulicErosionMode == FreeParticles)
	{
		PositionA =
			FTerrainUtils::GetOrCreateTransientRenderTarget2D(PositionA, TEXT("PositionA"), SizeLocal, RTF_RGBA32f);
		PositionB =
			FTerrainUtils::GetOrCreateTransientRenderTarget2D(PositionB, TEXT("PositionB"), SizeLocal, RTF_RGBA32f);
	}
	else
	{
		PositionA = nullptr;
		PositionB = nullptr;
	}

	// Allocate User Defined WeightMaps
	for (auto& UserWeightmap : Weightmaps)
	{
		FIntPoint UserWeightmapResolutionLocal = FIntPoint(
			FMath::TruncToInt(OverallResolution.X / UserWeightmap.Value.ResolutionDivisor),
			FMath::TruncToInt(OverallResolution.X / UserWeightmap.Value.ResolutionDivisor));

		UserWeightmap.Value.RenderTarget = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
			PositionA,
			UserWeightmap.Key,
			UserWeightmapResolutionLocal,
			RTF_RGBA16f);
	}
}

void AHydraulicErosion::CreateMIDs()
{
	SeedNoiseMID = FTerrainUtils::GetOrCreateTransientMID(SeedNoiseMID, TEXT("SeedNoiseMID"), TerrainSeedMat);
	SeedNoiseMID->SetVectorParameterValue(
		TEXT("ElevationParams"),
		FLinearColor(ElevationScale, 0.f, ElevationPreOffset, bInvertNoise));
	SeedNoiseMID->SetVectorParameterValue(TEXT("NoiseSeedParams"), FLinearColor(Tiling, Seed, Octaves, Persistence));
	SeedNoiseMID->SetVectorParameterValue(TEXT("TestFeatureParams"), FLinearColor(Ramp, Plateau, Channel, EdgeLip));

	MeshPreviewMID = FTerrainUtils::GetOrCreateTransientMID(MeshPreviewMID, TEXT("MeshPreviewMID"), MeshPreviewMat);
	MeshPreviewMID->SetTextureParameterValue(TEXT("NormalRT"), NormalRT);
	MeshPreviewMID->SetTextureParameterValue(TEXT("SceneCaptureRT"), MeshDepthRT);

	// TODO User Defined WeightMaps
	for (auto& Iter : Weightmaps)
	{
		if (IsValid(Iter.Value.RenderTarget))
		{
			MeshPreviewMID->SetTextureParameterValue(Iter.Key, Iter.Value.RenderTarget);
		}
	}

	ComputeNormalMID =
		FTerrainUtils::GetOrCreateTransientMID(ComputeNormalMID, TEXT("ComputeNormalMID"), ComputeNormalMat);

	CombineLandscapeNoiseWaterMID = FTerrainUtils::GetOrCreateTransientMID(
		CombineLandscapeNoiseWaterMID,
		TEXT("CombineLandscapeNoiseWaterMID"),
		UnpackRG8HeightMat);
	CombineLandscapeNoiseWaterMID->SetTextureParameterValue(TEXT("SeedRT"), NoiseSeedRT);
	CombineLandscapeNoiseWaterMID->SetTextureParameterValue(TEXT("LandscapeRT"), LandscapeHeightRT);

	DebugDrawMID = FTerrainUtils::GetOrCreateTransientMID(DebugDrawMID, TEXT("DebugDrawMID"), DebugDrawingMat);

	NormalizeHeightMID =
		FTerrainUtils::GetOrCreateTransientMID(NormalizeHeightMID, TEXT("NormalizeHeightMID"), NormalizeHeightMat);

	// TODO Add Proto Water System Hookup Here
}


void AHydraulicErosion::SetMPCParams()
{
	MPC_Erosion = (UMaterialParameterCollection*)TerrainLayerStackLocal::MPC_Erosion_AssetReference.TryLoad();
	check(MPC_Erosion) UMaterialParameterCollectionInstance* MaterialParamInstance =
		GetWorld()->GetParameterCollectionInstance(MPC_Erosion);
	check(MaterialParamInstance) MaterialParamInstance->SetScalarParameterValue(TEXT("ResX"), OverallResolution.X);
	MaterialParamInstance->SetScalarParameterValue(TEXT("ResY"), OverallResolution.Y);
	MaterialParamInstance->SetScalarParameterValue(TEXT("RainAmount"), RainAmount);
	MaterialParamInstance->SetScalarParameterValue(TEXT("Solubility"), Solubility);
	MaterialParamInstance->SetScalarParameterValue(TEXT("Evaporation"), Evaporation);
	MaterialParamInstance->SetScalarParameterValue(TEXT("deltaT"), DeltaT);
	MaterialParamInstance->SetScalarParameterValue(TEXT("StartWater"), StartWater);
	MaterialParamInstance->SetScalarParameterValue(TEXT("GridSize"), TotalSize.X / (OverallResolution.X - 1));
	MaterialParamInstance->SetScalarParameterValue(
		TEXT("SlopeSmoothThreshold"),
		FMath::IsNearlyEqual(SmoothSlopesGreaterThan, 0.f) ? 999.f : SmoothSlopesGreaterThan);
	// TODO MaterialParamInstance->SetScalarParameterValue( TEXT("BlurAmount"),  );
	MaterialParamInstance->SetScalarParameterValue(TEXT("NormalAdvection"), NormalAdvectionErosion);
	MaterialParamInstance->SetScalarParameterValue(TEXT("AdvectionErosion"), VelocityAdvectionErosion);
	MaterialParamInstance->SetScalarParameterValue(
		TEXT("MaxAdvectSlope"),
		FMath::IsNearlyEqual(MaxAdvectionSlope, 0.f) ? 999.f : MaxAdvectionSlope);
	MaterialParamInstance->SetScalarParameterValue(
		TEXT("ShowWater"),
		(HydraulicErosionMode != FreeParticles && bShowWater));
	MaterialParamInstance->SetScalarParameterValue(TEXT("VelocityDisplay"), VelocityDebugBrightness);
	MaterialParamInstance->SetScalarParameterValue(TEXT("ShowProcedural"), bShowProceduralImpact);
	MaterialParamInstance->SetScalarParameterValue(TEXT("TotalSizeX"), TotalSize.X);
	MaterialParamInstance->SetScalarParameterValue(TEXT("TotalSizeY"), TotalSize.Y);
	MaterialParamInstance->SetScalarParameterValue(TEXT("ShowComponents"), bHighlightComponents);

	if (ComponentSize == ESS_127x127_Quads)
	{
		MaterialParamInstance->SetScalarParameterValue(TEXT("ComponentRes"), 128.f);
	}

	if (ComponentSize == ESS_255x255_Quads)
	{
		MaterialParamInstance->SetScalarParameterValue(TEXT("ComponentRes"), 256.f);
	}

	MaterialParamInstance->SetScalarParameterValue(TEXT("VelocityDamping"), VelocityDamping);
	// TODO MaterialParamInstance->SetScalarParameterValue( TEXT("idx"), );
	MaterialParamInstance->SetScalarParameterValue(TEXT("ZScale"), RawZScale);
	MaterialParamInstance->SetScalarParameterValue(
		TEXT("FlatSmoothThreshold"),
		FMath::IsNearlyEqual(SmoothSlopesLessThan, 0.f) ? 0.f : SmoothSlopesLessThan);
	MaterialParamInstance->SetScalarParameterValue(TEXT("UseWaterLevel"), bWaterIsLevel);
}

void AHydraulicErosion::LoadMaterials()
{
	ComputeNormalMat = Cast<UMaterialInterface>(TerrainLayerStackLocal::ComputeNormal_MaterialReference.TryLoad());
	MeshPreviewMat = Cast<UMaterialInterface>(TerrainLayerStackLocal::MeshPreview_MaterialReference.TryLoad());
	TerrainSeedMat = Cast<UMaterialInterface>(TerrainLayerStackLocal::TerrainSeed_MaterialReference.TryLoad());
	UnpackRG8HeightMat = Cast<UMaterialInterface>(TerrainLayerStackLocal::UnpackRG8Height_MaterialReference.TryLoad());
	DebugDrawingMat = Cast<UMaterialInterface>(TerrainLayerStackLocal::DebugDrawing_MaterialReference.TryLoad());
	NormalizeHeightMat = Cast<UMaterialInterface>(TerrainLayerStackLocal::NormalizeHeight_MaterialReference.TryLoad());
	PrepHMforLS = Cast<UMaterialInterface>(TerrainLayerStackLocal::PrepRTforLS_MaterialReference.TryLoad());
	PrepWMforLS = Cast<UMaterialInterface>(TerrainLayerStackLocal::PrepWMforLS_MaterialReference.TryLoad());
}

void AHydraulicErosion::UpdateNormalAndPreviewRTAssignments(UTextureRenderTarget2D* InVelocityRT)
{
	if (InVelocityRT)
	{
		MeshPreviewMID->SetTextureParameterValue("VelocityRT", InVelocityRT);
	}

	MeshPreviewMID->SetTextureParameterValue("HeightRT", HeightPingPong_Source());
	ComputeNormalMID->SetTextureParameterValue("HeightRT", HeightPingPong_Source());

	UKismetRenderingLibrary::ClearRenderTarget2D(this, NormalRT);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, NormalRT, ComputeNormalMID);

	// Use Landscape for Debug
	if (Landscape)
	{
		Landscape->SetLandscapeMaterialTextureParameterValue("StackRT", HeightPingPong_Source());
		Landscape->SetLandscapeMaterialTextureParameterValue("LandscapeRT", LandscapeHeightRT);
		Landscape->SetLandscapeMaterialTextureParameterValue("NormalRT", NormalRT);
	}
}

bool AHydraulicErosion::CheckSizeParamsChanged() const
{
	return (
		(!UKismetMathLibrary::EqualEqual_Vector2DVector2D(CachedOverallResolution, OverallResolution))
		|| (!UKismetMathLibrary::EqualEqual_IntInt(CachedOverrideSize, OverrideSize))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(TotalZRangeKilometers, CachedZRangeKilometers, 0.001f))
		|| (!UKismetMathLibrary::EqualEqual_VectorVector(CachedPosition, GetActorLocation(), 16.f))
		|| (!UKismetMathLibrary::EqualEqual_IntInt(StoredInstances, SectionsX * SectionsY))
		|| (!(HydraulicErosionMode == CachedErosionMode)) || (!(SectionsX * SectionsY == StoredInstances)));
}

bool AHydraulicErosion::CheckSeedParamsChanged() const
{
	return (
		(!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedElevationScale, ElevationScale, 10.f))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedElevationPreOffset, ElevationPreOffset, 0.02f))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedTiling, Tiling, 0.02f))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedSeed, Seed, 0.01f))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedRamp, Ramp, 10.f))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedPlateau, Plateau, 10.f))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedChannel, Channel, 10.f))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedStartWater, StartWater, 1.f))
		|| (!UKismetMathLibrary::NearlyEqual_FloatFloat(CachedPersistence, Persistence, 0.001f))
		|| (!UKismetMathLibrary::EqualEqual_BoolBool(bCachedInvert, bInvertNoise))
		|| (!UKismetMathLibrary::EqualEqual_IntInt(CachedOctaves, Octaves)));
}

void AHydraulicErosion::RenderSeed()
{
	UKismetRenderingLibrary::ClearRenderTarget2D(this, NoiseSeedRT);
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, NoiseSeedRT, SeedNoiseMID);
}

void AHydraulicErosion::UpdateBrushesAndNormals()
{
	// Clear Render Targets
	UKismetRenderingLibrary::ClearRenderTarget2D(this, HeightPingPong_Source());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, HeightPingPong_Target());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, JumpFloodA, FLinearColor::Transparent);
	UKismetRenderingLibrary::ClearRenderTarget2D(this, JumpFloodB, FLinearColor::Transparent);
	UKismetRenderingLibrary::ClearRenderTarget2D(this, MeshDepthRT);

	// Reset Simulation Index
	HydraulicErosion->StackHeightIdx = 0;
	HydraulicErosion->PositionIdx = 0;
	HydraulicErosion->VelocityIdx = 0;

	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, HeightPingPong_Source(), CombineLandscapeNoiseWaterMID);

	for (auto& Iter : Weightmaps)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, Iter.Value.RenderTarget);
	}

	UpdateNormalAndPreviewRTAssignments(nullptr);

	if (HydraulicErosionMode == FreeParticles)
	{
		HydraulicErosion->SeedParticlePositions();
		UKismetRenderingLibrary::ClearRenderTarget2D(this, JumpFloodA, FLinearColor::Transparent);
		UKismetRenderingLibrary::ClearRenderTarget2D(this, JumpFloodB, FLinearColor::Transparent);
	}
}

void AHydraulicErosion::UpdateHeight()
{
	RenderSeed();
	UpdateBrushesAndNormals();
}

UTextureRenderTarget2D* AHydraulicErosion::HeightPingPong_Source() const
{
	if (HydraulicErosion->StackHeightIdx % 2)
		return HeightRTB;
	else
		return HeightRTA;
}

UTextureRenderTarget2D* AHydraulicErosion::HeightPingPong_Target() const
{
	if ((HydraulicErosion->StackHeightIdx + 1) % 2)
		return HeightRTB;
	else
		return HeightRTA;
}

void AHydraulicErosion::CacheSizeParams()
{
	CachedOverallResolution = OverallResolution;
	CachedOverrideSize = OverrideSize;
	CachedErosionMode = HydraulicErosionMode;
	CachedZRangeKilometers = TotalZRangeKilometers;
	CachedPosition = GetActorLocation();
}

void AHydraulicErosion::CacheSeedParams()
{
	CachedElevationScale = ElevationScale;
	CachedElevationPreOffset = ElevationPreOffset;
	CachedTiling = Tiling;
	CachedSeed = Seed;
	CachedRamp = Ramp;
	CachedPlateau = Plateau;
	CachedChannel = Channel;
	CachedStartWater = StartWater;
	bCachedInvert = bInvertNoise;
	CachedOctaves = Octaves;
	CachedPersistence = Persistence;
}

FVector AHydraulicErosion::GetLandscapeCenter() const
{
	if (Landscape)
	{
		TArray<ULandscapeComponent*> LandscapeComponentsLocal;
		for (TObjectIterator<ULandscapeComponent> ComponentIt; ComponentIt; ++ComponentIt)
		{
			LandscapeComponentsLocal.Add(*ComponentIt);
		}

		const FVector TerrainExtent = (LandscapeComponentsLocal[0]->Bounds.BoxExtent * LandscapeComponentsLocal.Num())
			/ FMath::Sqrt(LandscapeComponentsLocal.Num());
		const FVector TerrainOrigin = Landscape->GetActorLocation() + TerrainExtent;
		return TerrainOrigin;
	}
	return FVector::ZeroVector;
}
