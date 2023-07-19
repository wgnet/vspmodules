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

#include "Inspections/StaticMesh/StaticMesh_NoLODs.h"

namespace FNoLODsLocal
{
	const static FName ShortFailedDescription = TEXT("No LODs");
	const static FName FullFailedDescription = TEXT("No LODs. It is necessary to create additional LODs");
	const static bool bCriticalInspection = false;
}

void UStaticMesh_NoLODs::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_NoLODs;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_NoLODs::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto StaticMeshAsset = Cast<UStaticMesh>(InAssetData.GetAsset()))
		{
			const int32 LODs = StaticMeshAsset->GetNumLODs();

			OutInspectionResult.bInspectionFailed = LODs < 2;
			OutInspectionResult.bCriticalInspection = FNoLODsLocal::bCriticalInspection;
			OutInspectionResult.ShortFailedDescription = FNoLODsLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FNoLODsLocal::FullFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
