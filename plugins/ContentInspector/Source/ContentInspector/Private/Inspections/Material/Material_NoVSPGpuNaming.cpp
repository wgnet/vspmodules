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

#include "Inspections/Material/Material_NoVSPGpuNaming.h"

namespace FNoVSPGpuNamingLocal
{
	const static FName ShortFailedDescription = TEXT("Material has no VSPGpu mark");
	const static FName FullFailedDescription = TEXT("Material has no VSPGpu mark");
	const static FName DefaultVSPGpuProfileName = TEXT("VSPGPUUndefinedName");
	const static bool bIsCriticalinspection = false;
}

void UMaterial_NoVSPGpuNaming::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_NoVSPGpuNaming;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_NoVSPGpuNaming::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const UMaterial* MaterialAsset = Cast<UMaterial>(InAssetData.GetAsset()))
		{
			FString Name;
			MaterialAsset->GetName(Name);
			OutInspectionResult.bInspectionFailed = Name.IsEmpty();

			OutInspectionResult.bCriticalInspection = FNoVSPGpuNamingLocal::bIsCriticalinspection;
			OutInspectionResult.ShortFailedDescription = FNoVSPGpuNamingLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FNoVSPGpuNamingLocal::FullFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
