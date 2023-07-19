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

#include "WeightmapManager.h"

#include "Async/ParallelFor.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Generators/TGBaseLayer.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "LandscapeEdit.h"
#include "LandscapeStreamingProxy.h"
#include "Misc/FeedbackContext.h"
#include "Rendering/Texture2DResource.h"
#include "TerrainGeneratorHelper.h"
#include "TerrainGeneratorUtils.h"


AWeightmapManager::AWeightmapManager()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickGroup = TG_DuringPhysics;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SceneCapture = CreateDefaultSubobject<USceneCaptureComponent2D>("SceneCapture");
	SceneCapture->SetupAttachment(RootComponent);
	SceneCapture->SetRelativeRotation(FRotator { -90, 0, -90 });
	SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCapture->bCaptureEveryFrame = false;
	SceneCapture->bCaptureOnMovement = false;
	SceneCapture->CaptureSortPriority = MAX_int32;
	SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;
	SceneCapture->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	SceneCapture->MaxViewDistanceOverride = 1.f;
	SceneCapture->LODDistanceFactor = 0.f;
	// General Show Flags
	SceneCapture->ShowFlags.SetAntiAliasing(false);
	SceneCapture->ShowFlags.SetAtmosphere(false);
	SceneCapture->ShowFlags.SetBSP(false);
	SceneCapture->ShowFlags.SetDecals(false);
	SceneCapture->ShowFlags.SetFog(false);
	SceneCapture->ShowFlags.SetLandscape(false);
	SceneCapture->ShowFlags.SetParticles(false);
	SceneCapture->ShowFlags.SetSkeletalMeshes(true);
	SceneCapture->ShowFlags.SetStaticMeshes(true);
	SceneCapture->ShowFlags.SetTranslucency(true);
	// Advanced Show Flags
	SceneCapture->ShowFlags.SetDeferredLighting(false);
	SceneCapture->ShowFlags.SetInstancedFoliage(false);
	SceneCapture->ShowFlags.SetInstancedGrass(false);
	SceneCapture->ShowFlags.SetInstancedStaticMeshes(true);
	SceneCapture->ShowFlags.SetPaper2DSprites(false);
	SceneCapture->ShowFlags.SetTextRender(false);
	SceneCapture->ShowFlags.SetTemporalAA(true);
	// Post Processing Show Flags
	SceneCapture->ShowFlags.SetBloom(false);
	SceneCapture->ShowFlags.SetEyeAdaptation(false);
	SceneCapture->ShowFlags.SetMotionBlur(false);
	SceneCapture->ShowFlags.SetToneCurve(false);
	// LightType Show Flags
	SceneCapture->ShowFlags.SetSkyLighting(false);
	// Lighting Components Show Flags
	SceneCapture->ShowFlags.SetAmbientOcclusion(false);
	SceneCapture->ShowFlags.SetDynamicShadows(false);
	// Lighting Features Show Flags
	SceneCapture->ShowFlags.SetAmbientCubemap(false);
	SceneCapture->ShowFlags.SetDistanceFieldAO(false);
	SceneCapture->ShowFlags.SetLightFunctions(false);
	SceneCapture->ShowFlags.SetLightShafts(false);
	SceneCapture->ShowFlags.SetReflectionEnvironment(false);
	SceneCapture->ShowFlags.SetScreenSpaceReflections(false);
	SceneCapture->ShowFlags.SetTexturedLightProfiles(false);
	SceneCapture->ShowFlags.SetVolumetricFog(false);
	// Hiden Show Flags
	SceneCapture->ShowFlags.SetGame(true);
	SceneCapture->ShowFlags.SetLighting(false);
	SceneCapture->ShowFlags.SetPostProcessing(false);
	SceneCapture->ShowFlags.SetVirtualTexturePrimitives(true);
}


