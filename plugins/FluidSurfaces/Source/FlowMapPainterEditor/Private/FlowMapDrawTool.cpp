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
#include "FlowMapDrawTool.h"

#include "Engine/TextureRenderTarget2D.h"
#include "InteractiveToolManager.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ToolBuilderUtil.h"
#include "UObject/ConstructorHelpers.h"
#include "WaterPlaneInterface.h"

#define LOCTEXT_NAMESPACE "FlowMapDrawTool"

namespace FFlowMapDrawToolLocal
{
	const TCHAR* AddForceMaterialPath(TEXT("/FluidSurfaces/FlowMap/M_AddForce.M_AddForce"));
	const TCHAR* FlowMapRTPath(TEXT("/FluidSurfaces/FlowMap/RT_FlowMap.RT_FlowMap"));
}

UMeshSurfacePointTool* UFlowMapDrawToolBuilder::CreateNewTool(const FToolBuilderState& SceneState) const
{
	UFlowMapDrawTool* NewTool = NewObject<UFlowMapDrawTool>(SceneState.ToolManager);
	return NewTool;
}

bool UFlowMapDrawToolBuilder::CanBuildTool(const FToolBuilderState& SceneState) const
{
	for (const AActor* Actor : SceneState.SelectedActors)
	{
		if (IsValid(Actor) && Actor->GetClass()->ImplementsInterface(UWaterPlaneInterface::StaticClass()))
		{
			return true;
		}
	}
	return false;
}

void UFlowMapDrawToolBuilder::InitializeNewTool(UMeshSurfacePointTool* Tool, const FToolBuilderState& SceneState) const
{
	for (AActor* Actor : SceneState.SelectedActors)
	{
		if (IsValid(Actor))
		{
			if (Actor->GetClass()->ImplementsInterface(UWaterPlaneInterface::StaticClass()))
			{
				if (IWaterPlaneInterface* WaterPlaneInterface = Cast<IWaterPlaneInterface>(Actor))
				{
					if (UPrimitiveComponent* Component = WaterPlaneInterface->GetSurface())
					{
						Tool->SetStylusAPI(this->StylusAPI);
						Tool->SetSelection(MakeComponentTarget(Component));
						break;
					}
				}
			}
		}
	}
}

UFlowMapDrawToolProperties::UFlowMapDrawToolProperties()
{
}

UFlowMapDrawTool::UFlowMapDrawTool()
{
	struct FConstructorStaticsFlow
	{
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> AddForceMaterial;
		ConstructorHelpers::FObjectFinderOptional<UTextureRenderTarget2D> FlowMapRT;

		FConstructorStaticsFlow()
			: AddForceMaterial(FFlowMapDrawToolLocal::AddForceMaterialPath)
			, FlowMapRT(FFlowMapDrawToolLocal::FlowMapRTPath)
		{
		}
	};
	static FConstructorStaticsFlow ConstructorStaticsFlow;
	FlowMapRT = ConstructorStaticsFlow.FlowMapRT.Get();
	ForceMaterial = ConstructorStaticsFlow.AddForceMaterial.Get();
}

void UFlowMapDrawTool::Setup()
{
	UMeshSurfacePointTool::Setup();

	Settings = NewObject<UFlowMapDrawToolProperties>(this, TEXT("Settings"));
	Settings->RestoreProperties(this);
	AddToolPropertySource(Settings.Get());

	bEraseModifierDown = false;
	bSizeModifierDown = false;
}

void UFlowMapDrawTool::Shutdown(EToolShutdownType ShutdownType)
{
	Settings->SaveProperties(this);
	Super::Shutdown(ShutdownType);
}

void UFlowMapDrawTool::Render(IToolsContextRenderAPI* RenderAPI)
{
	UMeshSurfacePointTool::Render(RenderAPI);
	FPrimitiveDrawInterface* PDI = RenderAPI->GetPrimitiveDrawInterface();

	const FMaterialRenderProxy* SphereMaterialProxy = GEngine->EditorBrushMaterial->GetRenderProxy();
	const FVector DebugSphereSize = GetDrawBrushSize() * FVector(0.5f);
	DrawSphere(PDI, LastLocation, FRotator::ZeroRotator, DebugSphereSize, 64, 64, SphereMaterialProxy, SDPG_Foreground);
}

