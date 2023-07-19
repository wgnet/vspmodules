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

#include "Inspections/MaterialInstance/MaterialInstance_Warnings.h"

#include "Materials/MaterialExpressionRuntimeVirtualTextureSampleParameter.h"
#include "Materials/MaterialInstance.h"

namespace FMIWarningsLocal
{
	const static FName ShortFailedDescription = TEXT("Material Instance has warnings");
	const static FName FullFailedDescription = TEXT("Material Instance has warnings. Fix them to avoid visual bugs");
}

void UMaterialInstance_MIWarnings::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_MIWarnings;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_MIWarnings::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (UMaterialInstance* MaterialInstanceAsset = Cast<UMaterialInstance>(InAssetData.GetAsset()))
		{
			OutInspectionResult.bInspectionFailed = false;
			for (FTextureParameterValue TextureParam : MaterialInstanceAsset->TextureParameterValues)
			{
				if (TextureParam.ParameterValue && TextureParam.ExpressionGUID.IsValid())
				{
					EMaterialSamplerType SamplerType =
						UMaterialExpressionTextureBase::GetSamplerTypeForTexture(TextureParam.ParameterValue);
					UMaterialExpressionTextureSampleParameter* Expression = MaterialInstanceAsset->GetMaterial()
						->FindExpressionByGUID<UMaterialExpressionTextureSampleParameter>(TextureParam.ExpressionGUID);

					FString ErrorMessage;
					if (Expression && !Expression->TextureIsValid(TextureParam.ParameterValue, ErrorMessage))
					{
						OutInspectionResult.bInspectionFailed = true;
					}
					else
					{
						UEnum* SamplerTypeEnum = StaticEnum<EMaterialSamplerType>();
						if (Expression && Expression->SamplerType != SamplerType)
						{
							FString SamplerTypeDisplayName =
								SamplerTypeEnum->GetDisplayNameTextByValue(Expression->SamplerType).ToString();

							OutInspectionResult.bInspectionFailed = true;
						}
						else if (
							Expression
							&& ((Expression->SamplerType == (EMaterialSamplerType)TC_Normalmap
								 || Expression->SamplerType == (EMaterialSamplerType)TC_Masks)
								&& TextureParam.ParameterValue->SRGB))
						{
							FString SamplerTypeDisplayName =
								SamplerTypeEnum->GetDisplayNameTextByValue(Expression->SamplerType).ToString();

							OutInspectionResult.bInspectionFailed = true;
						}
					}
				}
			}
			for (FRuntimeVirtualTextureParameterValue VTParam :
				 MaterialInstanceAsset->RuntimeVirtualTextureParameterValues)
			{
				if (VTParam.ParameterValue && VTParam.ExpressionGUID.IsValid())
				{
					UEnum* MaterialTypeEnum = StaticEnum<ERuntimeVirtualTextureMaterialType>();
					check(MaterialTypeEnum);
					UMaterialExpressionRuntimeVirtualTextureSampleParameter* Expression =
						MaterialInstanceAsset->GetMaterial()
							->FindExpressionByGUID<UMaterialExpressionRuntimeVirtualTextureSampleParameter>(
								VTParam.ExpressionGUID);
					if (!Expression)
					{
						OutInspectionResult.bInspectionFailed = true;
					}
					else if (Expression && Expression->MaterialType != VTParam.ParameterValue->GetMaterialType())
					{
						OutInspectionResult.bInspectionFailed = true;
					}
					else if (
						Expression
						&& Expression->bSinglePhysicalSpace != VTParam.ParameterValue->GetSinglePhysicalSpace())
					{
						OutInspectionResult.bInspectionFailed = true;
					}
					else if (Expression && Expression->bAdaptive != VTParam.ParameterValue->GetAdaptivePageTable())
					{
						OutInspectionResult.bInspectionFailed = true;
					}
				}
			}
			OutInspectionResult.ShortFailedDescription = FMIWarningsLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FMIWarningsLocal::FullFailedDescription;
			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
