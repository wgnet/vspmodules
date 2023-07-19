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


#include "TerrainMaterialProxy.h"

#include "Engine/Texture2DArray.h"
#include "EngineUtils.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "LandscapeHeightfieldCollisionComponent.h"
#include "LandscapeInfo.h"
#include "Materials/MaterialInstanceDynamic.h"
#if WITH_EDITOR
	#include "Components/BillboardComponent.h"
	#include "DrawDebugHelpers.h"
	#include "UObject/ConstructorHelpers.h"
#endif


namespace TerrainMaterialProxyLocal
{
	static TMap<FName, FTerrainMaterialInfo> DefaultMaterialInfo = {
		{ "M00", { "M00", 0, 0, nullptr } }, { "M01", { "M01", 1, 0, nullptr } }, { "M02", { "M02", 2, 0, nullptr } },
		{ "M03", { "M03", 3, 0, nullptr } }, { "M04", { "M04", 4, 0, nullptr } }, { "M05", { "M05", 5, 0, nullptr } },
		{ "M06", { "M06", 6, 0, nullptr } }, { "M07", { "M07", 7, 0, nullptr } }
	};

	static FName TextureParameterName(TEXT("BaseColor (RGB) / Roughness (A)"));
}

ATerrainMaterialProxy::ATerrainMaterialProxy()
{
	PrimaryActorTick.bCanEverTick = false;
	bIsEditorOnlyActor = true;
	//bListedInSceneOutliner = false;

	MaterialsInfo = TerrainMaterialProxyLocal::DefaultMaterialInfo;

#if WITH_EDITOR
	static const FName SpriteComponentName = FName(TEXT("Sprite"));

	Sprite = CreateEditorOnlyDefaultSubobject<UBillboardComponent>(SpriteComponentName);
	SetRootComponent(Sprite);

	if (!IsRunningCommandlet())
	{
		struct FConstructorStatics
		{
			ConstructorHelpers::FObjectFinderOptional<UTexture2D> DecalTexture;
			FName ID_Decals;
			FText NAME_Decals;

			FConstructorStatics()
				: DecalTexture(TEXT("/Engine/EditorResources/S_Terrain.S_Terrain"))
				, ID_Decals(TEXT("Decals"))
				, NAME_Decals(NSLOCTEXT("SpriteCategory", "Decals", "Decals"))
			{
			}
		};
		static FConstructorStatics ConstructorStatics;

		if (Sprite)
		{
			SpriteScale = 10.f;
			Sprite->Sprite = ConstructorStatics.DecalTexture.Get();
			Sprite->SetRelativeScale3D(FVector::OneVector);
			Sprite->SpriteInfo.Category = ConstructorStatics.ID_Decals;
			Sprite->SpriteInfo.DisplayName = ConstructorStatics.NAME_Decals;
			Sprite->bIsScreenSizeScaled = true;
			Sprite->SetUsingAbsoluteScale(true);
			Sprite->bReceivesDecals = false;
		}
	}

#endif
}

void ATerrainMaterialProxy::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	const FVector RayStart = Transform.GetLocation() + Transform.GetLocation() * FVector(0, 0, 10000);
	const FVector RayEnd = Transform.GetLocation() + Transform.GetLocation() * FVector(0, 0, -10000);
	const FCollisionObjectQueryParams QueryParams(FCollisionObjectQueryParams::AllStaticObjects);

	FHitResult Result;
	const bool bHitWorld = GetWorld()->LineTraceSingleByObjectType(Result, RayStart, RayEnd, QueryParams);

	if (bHitWorld)
	{
		if (Result.GetComponent())
		{
			if (ULandscapeHeightfieldCollisionComponent* LandscapeHeightfieldCollisionComponent =
					Cast<ULandscapeHeightfieldCollisionComponent>(Result.GetComponent()))
			{
				if (ULandscapeComponent* LandscapeComponentLocal =
						LandscapeHeightfieldCollisionComponent->RenderComponent.Get())
				{
					if (LandscapeComponentLocal)
					{
						const FVector Center = LandscapeComponentLocal->Bounds.Origin;
						const FVector Extent = LandscapeComponentLocal->Bounds.BoxExtent;
#if WITH_EDITOR
						DrawDebugBox(GetWorld(), Center, Extent, FColor::Red, false, 1.f, 0, 64);
#endif
						this->SetActorLocation(Center);

						if (LandscapeComponent != LandscapeComponentLocal)
						{
							FillData(LandscapeComponent.Get());
							LandscapeComponent = LandscapeComponentLocal;
						}
					}
				}
			}
		}
	}
}

