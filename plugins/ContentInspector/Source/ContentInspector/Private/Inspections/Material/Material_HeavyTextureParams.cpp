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

#include "Inspections/Material/Material_HeavyTextureParams.h"

namespace FHeavyTextureParamsLocal
{
	const static FName ShortFailedDescription = TEXT("Large texture parameter default value");
	const static FName FullFailedDescription =
		TEXT("Large texture parameter default value. Critical to replace it for a small dummy texture");
	const static bool bCriticalInspection = false;

	const static int32 MaximumTextureSize = 262144;
}

void UMaterial_HeavyTextureParams::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_HeavyTextureParams;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_HeavyTextureParams::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto MaterialAsset = Cast<UMaterial>(InAssetData.GetAsset()))
		{
			TArray<FMaterialParameterInfo> TextureParameters;
			TArray<FGuid> ParamIDs;
			MaterialAsset->GetAllTextureParameterInfo(TextureParameters, ParamIDs);
			OutInspectionResult.bInspectionFailed = false;
			for (FMaterialParameterInfo TexParamInfo : TextureParameters)
			{
				UTexture* TextureAsset;
				MaterialAsset->GetTextureParameterDefaultValue(TexParamInfo, TextureAsset);
				if (IsValid(TextureAsset))
				{
					if (TextureAsset->CalcTextureMemorySizeEnum(TMC_MAX) > FHeavyTextureParamsLocal::MaximumTextureSize)
					{
						OutInspectionResult.bInspectionFailed = true;
					}
				}
			}

			OutInspectionResult.bCriticalInspection = FHeavyTextureParamsLocal::bCriticalInspection;
			OutInspectionResult.ShortFailedDescription = FHeavyTextureParamsLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FHeavyTextureParamsLocal::FullFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