void AWeightmapManager::Generate()
{
	if (!Landscape.Get())
		return;

	InitializeManager();

	if (bExportNormals)
	{
		PackedHeightRT = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
			PackedHeightRT,
			"HeightmapRT",
			Resolution,
			ETextureRenderTargetFormat::RTF_RGBA8);
		UTerrainGeneratorHelper::TerrainExportHeightmapToRenderTarget(Landscape.Get(), PackedHeightRT, true, true);
		DecodedHeight = FHeightmapUtils::UnpackHeightToFloat(PackedHeightRT);
	}
	else
	{
		GetPackedHeightRT(PackedHeightRT);
		DecodedHeight = GetUnpackedHeight();
	}

	LayerStack.Empty();
	Resolution = FIntPoint { PackedHeightRT->SizeX, PackedHeightRT->SizeY };
	const bool bHasLayersContent = Landscape->HasLayersContent();

	for (auto Iter : Generators)
	{
		if (IsValid(Iter.Value))
		{
			const FTGTerrainInfo TerrainInfo { UpdateObjectsDepth(Iter.Key), DecodedHeight, Resolution, false };

			Iter.Value->AffectedLayer = Iter.Key;
			UTextureRenderTarget2D* GeneratedData = Iter.Value->Generate(TerrainInfo);
			if (GeneratedData)
				LayerStack.FindOrAdd(Iter.Key, GeneratedData);
		}
	}

	if (bComposite)
	{
		Composite();
	}

	EditingLayerName = GetBaseLayerName();

	for (TTuple<FName, UTextureRenderTarget2D*>& Iter : LayerStack)
	{
		if (Landscape->HasLayersContent())
		{
			UTerrainGeneratorHelper::TerrainImportWeightmapFromRenderTargetToEditingLayer(
				Landscape.Get(),
				Iter.Value,
				EditingLayerName.ToString(),
				Iter.Key.ToString());
		}
		else
		{
			UTerrainGeneratorHelper::TerrainImportWeightmapFromRenderTarget(Landscape.Get(), Iter.Value, Iter.Key);
		}
	}

	if (bCleanMemory)
	{
		PackedHeightRT = nullptr;
		ObjectDepth = nullptr;
		TerrainDepth = nullptr;
		DecodedHeight.Empty();
		ObjectsDepth.Empty();
	}

	Modify();
	PostEditChange();

	if (GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("r.VT.Flush"));
	}
}


void AWeightmapManager::Reapply()
{
	for (const TTuple<FName, UTextureRenderTarget2D*>& Iter : LayerStack)
	{
		UTerrainGeneratorHelper::TerrainImportWeightmapFromRenderTarget(Landscape.Get(), Iter.Value, Iter.Key);
	}

	Modify();
	PostEditChange();

	if (GEngine)
	{
		GEngine->Exec(GetWorld(), TEXT("r.VT.Flush"));
	}
}

void AWeightmapManager::InitializeManager()
{
	if (Landscape.IsValid())
	{
		EditingLayerName = GetBaseLayerName();
		Resolution = Landscape->GetBoundingRect().Max + 1;
	}
}


void AWeightmapManager::GetPackedHeightRT(UTextureRenderTarget2D*& InRenderTarget)
{
	TArray<uint16> HeightData = GetPackedHeight();
	TArray<uint8> PackedHeightData;
	PackedHeightData.SetNum(HeightData.Num() * 4);
	for (int32 i = 0; i < HeightData.Num(); i++)
	{
		PackedHeightData[i * 4 + 0] = MIN_uint8; // B
		PackedHeightData[i * 4 + 1] = HeightData[i] & 0xFF; // G
		PackedHeightData[i * 4 + 2] = HeightData[i] >> 8; // R
		PackedHeightData[i * 4 + 3] = MIN_uint8; // A
	}
	InRenderTarget =
		FTerrainUtils::GetOrCreateTransientRenderTarget2D(InRenderTarget, TEXT("Heightmap"), Resolution, RTF_RGBA8);
	FTextureRenderTargetResource* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource();

	ENQUEUE_RENDER_COMMAND(UpdateTexture2D)
	(
		[&RenderTargetResource, Data = MoveTemp(PackedHeightData)](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.UpdateTexture2D(
				RenderTargetResource->GetTexture2DRHI(),
				0,
				FUpdateTextureRegion2D(0, 0, 0, 0, RenderTargetResource->GetSizeX(), RenderTargetResource->GetSizeY()),
				RenderTargetResource->GetSizeX() * sizeof(FColor),
				Data.GetData());
		});

	FlushRenderingCommands();
}


