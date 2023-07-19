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


#include "HydraulicErosionComponent.h"

#include "HydraulicErosion.h"
#include "TerrainGeneratorUtils.h"

#include "Components/SceneCaptureComponent2D.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetStringLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"

namespace HydraulicErosionLocal
{
	const FSoftObjectPath GravAcceleration_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Advect/M_GravAcceleration.M_GravAcceleration'");
	const FSoftObjectPath ShallowWaterAdvectHeight_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Advect/M_Advect_Height.M_Advect_Height'");
	const FSoftObjectPath ShallowWaterAdvectVelocity_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Advect/M_Advect_Velocity.M_Advect_Velocity'");
	const FSoftObjectPath CalcDivergence_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Advect/M_CalcDivergence.M_CalcDivergence'");
	const FSoftObjectPath ScatterGatherVelocity_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/ScatterGather/M_ScatterVel.M_ScatterVel'");
	const FSoftObjectPath GatherData_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/ScatterGather/M_GatherData.M_GatherData'");
	const FSoftObjectPath GatherVelocity_MaterialReferencel(
		"Material'/TerrainTools/HydraulicErosion/Materials/ScatterGather/M_GatherVel.M_GatherVel'");
	const FSoftObjectPath WaterLevel_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Levelling/M_WaterLevel.M_WaterLevel'");
	const FSoftObjectPath ErosionWeightmap_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Brushes/M_ErosionWeightmap.M_ErosionWeightmap'");
	const FSoftObjectPath ParticlePreview_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Scatter/M_ParticlePreview.M_ParticlePreview'");
	const FSoftObjectPath ParticleRender_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Scatter/M_ParticleRender.M_ParticleRender'");
	const FSoftObjectPath ScatterVelocity_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Scatter/M_ScatterVelocity.M_ScatterVelocity'");
	const FSoftObjectPath ScatterPosition_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Scatter/M_ScatterPosition.M_ScatterPosition'");
	const FSoftObjectPath ParticleWaterSeed_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Scatter/M_ParticleWaterSeed.M_ParticleWaterSeed'");
	const FSoftObjectPath StartPositionErode_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Scatter/M_StartPositionErode.M_StartPositionErode'");
	const FSoftObjectPath UnpackRG8Height_MaterialReference(
		"Material'/TerrainTools/HydraulicErosion/Materials/Seed/M_UnpackRG8Height.M_UnpackRG8Height'");
	const FSoftObjectPath MPC_Erosion_AssetReference(
		"MaterialParameterCollection'/TerrainTools/HydraulicErosion/MPC/MPC_Erosion.MPC_Erosion'");
}



UHydraulicErosionComponent::UHydraulicErosionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UHydraulicErosionComponent::Constructor(FHydraulicErosionDada InHydraulicErosionDada)
{
	HydraulicErosionDada = InHydraulicErosionDada;
	OwningLayerStack = Cast<AHydraulicErosion>(GetOwner());
	check(OwningLayerStack)

		if (OwningLayerStack)
	{
		LoadMaterials();
		CreateMIDs();
		SetMIDParams();

		if (HydraulicErosionMode == FreeParticles && OwningLayerStack->ParticlesScatterISMC)
		{
			OwningLayerStack->ParticlesScatterISMC->SetMaterial(0, ScatterParticlePreviewMID);
		}
	}
}

void UHydraulicErosionComponent::TickErosion(int32 InIterations)
{
	StartErode(FMath::Min(InIterations - 1, 32));
}

void UHydraulicErosionComponent::TickErosionOnce(int32 InIterations)
{
	StartErode(FMath::Min(InIterations - 1, 32));
}

void UHydraulicErosionComponent::StartErode(const int32 InIterations)
{
	for (int32 Iter = 0; Iter <= InIterations; Iter++)
	{
		if (HydraulicErosionMode == EHydraulicErosionMode::FreeParticles)
		{
			FreeParticlesScatter();
		}
		if (HydraulicErosionMode == EHydraulicErosionMode::ShallowWater)
		{
			ShallowWaterAdvection();
		}
		if (HydraulicErosionMode == EHydraulicErosionMode::ScatterGather)
		{
			ScatterGatherMethod();
		}
		if (HydraulicErosionMode == EHydraulicErosionMode::SimpleWaterLevelling)
		{
			SimpleWaterLevelling();
		}

		IterationIdx++;

		if (UMaterialParameterCollectionInstance* MaterialParamInstance =
				GetWorld()->GetParameterCollectionInstance(ErosionMPC))
		{
			MaterialParamInstance->SetScalarParameterValue("idx", IterationIdx);
		}

		RenderWeightmapEffects();
	}

	OwningLayerStack->UpdateNormalAndPreviewRTAssignments(PingPongVelocitySource());
}