void UFlowMapDrawTool::OnBeginDrag(const FRay& Ray)
{
	Super::OnBeginDrag(Ray);
	FHitResult OutHit;
	if (HitTest(Ray, OutHit))
	{
		Location = OutHit.ImpactPoint;
		TextureSize = FIntPoint(FlowMapRT->SizeX, FlowMapRT->SizeX);
		WorldSize = ComponentTarget->Component->Bounds.BoxExtent.GetMax() * 2;
		Transform = ComponentTarget->Component->GetComponentTransform();

		if (IWaterPlaneInterface* WaterPlaneInterface =
				Cast<IWaterPlaneInterface>(ComponentTarget->Component->GetOwner()))
		{
			if (UPrimitiveComponent* TargetComponent = WaterPlaneInterface->GetTargetSurface())
			{
				if (UMeshComponent* MeshComponent = Cast<UMeshComponent>(TargetComponent))
				{
					if (!IsValid(OutputTexture))
					{
						OutputTexture = WaterPlaneInterface->GetOutputTexture();
					}

					DefaultApplyOrRemoveTextureOverride(MeshComponent, OutputTexture, FlowMapRT);
				}
			}
		}
	}
}

void UFlowMapDrawTool::OnEndDrag(const FRay& Ray)
{
	Super::OnEndDrag(Ray);
	FHitResult OutHit;
	if (HitTest(Ray, OutHit))
	{
		Location = OutHit.ImpactPoint;
	}
}

void UFlowMapDrawTool::OnUpdateDrag(const FRay& Ray)
{
	FHitResult OutHit;
	if (HitTest(Ray, OutHit))
	{
		if (ComponentTarget.IsValid() && IsValid(ComponentTarget->Component))
		{
			const UWorld* World = GEditor->GetActiveViewport()->GetClient()->GetWorld();

			if (!World)
				return;

			if (!bSizeModifierDown)
			{
				for (int32 Iter = 0; Iter < Settings->Flow; Iter++)
				{
					const FVector NewLocation =
						UKismetMathLibrary::VLerp(OutHit.Location, Location, Iter / (float)Settings->Flow);

					if (NewLocation.SizeSquared() < KINDA_SMALL_NUMBER)
					{
						return;
					}

					const FVector NewVelocity =
						bEraseModifierDown ? (FVector::ZeroVector) : (NewLocation - Location).GetUnsafeNormal2D();

					FLinearColor Channel = FLinearColor::Transparent;
					switch (Settings->BrushType)
					{
					case EFlowmapToolBrushType::FMT_Flowmap:
						Channel = FLinearColor { 1, 1, 0, 0 };
						break;
					case EFlowmapToolBrushType::FMT_Swamp:
						Channel = FLinearColor { 0, 0, 1, 0 };
						break;
					case EFlowmapToolBrushType::FMT_Foam:
						Channel = FLinearColor { 0, 0, 0, 1 };
						break;
					default:;
					}

					DrawTexture(
						World->GetWorld(),
						NewLocation,
						NewVelocity,
						GetDrawBrushSize(),
						bEraseModifierDown ? GetEraseDensity() : GetDrawDensity(),
						GetHardness(),
						bEraseModifierDown,
						Channel);
				}
			}

			if (!bSizeModifierDown)
			{
				Location = OutHit.ImpactPoint;
				LastLocation = OutHit.ImpactPoint;
				Normal = OutHit.ImpactNormal;
			}
			else
			{
				Settings->BrushSize = abs((Location - OutHit.Location).Size() * 2);
			}
		}
	}
}

bool UFlowMapDrawTool::OnUpdateHover(const FInputDeviceRay& DevicePos)
{
	FHitResult OutHit;
	if (HitTest(DevicePos.WorldRay, OutHit))
	{
		LastLocation = OutHit.ImpactPoint;
		Normal = OutHit.ImpactNormal;
	}
	return true;
}

