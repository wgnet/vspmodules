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

#include "Inspections/MaterialInstance/MaterialInstance_NoPhysicalMaterial.h"

#include "Materials/MaterialInstance.h"

namespace FNoPhysicalMaterialLocal
{
	const static FName ShortFailedDescription = TEXT("No physical material");
	const static FName FullFailedDescription = TEXT("No physical material. Assign it if required");
	const static FName DefaultPhysMaterial = "DefaultPhysicalMaterial";
}

void UMaterialInstance_NoPhysicalMaterial::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_NoPhysicalMaterial;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_NoPhysicalMaterial::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto MaterialInstanceAsset = Cast<UMaterialInstance>(InAssetData.GetAsset()))
		{
			if (UPhysicalMaterial* Material = MaterialInstanceAsset->GetPhysicalMaterial())
				OutInspectionResult.bInspectionFailed =
					Material->GetFName() == FNoPhysicalMaterialLocal::DefaultPhysMaterial;

			OutInspectionResult.ShortFailedDescription = FNoPhysicalMaterialLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FNoPhysicalMaterialLocal::FullFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
