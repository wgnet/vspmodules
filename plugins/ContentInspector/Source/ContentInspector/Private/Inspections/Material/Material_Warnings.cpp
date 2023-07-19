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

#include "Inspections/Material/Material_Warnings.h"

namespace FMaterialWarningsLocal
{
	const static FName ShortFailedDescription = TEXT("Material has warnings");
	const static FName FullFailedDescription = TEXT("Material has warnings, please fix them to avoid visual bugs");
}

void UMaterial_MWarnings::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_MWarnings;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_MWarnings::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto MaterialAsset = Cast<UMaterial>(InAssetData.GetAsset()))
		{
			if (FMaterialResource* MaterialResource =
					MaterialAsset->GetMaterialResource(GEditor->GetEditorWorldContext().World()->FeatureLevel))
			{
				OutInspectionResult.bInspectionFailed = MaterialResource->GetCompileErrors().Num() > 0;
			}

			OutInspectionResult.ShortFailedDescription = FMaterialWarningsLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FMaterialWarningsLocal::FullFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