void UHydraulicErosionComponent::SeedParticlePositions()
{
	UKismetRenderingLibrary::ClearRenderTarget2D(this, HydraulicErosionDada.PositionRTA);
	UKismetRenderingLibrary::ClearRenderTarget2D(this, HydraulicErosionDada.PositionRTB);
	ParticlePositionSeedMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(
		this,
		HydraulicErosionDada.PositionRTA,
		ParticlePositionSeedMID);
	ScatterParticlePreviewMID->SetTextureParameterValue("PositionRT", HydraulicErosionDada.PositionRTA);
}

void UHydraulicErosionComponent::CreateMIDs()
{
	if (HydraulicErosionMode == FreeParticles)
	{
		// Particle Rendering MIDs
		ScatterParticlePreviewMID = FTerrainUtils::GetOrCreateTransientMID(
			ScatterParticlePreviewMID,
			"ScatterParticlePreviewMID",
			ParticlePreviewMat);
		ScatterParticleRenderMID = FTerrainUtils::GetOrCreateTransientMID(
			ScatterParticleRenderMID,
			"ScatterParticleRenderMID",
			ParticleRenderMat);

		// Simulation MIDs
		ScatterVelocityMID =
			FTerrainUtils::GetOrCreateTransientMID(ScatterVelocityMID, "ScatterVelocityMID", ScatterVelocityMat);
		ScatterPositionMID =
			FTerrainUtils::GetOrCreateTransientMID(ScatterPositionMID, "ScatterPositionMID", ScatterPositionMat);
		ParticlePositionSeedMID = FTerrainUtils::GetOrCreateTransientMID(
			ParticlePositionSeedMID,
			"ParticlePositionSeedMID",
			ParticleWaterSeedMat);
		ScatterHeightErosionMID = FTerrainUtils::GetOrCreateTransientMID(
			ScatterHeightErosionMID,
			"ScatterHeightErosionMID",
			StartPositionErodeMat);
	}
	else
	{
		ScatterParticlePreviewMID = nullptr;
		ScatterParticleRenderMID = nullptr;
		ScatterVelocityMID = nullptr;
		ScatterPositionMID = nullptr;
		ParticlePositionSeedMID = nullptr;
		ScatterHeightErosionMID = nullptr;
	}

	if (HydraulicErosionMode == EHydraulicErosionMode::ShallowWater)
	{
		// Velocity Acceleration of Gravity
		VelocityAccelerationMID = FTerrainUtils::GetOrCreateTransientMID(
			VelocityAccelerationMID,
			"VelocityAccelerationMID",
			GravAccelerationMat);
		// Advect or Gather Height
		AdvectHeightMID =
			FTerrainUtils::GetOrCreateTransientMID(AdvectHeightMID, "AdvectHeightMID", ShallowWaterAdvectHeightMat);
		// Advect or Gather Velocity
		AdvectVelocityMID = FTerrainUtils::GetOrCreateTransientMID(
			AdvectVelocityMID,
			"AdvectVelocityMID",
			ShallowWaterAdvectVelocityMat);

		DivergenceHeightUpdateMID = FTerrainUtils::GetOrCreateTransientMID(
			DivergenceHeightUpdateMID,
			"DivergenceHeightUpdateMID",
			CalcDivergenceMat);
	}
	else
	{
		DivergenceHeightUpdateMID = nullptr;
	}

	if (HydraulicErosionMode == EHydraulicErosionMode::ScatterGather)
	{
		// Velocity Acceleration of Gravity
		VelocityAccelerationMID = FTerrainUtils::GetOrCreateTransientMID(
			VelocityAccelerationMID,
			"VelocityAccelerationMID",
			ScatterGatherVelocityMat);
		// Advect or Gather Height
		AdvectHeightMID = FTerrainUtils::GetOrCreateTransientMID(AdvectHeightMID, "AdvectHeightMID", GatherDataMat);
		// Advect or Gather Velocity
		AdvectVelocityMID =
			FTerrainUtils::GetOrCreateTransientMID(AdvectVelocityMID, "AdvectVelocityMID", GatherVelocityMat);
		// Water level
		WaterLevelMID = FTerrainUtils::GetOrCreateTransientMID(WaterLevelMID, "WaterLevelMID", WaterLevelMat);
	}
	else
	{
		WaterLevelMID = nullptr;
	}

	if (HydraulicErosionMode == EHydraulicErosionMode::SimpleWaterLevelling)
	{
		// Velocity Acceleration of Gravity
		VelocityAccelerationMID = nullptr;
		// Advect or Gather Height
		AdvectHeightMID = FTerrainUtils::GetOrCreateTransientMID(AdvectHeightMID, "AdvectHeightMID", WaterLevelMat);
		// Advect or Gather Velocity
		AdvectVelocityMID = nullptr;
	}

	WeightmapEffectsMID =
		FTerrainUtils::GetOrCreateTransientMID(WeightmapEffectsMID, "WeightmapEffectsMID", ErosionWeightmapMat);
}

