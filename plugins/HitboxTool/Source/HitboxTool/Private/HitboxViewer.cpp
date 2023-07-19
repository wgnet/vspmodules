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

#include "HitboxViewer.h"

#include "Components/SceneCaptureComponent2D.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "HitboxBuilder.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "LevelEditorViewport.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

DECLARE_LOG_CATEGORY_EXTERN(HitboxViewer, Log, All);
DEFINE_LOG_CATEGORY(HitboxViewer);

namespace HitboxViewerLocal
{
	static const FName RebuildHitboxesMeta = TEXT("RebuildHitboxes");
	static const FName UpdateMaterialMeta = TEXT("UpdateMaterial");
	static const FName UpdateViewMeta = TEXT("UpdateView");
}

UHitboxViewer::UHitboxViewer()
{
	GetMaterial();
	UpdateMaterials();
	RenderTarget = CastChecked<UTextureRenderTarget2D>(RenderTargetPath.TryLoad());
}

void UHitboxViewer::BeginDestroy()
{
	Super::BeginDestroy();
	UE_LOG(HitboxViewer, Log, TEXT("Destroy All"));
	Clear();
}

void UHitboxViewer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property->HasMetaData(HitboxViewerLocal::RebuildHitboxesMeta))
	{
		BuildHitboxes(SkeletalMeshComponent);
	}

	if (PropertyChangedEvent.Property->HasMetaData(HitboxViewerLocal::UpdateMaterialMeta))
	{
		UpdateMaterials();
	}

	if (PropertyChangedEvent.Property->HasMetaData(HitboxViewerLocal::UpdateViewMeta))
	{
		StartRendering();
	}
}

void UHitboxViewer::UpdateMaterials()
{
	UE_LOG(HitboxViewer, Log, TEXT("Update Material"));
	if (UMaterialInstanceDynamic* Mid = GetMaterial())
	{
		Mid->SetVectorParameterValue(TEXT("FrontColor"), FrontColor);
		Mid->SetVectorParameterValue(TEXT("BackColor"), BackColor);
		Mid->SetScalarParameterValue(TEXT("Glow"), Glow);
		Mid->SetScalarParameterValue(TEXT("Opacity"), Opacity);
		Mid->SetScalarParameterValue(TEXT("Exponent"), Exponent);
		Mid->SetScalarParameterValue(TEXT("BaseReflect"), BaseReflect);
		Mid->SetScalarParameterValue(TEXT("FadeLenght"), FadeLenght);
		Mid->SetScalarParameterValue(TEXT("FadeOffset"), FadeOffset);
	}
}

UMaterialInstanceDynamic* UHitboxViewer::GetMaterial()
{
	if (MaterialInstanceDynamic)
	{
		return MaterialInstanceDynamic;
	}

	if (!GeometryMaterial || !PostProcessMaterial)
	{
		GeometryMaterial = CastChecked<UMaterialInterface>(GeometryMaterialPath.TryLoad());
		MaterialInstanceDynamic =
			UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, GeometryMaterial, TEXT("DMI"));
		BackMaterialInstanceDynamic =
			UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, GeometryMaterial, TEXT("DMI"));

		PostProcessMaterial = CastChecked<UMaterialInterface>(PostProcessMaterialPath.TryLoad());
	}

	return MaterialInstanceDynamic;
}

UTextureRenderTarget2D* UHitboxViewer::GetRenderTarget()
{
	return RenderTarget;
}

USceneCaptureComponent2D* UHitboxViewer::GetSceneCapture() const
{
	return SceneCapture;
}

void UHitboxViewer::BuildHitboxes(USkeletalMeshComponent* InSkeletalMeshComponent)
{
	if (IsValid(InSkeletalMeshComponent))
	{
		SkeletalMeshComponent = InSkeletalMeshComponent;
		{
			Clear();
			UE_LOG(HitboxViewer, Log, TEXT("Build Hitboxes"));
			FHitboxBuilder HitboxBuilder {
				InSkeletalMeshComponent, MaterialInstanceDynamic, bConvex, bCapsule, bBox, bSphere
			};
			HitboxBuilder.Build(ProceduralMeshComponents, TEXT("Back"));
		}

		{
			UE_LOG(HitboxViewer, Log, TEXT("StartRendering"));
			StartRendering();
			PrevOwner = InSkeletalMeshComponent->GetOwner();
		}
	}
}

