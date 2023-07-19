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

#include "Inspections/StaticMesh/StaticMesh_UseCustomizedCollision.h"

namespace FUseCustomizedCollisionLocal
{
	const static FName ShortFailedDescriptionBoth =
		TEXT("Fields 'Complex Collision Mesh' and 'Complex Collision Mesh' is incorrect.");
	const static FName FullFailedDescriptionBoth = TEXT(
		"Clear the 'Complex Collision Mesh' field and uncheck the 'Customized Collision' option  in the details panel in the asset settings");

	const static FName ShortFailedDescriptionCustomizedCollision = TEXT("Property 'Customized Collision' set as true.");
	const static FName FullFailedDescriptionCustomizedCollision =
		TEXT("Uncheck the 'Customized Collision' option in the details panel in the asset settings");

	const static FName ShortFailedDescriptionComplexCollisionMesh =
		TEXT("Field 'Complex Collision Mesh' contain something.");
	const static FName FullFailedDescriptionComplexCollisionMesh =
		TEXT("Clear the 'Complex Collision Mesh' field in the details panel in the asset settings");
	const static bool bCriticalInspection = false;
}

void UStaticMesh_UseCustomizedCollision::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_UseCustomizedCollision;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_UseCustomizedCollision::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto StaticMeshAsset = Cast<UStaticMesh>(InAssetData.GetAsset()))
		{
			if (StaticMeshAsset->bCustomizedCollision && IsValid(StaticMeshAsset->ComplexCollisionMesh))
			{
				OutInspectionResult.bInspectionFailed = true;
				OutInspectionResult.ShortFailedDescription = FUseCustomizedCollisionLocal::ShortFailedDescriptionBoth;
				OutInspectionResult.FullFailedDescription = FUseCustomizedCollisionLocal::FullFailedDescriptionBoth;
			}

			else if (StaticMeshAsset->bCustomizedCollision)
			{
				OutInspectionResult.bInspectionFailed = true;
				OutInspectionResult.ShortFailedDescription =
					FUseCustomizedCollisionLocal::ShortFailedDescriptionCustomizedCollision;
				OutInspectionResult.FullFailedDescription =
					FUseCustomizedCollisionLocal::FullFailedDescriptionCustomizedCollision;
			}

			else if (IsValid(StaticMeshAsset->ComplexCollisionMesh))
			{
				OutInspectionResult.bInspectionFailed = true;
				OutInspectionResult.ShortFailedDescription =
					FUseCustomizedCollisionLocal::ShortFailedDescriptionComplexCollisionMesh;
				OutInspectionResult.FullFailedDescription =
					FUseCustomizedCollisionLocal::FullFailedDescriptionComplexCollisionMesh;
			}

			OutInspectionResult.bCriticalInspection = FUseCustomizedCollisionLocal::bCriticalInspection;
			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
