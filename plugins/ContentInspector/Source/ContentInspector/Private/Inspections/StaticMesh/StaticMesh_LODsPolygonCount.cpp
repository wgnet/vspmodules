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
#include "Inspections/StaticMesh/StaticMesh_LODsPolygonCount.h"
#include "ContentInspectorSettings.h"

namespace FLODsPolygonCountLocal
{
	const static FName ShortFailedDescription = TEXT("Incorrect LODs Polycount");
	const static FName FullFailedDescription = TEXT("LODs polycount must be 40 or less percent of the previous");
	const static bool bCriticalInspection = false;
}

void UStaticMesh_LODsPolygonCount::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_LODsPolygonCount;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_LODsPolygonCount::DoInspection(
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
			if (LODs == 1)
			{
				OutInspectionResult.bInspectionFailed = false;
			}
			else
			{
				for (int32 LodIdx = 1; LodIdx < LODs; LodIdx++)
				{
					const float MinimumPolyCountDifference =
						UContentInspectorSettings::Get()->GetMinimumPolyCountDifferenceValue();
					const int32 TrisCurrent = StaticMeshAsset->GetLODForExport(LodIdx).GetNumTriangles();
					const int32 TrisPreview = StaticMeshAsset->GetLODForExport(LodIdx - 1).GetNumTriangles();
					const float Percentage = TrisCurrent / TrisPreview * 100;
					if (Percentage > MinimumPolyCountDifference)
					{
						OutInspectionResult.bInspectionFailed = true;
						OutInspectionResult.bCriticalInspection = FLODsPolygonCountLocal::bCriticalInspection;
						OutInspectionResult.ShortFailedDescription = FLODsPolygonCountLocal::ShortFailedDescription;
						OutInspectionResult.FullFailedDescription = FLODsPolygonCountLocal::FullFailedDescription;

						break;
					}
				}
			}
		}

		OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