void ATerrainMaterialProxy::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
#if WITH_EDITOR
	GEditor->BeginTransaction(FText::FromString(TEXT("ChangeProperty")));
	ApplyData();
	Refresh();
	GEditor->EndTransaction();

	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.NotifyCustomizationModuleChanged();
#endif
}

void ATerrainMaterialProxy::FillData(ULandscapeComponent* InLandscapeComponent)
{
	if (InLandscapeComponent && InLandscapeComponent->MaterialInstancesDynamic.IsValidIndex(0))
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = InLandscapeComponent->MaterialInstancesDynamic[0])
		{
			for (auto& Elem : MaterialsInfo)
			{
				float ScalarParameterValue;
				DynamicMaterial->GetScalarParameterValue(Elem.Value.ParameterName, ScalarParameterValue);
				Elem.Value.ParameterIndex = ScalarParameterValue;

				UTexture* TextureParameterValue;
				DynamicMaterial->GetTextureParameterValue(
					TerrainMaterialProxyLocal::TextureParameterName,
					TextureParameterValue);
				if (UTexture2DArray* Texture2DArray = Cast<UTexture2DArray>(TextureParameterValue))
				{
					if (Texture2DArray->SourceTextures.IsValidIndex(Elem.Value.ParameterIndex))
					{
						Elem.Value.Texture = Texture2DArray->SourceTextures[Elem.Value.ParameterIndex];
						Elem.Value.MaxIndex = Texture2DArray->SourceTextures.Num() - 1;
					}
				}
			}
		}
	}
}

void ATerrainMaterialProxy::ApplyData()
{
	if (LandscapeComponent && LandscapeComponent.Get()->MaterialInstancesDynamic.IsValidIndex(0))
	{
		if (UMaterialInstanceDynamic* DynamicMaterial = LandscapeComponent.Get()->MaterialInstancesDynamic[0])
		{
			for (auto& Elem : MaterialsInfo)
			{
				DynamicMaterial->SetScalarParameterValue(Elem.Value.ParameterName, Elem.Value.ParameterIndex);
			}
			// Register actor in opened transaction (undo/redo)
			Modify();
			// Register component in opened transaction (undo/redo)
			LandscapeComponent->Modify();
			FPropertyEditorModule& PropertyEditorModule =
				FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
			PropertyEditorModule.NotifyCustomizationModuleChanged();
		}
	}
}

#if WITH_EDITOR

void ATerrainMaterialProxy::Refresh()
{
	if (LandscapeComponent)
	{
		FillData(LandscapeComponent.Get());
		FPropertyEditorModule& PropertyEditorModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyEditorModule.NotifyCustomizationModuleChanged();
	}
}

void ATerrainMaterialProxy::ResetIDs()
{
	if (LandscapeComponent)
	{
		MaterialsInfo = TerrainMaterialProxyLocal::DefaultMaterialInfo;
		for (auto& Elem : MaterialsInfo)
		{
			for (UMaterialInstanceDynamic* DynamicMaterial : LandscapeComponent.Get()->MaterialInstancesDynamic)
			{
				DynamicMaterial->SetScalarParameterValue(Elem.Key, Elem.Value.ParameterIndex);
			}
		}
	}

	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyEditorModule.NotifyCustomizationModuleChanged();
}

TArray<ULandscapeComponent*> ATerrainMaterialProxy::GetLandscapeComponents()
{
	TArray<ULandscapeComponent*> LandscapeComponentsLocal;
	for (TObjectIterator<ULandscapeComponent> ComponentIt; ComponentIt; ++ComponentIt)
	{
		LandscapeComponentsLocal.Add(*ComponentIt);
	}
	return LandscapeComponentsLocal;
}

#endif
