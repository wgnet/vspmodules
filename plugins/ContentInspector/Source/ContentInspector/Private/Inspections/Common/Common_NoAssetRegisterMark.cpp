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

#include "Inspections/Common/Common_NoAssetRegisterMark.h"

#include "ContentInspectorSettings.h"

namespace FNoAssetRegisterMarkLocal
{
	const static FString ValidatePath = "Game/Environment";
	const static FName ShortFailedDescription = TEXT("No Asset register mark");
	const static FName FullFailedDescription =
		TEXT("No Asset register mark. Need assign an asset register mark and select a primary tag");
	const static bool bCriticalInspection = true;
}

void UCommon_NoAssetRegisterMark::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	if (UContentInspectorSettings::Get()->GetAssetRegisterMarkClass().IsValid())
	{
		auto InspectionTask = new FInspectionTask_NoAssetRegisterMark;
		InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
	}
}

void FInspectionTask_NoAssetRegisterMark::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (InAssetData.GetFullName().Contains(FNoAssetRegisterMarkLocal::ValidatePath))
		{
			if (const auto AssetRegisterMarkClass =
					Cast<UClass>(UContentInspectorSettings::Get()->GetAssetRegisterMarkClass().TryLoad()))
			{
				if (const auto AssetUserDataInterface = Cast<IInterface_AssetUserData>(InAssetData.GetAsset()))
				{
					OutInspectionResult.bInspectionFailed =
						!AssetUserDataInterface->GetAssetUserDataOfClass(AssetRegisterMarkClass);
				}
			}

			OutInspectionResult.ShortFailedDescription = FNoAssetRegisterMarkLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FNoAssetRegisterMarkLocal::FullFailedDescription;
			OutInspectionResult.bCriticalInspection = FNoAssetRegisterMarkLocal::bCriticalInspection;
		}
		else
		{
			OutInspectionResult.bInspectionFailed = false;
		}

		OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