void UHydraulicErosionComponent::SetMIDParams()
{
	if (ParticlePositionSeedMID)
	{
		ParticlePositionSeedMID->SetTextureParameterValue("HeightRT", HydraulicErosionDada.HeightRTA);
	}

	if (ScatterParticlePreviewMID)
	{
		ScatterParticlePreviewMID->SetTextureParameterValue("VelocityRT", HydraulicErosionDada.FluidVelocityA);
		ScatterParticlePreviewMID->SetTextureParameterValue("PositionRT", HydraulicErosionDada.PositionRTA);
	}

	if (VelocityAccelerationMID)
	{
		VelocityAccelerationMID->SetTextureParameterValue("NormalRT", HydraulicErosionDada.NormalRT);
	}

	if (AdvectVelocityMID)
	{
		AdvectVelocityMID->SetTextureParameterValue("NormalRT", HydraulicErosionDada.NormalRT);
	}

	if (AdvectHeightMID)
	{
		AdvectHeightMID->SetTextureParameterValue("NormalRT", HydraulicErosionDada.NormalRT);
	}

	if (ScatterVelocityMID)
	{
		ScatterVelocityMID->SetTextureParameterValue("NormalRT", HydraulicErosionDada.NormalRT);
	}

	if (ScatterPositionMID)
	{
		ScatterPositionMID->SetTextureParameterValue("NormalRT", HydraulicErosionDada.NormalRT);
	}

	if (ScatterHeightErosionMID)
	{
		ScatterHeightErosionMID->SetTextureParameterValue("NormalRT", HydraulicErosionDada.NormalRT);
		ScatterHeightErosionMID->SetTextureParameterValue("SceneCaptureRT", HydraulicErosionDada.MeshDepthRT);
	}
}

void UHydraulicErosionComponent::LoadMaterials()
{
	GravAccelerationMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::GravAcceleration_MaterialReference.TryLoad());

	ScatterGatherVelocityMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ScatterGatherVelocity_MaterialReference.TryLoad());

	GatherDataMat = CastChecked<UMaterialInterface>(HydraulicErosionLocal::GatherData_MaterialReference.TryLoad());

	GatherVelocityMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::GatherVelocity_MaterialReferencel.TryLoad());

	WaterLevelMat = CastChecked<UMaterialInterface>(HydraulicErosionLocal::WaterLevel_MaterialReference.TryLoad());

	ErosionWeightmapMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ErosionWeightmap_MaterialReference.TryLoad());

	CalcDivergenceMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::CalcDivergence_MaterialReference.TryLoad());

	ParticlePreviewMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ParticlePreview_MaterialReference.TryLoad());

	ParticleRenderMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ParticleRender_MaterialReference.TryLoad());

	ScatterVelocityMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ScatterVelocity_MaterialReference.TryLoad());

	ScatterPositionMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ScatterPosition_MaterialReference.TryLoad());

	ParticleWaterSeedMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ParticleWaterSeed_MaterialReference.TryLoad());

	StartPositionErodeMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::StartPositionErode_MaterialReference.TryLoad());

	ShallowWaterAdvectHeightMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ShallowWaterAdvectHeight_MaterialReference.TryLoad());

	ShallowWaterAdvectVelocityMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::ShallowWaterAdvectVelocity_MaterialReference.TryLoad());

	UnpackRG8HeightMat =
		CastChecked<UMaterialInterface>(HydraulicErosionLocal::UnpackRG8Height_MaterialReference.TryLoad());

	ErosionMPC = CastChecked<UMaterialParameterCollection>(HydraulicErosionLocal::MPC_Erosion_AssetReference.TryLoad());
}

UTextureRenderTarget2D* UHydraulicErosionComponent::PingPongPositionSource() const
{
	if (PositionIdx % 2)
		return HydraulicErosionDada.PositionRTB;
	else
		return HydraulicErosionDada.PositionRTA;
}

UTextureRenderTarget2D* UHydraulicErosionComponent::PingPongPositionTarget() const
{
	if ((PositionIdx + 1) % 2)
		return HydraulicErosionDada.PositionRTB;
	else
		return HydraulicErosionDada.PositionRTA;
}