void UHitboxViewer::Clear()
{
	if (IsValid(SceneCapture))
	{
		SceneCapture->ConditionalBeginDestroy();
		UE_LOG(HitboxViewer, Log, TEXT("Destroy SceneCapture"));
	}

	for (UProceduralMeshComponent* ProceduralMesh : ProceduralMeshComponents)
	{
		if (IsValid(ProceduralMesh))
			ProceduralMesh->ConditionalBeginDestroy();
		UE_LOG(HitboxViewer, Log, TEXT("Destroy ProceduralMesh"));
	}

	ProceduralMeshComponents.Empty();
}

void UHitboxViewer::ClearAndMarkPendingKill()
{
	Clear();
	MarkPendingKill();
}

void UHitboxViewer::StartRendering()
{
	if (!IsValid(SkeletalMeshComponent))
		return;

	AActor* SkeletalMeshOwner = SkeletalMeshComponent->GetOwner();
	if (!IsValid(SceneCapture))
	{
		UE_LOG(HitboxViewer, Log, TEXT("Add New SceneCapture"));
		SceneCapture = NewObject<USceneCaptureComponent2D>(
			SkeletalMeshOwner,
			TEXT("HBSceneCapture"),
			RF_DuplicateTransient | RF_Transient | RF_TextExportTransient);
		SceneCapture->RegisterComponent();
		SceneCapture->SetComponentTickEnabled(true);
		SceneCapture->TextureTarget = GetRenderTarget();
		SceneCapture->ProjectionType = ECameraProjectionMode::Orthographic;
		SceneCapture->CaptureSource = ESceneCaptureSource::SCS_FinalColorHDR;
		SceneCapture->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_UseShowOnlyList;

		FWeightedBlendable WeightedBlendable;
		WeightedBlendable.Object = PostProcessMaterial;
		WeightedBlendable.Weight = 1;

		SceneCapture->PostProcessSettings.WeightedBlendables.Array.Add(WeightedBlendable);
	}

	if (!SceneCapture->IsRegistered())
		SceneCapture->RegisterComponent();

	auto GetViewOption = [SkeletalMeshOwner](EHitboxViewOptions InViewOptions) -> FVector
	{
		switch (InViewOptions)
		{
		case Top:
			return SkeletalMeshOwner->GetActorUpVector() * FVector(-1, -1, -1);
		case Bottom:
			return SkeletalMeshOwner->GetActorUpVector();
		case Right:
			return SkeletalMeshOwner->GetActorRightVector() * FVector(-1, -1, -1);
		case Left:
			return SkeletalMeshOwner->GetActorRightVector();
		case Front:
			return SkeletalMeshOwner->GetActorForwardVector() * FVector(-1, -1, -1);
		case Back:
			return SkeletalMeshOwner->GetActorForwardVector();
		default:
			return FVector::ZeroVector;
		}
	};

	const FVector ViewDir = GetViewOption(ViewOptions);
	const FAttachmentTransformRules Rules {
		EAttachmentRule::KeepRelative,
		EAttachmentRule::KeepRelative,
		EAttachmentRule::KeepRelative,
		true
	};
	USceneComponent* ComponentAttachTo = SkeletalMeshOwner->GetRootComponent();
	SceneCapture->AttachToComponent(ComponentAttachTo, Rules);
	SceneCapture->SetRelativeTransform(FTransform::Identity);
	SceneCapture->SetRelativeRotation(ViewDir.Rotation());
	const float Offset = 1000.f;
	const FVector Location = SkeletalMeshComponent->Bounds.Origin + Offset * ViewDir * (-1);
	SceneCapture->SetRelativeLocation(Location);

	SceneCapture->OrthoWidth = SkeletalMeshComponent->SkeletalMesh->GetBounds().SphereRadius * 2;

	SceneCapture->ShowOnlyComponents.Empty();
	for (UProceduralMeshComponent* Iter : ProceduralMeshComponents)
	{
		SceneCapture->ShowOnlyComponents.Add(Iter);
	}
}