void UFlowMapDrawTool::OnUpdateModifierState(int ModifierID, bool bIsOn)
{
	// keep track of the "Erase" modifier (shift key for mouse input)
	if (ModifierID == EraseModifierID)
	{
		bEraseModifierDown = bIsOn;
	}
	// keep track of the "Size" modifier (ctrl key for mouse input)
	if (ModifierID == SizeModifierID)
	{
		bSizeModifierDown = bIsOn;
	}
}

bool UFlowMapDrawTool::HitTest(const FRay& Ray, FHitResult& OutHit)
{
	const FVector End = Ray.PointAt(HALF_WORLD_MAX);
	if (ComponentTarget.IsValid() && ComponentTarget.Get()->Component)
	{
		return ComponentTarget.Get()->Component->LineTraceComponent(
			OutHit,
			Ray.Origin,
			End,
			FCollisionQueryParams(SCENE_QUERY_STAT(HitTest), true));
	}
	return false;
}

float UFlowMapDrawTool::GetDrawBrushSize() const
{
	return Settings->BrushSize;
}

float UFlowMapDrawTool::GetDrawDensity() const
{
	return Settings->Density / static_cast<float>(Settings->Flow);
}

float UFlowMapDrawTool::GetEraseDensity() const
{
	return Settings->Erase / static_cast<float>(Settings->Flow);
}

float UFlowMapDrawTool::GetHardness() const
{
	return Settings->Hardness;
}

void UFlowMapDrawTool::DefaultApplyOrRemoveTextureOverride(
	UMeshComponent* InMeshComponent,
	UTexture* SourceTexture,
	UTexture* OverrideTexture)
{
	const ERHIFeatureLevel::Type FeatureLevel = InMeshComponent->GetWorld()->FeatureLevel;

	// Check all the materials on the mesh to see if the user texture is there
	int32 MaterialIndex = 0;
	UMaterialInterface* MaterialToCheck = InMeshComponent->GetMaterial(MaterialIndex);
	while (MaterialToCheck != nullptr)
	{
		const bool bIsTextureUsed = DoesMaterialUseTexture(MaterialToCheck, SourceTexture);
		if (bIsTextureUsed)
		{
			MaterialToCheck->OverrideTexture(SourceTexture, OverrideTexture, FeatureLevel);
		}

		++MaterialIndex;
		MaterialToCheck = InMeshComponent->GetMaterial(MaterialIndex);
	}
}

void UFlowMapDrawTool::DrawTexture(
	UObject* WorldContextObject,
	FVector ForceLocation,
	FVector ForceVelocity,
	float BrushSize,
	float Density,
	float Hardness,
	bool bErase,
	FLinearColor Channel)
{
	if (!ForceMID)
	{
		ForceMID =
			UKismetMaterialLibrary::CreateDynamicMaterialInstance(WorldContextObject, ForceMaterial, TEXT("Force"));
	}

	if ((FlowMapRT->SizeX != TextureSize.X) || (FlowMapRT->SizeY != TextureSize.Y))
	{
		FlowMapRT->ResizeTarget(TextureSize.X, TextureSize.Y);
	}

	if (ForceMID && FlowMapRT)
	{
		const FVector Position =
			Transform.InverseTransformPosition(ForceLocation) / (float)WorldSize.GetMax() * Transform.GetScale3D()
			+ 0.5f;
		ForceMID->SetVectorParameterValue(TEXT("Position"), Position);
		ForceMID->SetVectorParameterValue(TEXT("Channel"), Channel);
		ForceMID->SetVectorParameterValue(TEXT("Velocity"), ForceVelocity);
		ForceMID->SetScalarParameterValue(TEXT("Radius"), BrushSize);
		ForceMID->SetScalarParameterValue(TEXT("Density"), Density);
		ForceMID->SetScalarParameterValue(TEXT("Hardness"), Hardness);
		ForceMID->SetScalarParameterValue(TEXT("Clear"), bErase);
		ForceMID->SetScalarParameterValue(TEXT("Size"), WorldSize.GetMax());

		UKismetRenderingLibrary::DrawMaterialToRenderTarget(WorldContextObject, FlowMapRT, ForceMID);
	}
}


#undef LOCTEXT_NAMESPACE
