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


#include "FlowMapPlane.h"

#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"


AFlowMapPlane::AFlowMapPlane()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsEditorOnlyActor = true;

	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> PreviewMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> AddForceMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> CopyTextureMaterial;
		ConstructorHelpers::FObjectFinderOptional<UTextureRenderTarget2D> FlowMapRT;

		FConstructorStatics()
			: PlaneMesh(TEXT("/FluidSurfaces/FlowMap/S_1_Unit_Plane.S_1_Unit_Plane"))
			, PreviewMaterial(TEXT("/FluidSurfaces/FlowMap/M_FlowMapPreview.M_FlowMapPreview"))
			, AddForceMaterial(TEXT("/FluidSurfaces/FlowMap/M_AddForce.M_AddForce"))
			, CopyTextureMaterial(TEXT("/FluidSurfaces/FlowMap/M_CopyTexture.M_CopyTexture"))
			, FlowMapRT(TEXT("/FluidSurfaces/FlowMap/RT_FlowMap.RT_FlowMap"))
		{
		}
	};
	static FConstructorStatics ConstructorStatics;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	DrawPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DrawPlane"));
	DrawPlane->SetupAttachment(RootComponent);
	DrawPlane->SetStaticMesh(ConstructorStatics.PlaneMesh.Get());
	DrawPlane->SetMaterial(0, ConstructorStatics.PreviewMaterial.Get());
	DrawPlane->SetWorldScale3D(FVector::OneVector);
	DrawPlane->SetCanEverAffectNavigation(false);
	DrawPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ForceMaterial = ConstructorStatics.AddForceMaterial.Get();
	CopyTextureMaterial = ConstructorStatics.CopyTextureMaterial.Get();
	FlowMapRT = ConstructorStatics.FlowMapRT.Get();
}

void AFlowMapPlane::DrawTexture(
	FVector ForceLocation,
	FVector ForceVelocity,
	float BrushSize,
	float Density,
	bool bErase)
{
	if (!ForceMID)
	{
		ForceMID = UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, ForceMaterial, TEXT("Force"));
	}

	if ((FlowMapRT->SizeX != TextureSize.X) || (FlowMapRT->SizeY != TextureSize.Y))
	{
		FlowMapRT->ResizeTarget(TextureSize.X, TextureSize.Y);
	}

	if (ForceMID && FlowMapRT)
	{
		const FVector Position = GetTransform().InverseTransformPosition(ForceLocation) / (float)WorldSize.GetMax()
				* GetTransform().GetScale3D()
			+ 0.5f;
		ForceMID->SetVectorParameterValue(TEXT("Position"), Position);
		ForceMID->SetVectorParameterValue(TEXT("Velocity"), ForceVelocity);
		ForceMID->SetScalarParameterValue(TEXT("Radius"), BrushSize);
		ForceMID->SetScalarParameterValue(TEXT("Density"), Density);
		ForceMID->SetScalarParameterValue(TEXT("Clear"), bErase);
		ForceMID->SetScalarParameterValue(TEXT("Size"), WorldSize.GetMax());

		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, FlowMapRT, ForceMID);
	}
}

void AFlowMapPlane::SetDrawPlane(UStaticMeshComponent* InComponent)
{
	DrawPlane = InComponent;
}

void AFlowMapPlane::ClearTexture()
{
	if (FlowMapRT)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, FlowMapRT);
	}
}

void AFlowMapPlane::SaveTexture()
{
	const TextureCompressionSettings CompressionSettings = TextureCompressionSettings::TC_VectorDisplacementmap;
	const TextureMipGenSettings MipSettings = TextureMipGenSettings::TMGS_NoMipmaps;

	if (OutputTexture)
	{
		UKismetRenderingLibrary::ConvertRenderTargetToTexture2DEditorOnly(this, FlowMapRT, OutputTexture);
		// Update Compression and Mip settings
		OutputTexture->CompressionSettings = CompressionSettings;
		OutputTexture->MipGenSettings = MipSettings;
		OutputTexture->PostEditChange();
	}
	else
	{
		FString Dir = OutputDirectory.Path + TEXT("/") + OutputTextureName;
		Dir = FPaths::CreateStandardFilename(Dir);
		OutputTexture = UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(
			FlowMapRT,
			Dir,
			CompressionSettings,
			MipSettings);
	}
}

void AFlowMapPlane::FillTexture()
{
	if (FlowMapRT)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, FlowMapRT, FillColor);
	}
}

void AFlowMapPlane::CopyTexture()
{
	if (OutputTexture && FlowMapRT)
	{
		UMaterialInstanceDynamic* CopyMID =
			UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CopyTextureMaterial, TEXT("CopyTexture"));
		CopyMID->SetTextureParameterValue(TEXT("Tex"), OutputTexture);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, FlowMapRT, CopyMID);
	}
}

void AFlowMapPlane::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	SetActorScale3D(FVector(1));
	DrawPlane->SetWorldScale3D(FVector(WorldSize.X, WorldSize.Y, WorldSize.GetMax()));
	DrawPlane->SetVisibility(bShowPreview);
}
