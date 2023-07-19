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
#include "VSPMeshPainterSettings.h"
#include "Engine/Texture2DArray.h"
#include "Materials/MaterialInstanceConstant.h"

using namespace VSPMeshPainterExpressions;

UVSPMeshPainterSettings* UVSPMeshPainterSettings::Get()
{
	return GetMutableDefault<UVSPMeshPainterSettings>();
}

FName UVSPMeshPainterSettings::GetLayerToTextureIdxParamName(int32 LayerIdx)
{
	return FName(FString::FromInt(LayerIdx) + LayerToTextureIdxPostfix);
}

FName UVSPMeshPainterSettings::GetAlphaToTextureIdxParamName(int32 LayerIdx)
{
	return FName(FString::FromInt(LayerIdx) + AlphaToTextureIdxPostfix);
}

FName UVSPMeshPainterSettings::GetAlphaToTextureChannelParamName(int32 LayerIdx)
{
	return FName(FString::FromInt(LayerIdx) + AlphaToTextureChannelPostfix);
}

FName UVSPMeshPainterSettings::GetAlphaToTintColorParamName(int32 LayerIdx)
{
	return FName(FString::FromInt(LayerIdx) + AlphaToTintColorPostfix);
}

FName UVSPMeshPainterSettings::GetAlphaToRoughnessParamName(int32 LayerIdx)
{
	return FName(FString::FromInt(LayerIdx) + AlphaToRoughnessPostfix);
}

FName UVSPMeshPainterSettings::GetPaintTintBackupParamName()
{
	return PaintTintBackupColorParam;
}

bool UVSPMeshPainterSettings::IsSupportToPaint(UMeshComponent* MeshComponent)
{
	return GetToPaintTextureFor(MeshComponent) != nullptr;
}

UTexture2D* UVSPMeshPainterSettings::GetToPaintTextureFor(UMeshComponent* MeshComponent) const
{
	UMaterialInstanceConstant* MeshMaterialInstance = GetMaterialInstanceConstant(MeshComponent);
	if (MeshComponent && MeshMaterialInstance)
	{
		UTexture* Texture;
		MeshMaterialInstance->GetTextureParameterValue(TEXT("MASK"), Texture, true);
		return Cast<UTexture2D>(Texture);
	}
	return nullptr;
}

UTexture2DArray* UVSPMeshPainterSettings::GetMaterialArrayFor(UMeshComponent* MeshComponent) const
{
	UMaterialInstanceConstant* MeshMaterialInstance = GetMaterialInstanceConstant(MeshComponent);
	if (MeshComponent && MeshMaterialInstance)
	{
		UTexture* Texture;
		MeshMaterialInstance->GetTextureParameterValue(MaterialArrayParamName, Texture, true);
		return Cast<UTexture2DArray>(Texture);
	}
	return nullptr;
}

TArray<int32> UVSPMeshPainterSettings::GetLayerToTextureMapFor(UMaterialInstanceConstant* Material) const
{
	TArray<int32> Out;
	float Value = 0;
	while (Material && Material->GetScalarParameterValue(GetLayerToTextureIdxParamName(Out.Num() + 1), Value, true))
	{
		Out.Add(FMath::RoundToInt(Value));
	}

	return Out;
}

TArray<FAlphaLayerData> UVSPMeshPainterSettings::GetAlphaToTextureMapFor(UMaterialInstanceConstant* Material) const
{
	TArray<FAlphaLayerData> Out;
	float TextureIndex = 0;
	int32 ParamIndex = Out.Num() + 1;
	if (Material)
	{
		while (Material->GetScalarParameterValue(GetAlphaToTextureIdxParamName(ParamIndex), TextureIndex, true))
		{

			float Roughness = 0;
			FLinearColor Channel = FLinearColor();
			FLinearColor TintColor = FLinearColor();

			Material->GetScalarParameterValue(GetAlphaToRoughnessParamName(ParamIndex), Roughness, true);
			Material->GetVectorParameterValue(GetAlphaToTextureChannelParamName(ParamIndex), Channel, true);


			FAlphaLayerData LayerData = FAlphaLayerData();
			LayerData.TextureIndex = FMath::RoundToInt(TextureIndex);
			LayerData.TextureChannel = Channel;
			LayerData.Roughness = Roughness;

			if (!Material->GetVectorParameterValue(GetAlphaToTintColorParamName(ParamIndex), TintColor, true))
				Material->GetVectorParameterValue(GetPaintTintBackupParamName(), TintColor, true);
			LayerData.TintColor = TintColor;

			Out.Add(LayerData);
			ParamIndex++;
		}
	}
	return Out;
}

UMaterial* UVSPMeshPainterSettings::GetMaskBakeMaterial() const
{
	return MaskBakeMaterial.LoadSynchronous();
}

UEditorUtilityWidgetBlueprint* UVSPMeshPainterSettings::GetUVWindowWidgetBlueprint()
{
	return UVWindowWidgetBlueprint.LoadSynchronous();
}

UMaterial* UVSPMeshPainterSettings::GetBrushFor(UMeshComponent* MeshComponent)
{
	FSupportedMaterialParam* SupportedMaterialParam = GetSupportedMaterialParamFor(MeshComponent);
	return SupportedMaterialParam ? SupportedMaterialParam->Brush.LoadSynchronous() : nullptr;
}

UMaterial* UVSPMeshPainterSettings::GetAccumulatorFor(UMeshComponent* MeshComponent)
{
	FSupportedMaterialParam* SupportedMaterialParam = GetSupportedMaterialParamFor(MeshComponent);
	return SupportedMaterialParam ? SupportedMaterialParam->Accumulator.LoadSynchronous() : nullptr;
}

UMaterial* UVSPMeshPainterSettings::GetUnwrapMaterialFor(UMeshComponent* MeshComponent)
{
	FSupportedMaterialParam* SupportedMaterialParam = GetSupportedMaterialParamFor(MeshComponent);
	return SupportedMaterialParam ? SupportedMaterialParam->UnwrapMaterial.LoadSynchronous() : nullptr;
}

UMaterialInstanceConstant* UVSPMeshPainterSettings::GetSupportedMaterialFor(UMeshComponent* MeshComponent)
{
	return IsSupportToPaint(MeshComponent) ? GetMaterialInstanceConstant(MeshComponent) : nullptr;
}

UMaterialInstanceConstant* UVSPMeshPainterSettings::GetMaterialInstanceConstant(UMeshComponent* MeshComponent) const
{
	if (MeshComponent)
	{
		TArray<UMaterialInterface*> MeshMaterials;
		MeshComponent->GetUsedMaterials(MeshMaterials, false);

		if (MeshMaterials.Num() == 1)
		{
			return Cast<UMaterialInstanceConstant>(MeshMaterials[0]);
		}
	}

	return nullptr;
}

FSupportedMaterialParam* UVSPMeshPainterSettings::GetSupportedMaterialParamFor(UMeshComponent* MeshComponent)
{
	if (MeshComponent)
	{
		UMaterialInstanceConstant* MeshMaterial = GetMaterialInstanceConstant(MeshComponent);
		if (MeshMaterial)
		{
			FSupportedMaterialParam* Sample = SupportedMaterials.FindByPredicate(
				[MeshMaterial](const FSupportedMaterialParam& Item)
				{
					UMaterialInterface* SupportedMaterial = Item.SupportedToPaint.LoadSynchronous();
					return SupportedMaterial && MeshMaterial->Parent == SupportedMaterial;
				});

			return Sample;
		}
	}

	return nullptr;
}
