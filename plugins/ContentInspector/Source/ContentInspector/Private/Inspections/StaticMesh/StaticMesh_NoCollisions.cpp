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

#include "Inspections/StaticMesh/StaticMesh_NoCollisions.h"

namespace FNoCollisionsLocal
{
	const static FName ShortFailedDescription = TEXT("No collision primitives");
	const static FName FullFailedDescription = TEXT("No collision primitives. Create if needed");
}

void UStaticMesh_NoCollisions::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_NoCollisions;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_NoCollisions::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto StaticMeshAsset = Cast<UStaticMesh>(InAssetData.GetAsset()))
		{
			if (StaticMeshAsset->GetBodySetup())
			{
				const int32 CollisionPrimitives = StaticMeshAsset->GetBodySetup()->AggGeom.GetElementCount();

				OutInspectionResult.bInspectionFailed = CollisionPrimitives == 0;
				OutInspectionResult.ShortFailedDescription = FNoCollisionsLocal::ShortFailedDescription;
				OutInspectionResult.FullFailedDescription = FNoCollisionsLocal::FullFailedDescription;

				OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
			}
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