TArray<uint8> AWeightmapManager::UpdateObjectsDepth(FName Tag)
{
	if (Landscape.Get())
	{
		TArray<TWeakObjectPtr<ULandscapeComponent>> LandscapeComponentsLocal;
		if (bStreamingProxy)
		{
			for (TObjectIterator<ALandscapeStreamingProxy> It; It; ++It)
			{
				LandscapeComponentsLocal.Append(It->LandscapeComponents);
			}
		}
		else
		{
			for (TObjectIterator<ULandscapeComponent> ComponentIt; ComponentIt; ++ComponentIt)
			{
				LandscapeComponentsLocal.Add(*ComponentIt);
			}
		}

		if (LandscapeComponentsLocal.Num() > 0)
		{
			UpdateSceneCapture();
			SceneCapture->ShowOnlyActors.Empty();
			SceneCapture->ClearShowOnlyComponents();
			UGameplayStatics::GetAllActorsWithTag(this, Tag, SceneCapture->ShowOnlyActors);

			if (SceneCapture->ShowOnlyActors.Num() > 0)
			{
				TArray<AActor*> RVTActorsTemp;
				for (AActor* Iter : SceneCapture->ShowOnlyActors)
				{
					if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Iter))
					{
						if (StaticMeshActor->GetStaticMeshComponent()->VirtualTextureRenderPassType
							== ERuntimeVirtualTextureMainPassType::Never)
						{
							StaticMeshActor->GetStaticMeshComponent()->VirtualTextureRenderPassType =
								ERuntimeVirtualTextureMainPassType::Always;
							StaticMeshActor->GetStaticMeshComponent()->MarkRenderStateDirty();
							RVTActorsTemp.Add(StaticMeshActor);
						}
					}
				}
				ETextureRenderTargetFormat Format = RTF_R16f;
				// Capture Objects
				ObjectDepth = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
					ObjectDepth,
					TEXT("ObjectDepth"),
					Resolution,
					Format);
				SceneCapture->TextureTarget = ObjectDepth;
				SceneCapture->CaptureScene();

				for (AActor* Iter : RVTActorsTemp)
				{
					if (AStaticMeshActor* StaticMeshActor = Cast<AStaticMeshActor>(Iter))
					{
						StaticMeshActor->GetStaticMeshComponent()->VirtualTextureRenderPassType =
							ERuntimeVirtualTextureMainPassType::Never;
						StaticMeshActor->GetStaticMeshComponent()->MarkRenderStateDirty();
					}
				}

				// Capture Landscape
				SceneCapture->ShowOnlyActors = { Landscape.Get() };
				SceneCapture->ClearShowOnlyComponents();

				for (TWeakObjectPtr<ULandscapeComponent> Iter : LandscapeComponentsLocal)
				{
					SceneCapture->ShowOnlyComponents.Add(Iter);
				}

				TerrainDepth = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
					TerrainDepth,
					TEXT("TerrainDepth"),
					Resolution,
					Format);
				SceneCapture->TextureTarget = TerrainDepth;
				SceneCapture->CaptureScene();

				FlushRenderingCommands();

				// Extract Mask from depths
				auto GetPixels = [](UTextureRenderTarget2D* InDepth)
				{
					const FTextureRenderTargetResource* RenderTargetResource =
						InDepth->GameThread_GetRenderTargetResource();
					const FLinearColor Rect =
						FLinearColor(0, 0, RenderTargetResource->GetSizeX(), RenderTargetResource->GetSizeY());
					return UTerrainGeneratorHelper::SampleRTData(InDepth, Rect);
				};

				TArray<FLinearColor> TerrainDepthPixels = GetPixels(TerrainDepth);
				TArray<FLinearColor> ObjectsDepthPixels = GetPixels(ObjectDepth);

				TArray<uint8> Data;
				Data.SetNum(TerrainDepthPixels.Num());
				for (int32 Iter = 0; Iter < Data.Num(); Iter++)
				{
					Data[Iter] = (TerrainDepthPixels[Iter].R - ObjectsDepthPixels[Iter].R) /* - CaptureLocation.Z*/ > 0
						? 255
						: 0;
				}

				return Data;
			}
		}
	}
	return TArray<uint8> {};
}

