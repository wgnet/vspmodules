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

#include "Inspections/StaticMesh/StaticMesh_UseMaterialInstances.h"
#include "Algo/Copy.h"

namespace FUseMaterialInstancesLocal
{
	const static FName ShortFailedDescription = TEXT("Used material instead of material instance");
}

void UStaticMesh_UseMaterialInstances::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_UseMaterialInstances;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_UseMaterialInstances::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto StaticMeshAsset = Cast<UStaticMesh>(InAssetData.GetAsset()))
		{
			const TArray<FStaticMaterial>& AllMaterials = StaticMeshAsset->GetStaticMaterials();
			TArray<FStaticMaterial> BadMaterials;

			auto IsBadMaterial = [](const FStaticMaterial& InMaterial)
			{
				if (InMaterial.MaterialInterface)
					return InMaterial.MaterialInterface->IsA<UMaterial>();

				return true;
			};

			Algo::CopyIf(AllMaterials, BadMaterials, IsBadMaterial);

			if (BadMaterials.Num() > 0)
			{
				OutInspectionResult.bInspectionFailed = true;
				OutInspectionResult.FullFailedDescription = *FString::Printf(
					TEXT("Used %d materials. Need to replace them with material instance"),
					BadMaterials.Num());
			}
			else
				OutInspectionResult.bInspectionFailed = false;

			OutInspectionResult.ShortFailedDescription = FUseMaterialInstancesLocal::ShortFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