UTextureRenderTarget2D* UHydraulicErosionComponent::PingPongVelocitySource() const
{
	if (VelocityIdx % 2)
		return HydraulicErosionDada.FluidVelocityB;
	else
		return HydraulicErosionDada.FluidVelocityA;
}

UTextureRenderTarget2D* UHydraulicErosionComponent::PingPongVelocityTarget() const
{
	if ((VelocityIdx + 1) % 2)
		return HydraulicErosionDada.FluidVelocityB;
	else
		return HydraulicErosionDada.FluidVelocityA;
}

UTextureRenderTarget2D* UHydraulicErosionComponent::PingPongHeightSource() const
{
	if (StackHeightIdx % 2)
		return HydraulicErosionDada.HeightRTB;
	else
		return HydraulicErosionDada.HeightRTA;
}

UTextureRenderTarget2D* UHydraulicErosionComponent::PingPongHeightTarget() const
{
	if ((StackHeightIdx + 1) % 2)
		return HydraulicErosionDada.HeightRTB;
	else
		return HydraulicErosionDada.HeightRTA;
}

void UHydraulicErosionComponent::FreeParticlesScatter()
{
	// Scatter Velocity
	ScatterVelocityMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	ScatterVelocityMID->SetTextureParameterValue("PositionRT", PingPongPositionSource());
	ScatterVelocityMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongVelocityTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongVelocityTarget(), ScatterVelocityMID);
	VelocityIdx++;

	// Scatter Position
	ScatterPositionMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	ScatterPositionMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	ScatterPositionMID->SetTextureParameterValue("PositionRT", PingPongPositionSource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongPositionTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongPositionTarget(), ScatterPositionMID);
	PositionIdx++;

	// Capture Particles to Render Target
	OwningLayerStack->ParticlesScatterISMC->SetMaterial(0, ScatterParticleRenderMID);
	ScatterParticleRenderMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	ScatterParticleRenderMID->SetTextureParameterValue("VelocityRTPrev", PingPongVelocityTarget());
	ScatterParticleRenderMID->SetTextureParameterValue("PositionRT", PingPongPositionSource());

	ScatterParticlePreviewMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	ScatterParticlePreviewMID->SetTextureParameterValue("VelocityRTPrev", PingPongVelocityTarget());
	ScatterParticlePreviewMID->SetTextureParameterValue("PositionRT", PingPongPositionSource());

	OwningLayerStack->ParticlesScatterISMC->SetVisibility(true);
	OwningLayerStack->ScatterSceneCapture->CaptureScene();
	OwningLayerStack->ParticlesScatterISMC->SetMaterial(0, ScatterParticlePreviewMID);
	OwningLayerStack->ParticlesScatterISMC->SetVisibility(OwningLayerStack->bShowWater);

	// Erode Terrain using Particles
	ScatterHeightErosionMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongHeightTarget(), ScatterHeightErosionMID);
	StackHeightIdx++;

	// Reset Iterations
	if (OwningLayerStack->ResetEveryNInterations > 0)
	{
		if ((StackHeightIdx % OwningLayerStack->ResetEveryNInterations) == 0)
		{
			UKismetRenderingLibrary::ClearRenderTarget2D(
				this,
				HydraulicErosionDada.FluidVelocityA,
				FLinearColor::Transparent);
			UKismetRenderingLibrary::ClearRenderTarget2D(
				this,
				HydraulicErosionDada.FluidVelocityB,
				FLinearColor::Transparent);
			UKismetRenderingLibrary::ClearRenderTarget2D(this, HydraulicErosionDada.PositionRTA, FLinearColor::Black);
			UKismetRenderingLibrary::ClearRenderTarget2D(this, HydraulicErosionDada.PositionRTB, FLinearColor::Black);

			ParticlePositionSeedMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());

			UKismetRenderingLibrary::DrawMaterialToRenderTarget(
				this,
				HydraulicErosionDada.PositionRTA,
				ParticlePositionSeedMID);
			UKismetRenderingLibrary::DrawMaterialToRenderTarget(
				this,
				HydraulicErosionDada.PositionRTB,
				ParticlePositionSeedMID);

			StackHeightIdx = OwningLayerStack->ResetEveryNInterations;
			VelocityIdx = 0;
			PositionIdx = 0;
			IterationIdx = 0;
		}
	}
}