void AWeightmapManager::ExportOnDisk()
{
	for (auto Iter : LayerStack)
	{
		if (IsValid(Iter.Value))
		{
			if (Iter.Value->GetFormat() == EPixelFormat::PF_B8G8R8A8)
			{
				UKismetRenderingLibrary::ExportRenderTarget(
					this,
					Iter.Value,
					ExportDirectory.Path,
					Iter.Key.ToString() + ".PNG");
			}
			if (Iter.Value->GetFormat() == EPixelFormat::PF_G8)
			{ // TODO put in a separate function
				FLinearColor SampleRect = FLinearColor(0, 0, Iter.Value->SizeX, Iter.Value->SizeY);
				TArray<FLinearColor> RTData = UTerrainGeneratorHelper::SampleRTData(Iter.Value, SampleRect);
				UTextureRenderTarget2D* RenderTarget =
					FTerrainUtils::GetOrCreateTransientRenderTarget2D(nullptr, "RT", Resolution, RTF_RGBA8);
				FTextureRenderTargetResource* RTResourceDest = RenderTarget->GameThread_GetRenderTargetResource();

				TArray<uint8> NewData;
				NewData.SetNum(RTData.Num() * 4);
				for (int32 i = 0; i < RTData.Num(); i++)
				{
					NewData[i * 4 + 0] = RTData[i].ToFColor(true).B; // B
					NewData[i * 4 + 1] = RTData[i].ToFColor(true).G; // G
					NewData[i * 4 + 2] = RTData[i].ToFColor(true).R; // R
					NewData[i * 4 + 3] = RTData[i].ToFColor(true).A; // A
				}

				ENQUEUE_RENDER_COMMAND(UpdateTexture2D)
				(
					[&RTResourceDest, Data = MoveTemp(NewData)](FRHICommandListImmediate& RHICmdList)
					{
						RHICmdList.UpdateTexture2D(
							RTResourceDest->GetTexture2DRHI(),
							0,
							FUpdateTextureRegion2D(0, 0, 0, 0, RTResourceDest->GetSizeX(), RTResourceDest->GetSizeY()),
							RTResourceDest->GetSizeX() * sizeof(FColor),
							Data.GetData());
					});

				FlushRenderingCommands();

				UKismetRenderingLibrary::ExportRenderTarget(
					this,
					RenderTarget,
					ExportDirectory.Path,
					Iter.Key.ToString() + ".PNG");
			}
		}
	}
}

void AWeightmapManager::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.MemberProperty->GetFName() == GET_MEMBER_NAME_CHECKED(AWeightmapManager, Generators))
	{
	}
}

void AWeightmapManager::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateSceneCapture();
}

void AWeightmapManager::UpdateSceneCapture() const
{
	if (Landscape.Get())
	{
		const FVector TerrainExtent =
			FVector(Resolution.X, Resolution.Y, Resolution.GetMax()) * Landscape.Get()->GetActorScale();
		const FVector TerrainOrigin = Landscape.Get()->GetActorLocation() + (TerrainExtent / 2.f);
		const FVector CaptureLocation = FVector { TerrainOrigin.X, TerrainOrigin.Y, MAX_uint16 };
		SceneCapture->SetWorldLocation(CaptureLocation);
		SceneCapture->OrthoWidth = FMath::Max(TerrainExtent.X, TerrainExtent.Y);
	}
}


UTextureRenderTarget2D* AWeightmapManager::SubtractWeightmap(
	UTextureRenderTarget2D* InRenderTargetA,
	UTextureRenderTarget2D* InRenderTargetB)
{
	auto GetPixels = [](UTextureRenderTarget2D* RenderTarget)
	{
		const FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
		const FLinearColor Rect =
			FLinearColor(0, 0, RenderTargetResource->GetSizeX(), RenderTargetResource->GetSizeY());
		return UTerrainGeneratorHelper::SampleRTData(RenderTarget, Rect);
	};

	TArray<FLinearColor> SourceDataA = GetPixels(InRenderTargetA);
	TArray<FLinearColor> SourceDataB = GetPixels(InRenderTargetB);

	TArray<uint8> NewData;
	NewData.SetNum(SourceDataA.Num() * 4);
	for (int32 i = 0; i < SourceDataA.Num(); i++)
	{
		const FColor NewColor = (SourceDataA[i] - SourceDataB[i]).ToFColor(false);
		NewData[i * 4 + 0] = NewColor.B;
		NewData[i * 4 + 1] = NewColor.G;
		NewData[i * 4 + 2] = NewColor.R;
		NewData[i * 4 + 3] = NewColor.A;
	}

	UTextureRenderTarget2D* NewWeightmap = UKismetRenderingLibrary::CreateRenderTarget2D(
		this,
		InRenderTargetA->SizeX,
		InRenderTargetA->SizeY,
		InRenderTargetA->RenderTargetFormat);
	FTextureRenderTargetResource* RTResourceDest = NewWeightmap->GameThread_GetRenderTargetResource();

	ENQUEUE_RENDER_COMMAND(UpdateTexture2D)
	(
		[RTResourceDest, Data = MoveTemp(NewData)](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.UpdateTexture2D(
				RTResourceDest->GetTexture2DRHI(),
				0,
				FUpdateTextureRegion2D(0, 0, 0, 0, RTResourceDest->GetSizeX(), RTResourceDest->GetSizeY()),
				RTResourceDest->GetSizeX() * sizeof(FColor),
				Data.GetData());
		});

	return NewWeightmap;
}