void UHydraulicErosionComponent::ShallowWaterAdvection()
{
	// Advect Height
	AdvectHeightMID->SetScalarParameterValue("idx", IterationIdx);
	AdvectHeightMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	AdvectHeightMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongHeightTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongHeightTarget(), AdvectHeightMID);
	StackHeightIdx++;

	// Advect Velocity
	AdvectVelocityMID->SetScalarParameterValue("idx", IterationIdx);
	AdvectVelocityMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongVelocityTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongVelocityTarget(), AdvectVelocityMID);
	VelocityIdx++;

	// Divergence Height Update
	DivergenceHeightUpdateMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	DivergenceHeightUpdateMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongHeightTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongHeightTarget(), DivergenceHeightUpdateMID);
	StackHeightIdx++;

	// Add Acceleration to Grid Particles
	VelocityAccelerationMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	VelocityAccelerationMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongVelocityTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongVelocityTarget(), VelocityAccelerationMID);
	VelocityIdx++;
}

void UHydraulicErosionComponent::ScatterGatherMethod()
{
	// Add Acceleration to Grid Particles
	VelocityAccelerationMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	VelocityAccelerationMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongVelocityTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongVelocityTarget(), VelocityAccelerationMID);
	VelocityIdx++;

	// Gather Velocity
	AdvectVelocityMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	AdvectVelocityMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongVelocityTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongVelocityTarget(), AdvectVelocityMID);
	VelocityIdx++;

	// Gather Water and Height
	AdvectHeightMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	AdvectHeightMID->SetTextureParameterValue("VelocityRTCurrent", PingPongVelocitySource());
	AdvectHeightMID->SetTextureParameterValue("VelocityRT", PingPongVelocityTarget());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongHeightTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongHeightTarget(), AdvectHeightMID);
	StackHeightIdx++;
}

void UHydraulicErosionComponent::SimpleWaterLevelling()
{
	AdvectHeightMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
	UKismetRenderingLibrary::ClearRenderTarget2D(this, PingPongHeightTarget());
	UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, PingPongHeightTarget(), AdvectHeightMID);
	StackHeightIdx++;
}

void UHydraulicErosionComponent::RenderWeightmapEffects()
{
	FLinearColor ChannelMask;
	for (FErosionWeightmapEffects& ErosionWeightmapEffect : OwningLayerStack->ErosionWeightmapEffects)
	{
		TArray<FString> Parsed = UKismetStringLibrary::ParseIntoArray(ErosionWeightmapEffect.WeightmapName, ".");
		if (FRenderTargetChannels* RTChannels =
				OwningLayerStack->Weightmaps.Find(UKismetStringLibrary::Conv_StringToName(Parsed[0])))
		{
			if (IsValid(RTChannels->RenderTarget))
			{
				const FName ParsedName = UKismetStringLibrary::Conv_StringToName(Parsed[1]);

				if (RTChannels->R == ParsedName)
				{
					ChannelMask = FLinearColor { 1, 0, 0, 0 };
				}
				else if (RTChannels->G == ParsedName)
				{
					ChannelMask = FLinearColor { 0, 1, 0, 0 };
				}
				else if (RTChannels->B == ParsedName)
				{
					ChannelMask = FLinearColor { 0, 0, 1, 0 };
				}
				else if (RTChannels->A == ParsedName)
				{
					ChannelMask = FLinearColor { 0, 0, 0, 1 };
				}

				WeightmapEffectsMID->SetVectorParameterValue("ChannelMask", ChannelMask);
				WeightmapEffectsMID->SetScalarParameterValue("Deposition", ErosionWeightmapEffect.DepositionStrength);
				WeightmapEffectsMID->SetScalarParameterValue("Erosion", ErosionWeightmapEffect.ErosionStrength);
				WeightmapEffectsMID->SetScalarParameterValue("Velocity", ErosionWeightmapEffect.VelocityStrength);

				if (HydraulicErosionMode == FreeParticles)
				{
					WeightmapEffectsMID->SetTextureParameterValue("VelocityRT", HydraulicErosionDada.MeshDepthRT);
				}
				else
				{
					WeightmapEffectsMID->SetTextureParameterValue("VelocityRT", PingPongVelocitySource());
				}

				WeightmapEffectsMID->SetTextureParameterValue("HeightRT", PingPongHeightSource());
				WeightmapEffectsMID->SetTextureParameterValue("HeightRTPrev", PingPongHeightTarget());
				WeightmapEffectsMID->SetScalarParameterValue("ScatterParticles", HydraulicErosionMode == FreeParticles);

				UKismetRenderingLibrary::DrawMaterialToRenderTarget(
					this,
					RTChannels->RenderTarget,
					WeightmapEffectsMID);
			}
		}
	}
}