UTexture2D* AWeightmapManager::GenerateWeightmapTexture(UTextureRenderTarget2D* InRenderTarget)
{
	FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();
	const FLinearColor Rect = FLinearColor(0, 0, RTResource->GetSizeX(), RTResource->GetSizeY());
	TArray<FLinearColor> SampledData = UTerrainGeneratorHelper::SampleRTData(InRenderTarget, Rect);

	TArray<FColor> NewData;
	for (FLinearColor& Iter : SampledData)
	{
		NewData.Add(Iter.ToFColor(false));
	}

	UTexture2D* OutTexture = UTerrainGeneratorHelper::CreateTextureFromBGRA(
		NewData.GetData(),
		RTResource->GetSizeX(),
		RTResource->GetSizeY());
	return OutTexture;
}


void AWeightmapManager::Composite()
{
	GWarn->BeginSlowTask(FText::FromString(TEXT("Composite")), true, false);
	const int32 TotalProgress = LayerStack.Num();
	int32 Progress = 0;

	for (auto& IterX : LayerStack)
	{
		for (auto& IterY : LayerStack)
		{
			if (IterY.Key != IterX.Key)
			{
				IterX.Value = SubtractWeightmap(IterX.Value, IterY.Value);
			}
			GWarn->UpdateProgress(Progress++, TotalProgress);
		}
	}
	FlushRenderingCommands();
	GWarn->EndSlowTask();
}


TArray<ULandscapeComponent*> AWeightmapManager::GetLandscapeComponents() const
{
	TArray<ULandscapeComponent*> LandscapeComponentsLocal;
	if (bStreamingProxy)
	{
		for (TObjectIterator<ULandscapeComponent> ComponentIt; ComponentIt; ++ComponentIt)
		{
			LandscapeComponentsLocal.Add(*ComponentIt);
		}
	}
	else
	{
		for (TObjectIterator<ALandscapeStreamingProxy> It; It; ++It)
		{
			LandscapeComponentsLocal.Append(It->LandscapeComponents);
		}
	}

	return LandscapeComponentsLocal;
}




TArray<uint16> AWeightmapManager::GetPackedHeight() const
{
	const int32 MinX = 0;
	const int32 MinY = 0;
	const int32 MaxX = Resolution.X - 1;
	const int32 MaxY = Resolution.Y - 1;
	const int32 Stride = 0;
	const int32 XSize = Resolution.X;
	const int32 YSize = Resolution.Y;

	TArray<uint16> HeightData;
	HeightData.SetNum(XSize * YSize);

	FLandscapeEditDataInterface LandscapeEdit(Landscape.Get()->GetLandscapeInfo());
	LandscapeEdit.GetHeightDataFast(MinX, MinY, MaxX, MaxY, HeightData.GetData(), Stride);
	return HeightData;
}

TArray<float> AWeightmapManager::GetUnpackedHeight() const
{
	return FHeightmapUtils::UnpackHeightToFloat(GetPackedHeight());
}

FName AWeightmapManager::GetBaseLayerName() const
{
	if (Landscape.IsValid() && Landscape->HasLayersContent())
	{
		if (Landscape->LandscapeLayers.IsValidIndex(0))
			return Landscape->LandscapeLayers[0].Name;
	}
	return FName {};
}
