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

#include "Inspections/Common/Common_RedirectorExist.h"

#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"

namespace FRedirectorExistLocal
{
	const static FName ShortFailedDescription = TEXT("Found redirector");
	const static bool bCriticalInspection = true;
}

UCommon_RedirectorExist::UCommon_RedirectorExist()
{
	bAssetLoadingRequired = false;
}

void UCommon_RedirectorExist::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_RedirectorExist;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_RedirectorExist::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		FString DescriptionResult;
		FString RedirectorName {};

		const IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

		TArray<FAssetIdentifier> ReferenceAssetIdentifiers;
		AssetRegistry.GetReferencers(
			InAssetData.PackageName,
			ReferenceAssetIdentifiers,
			UE::AssetRegistry::EDependencyCategory::Package);

		for (const FAssetIdentifier& AssetIdentifier : ReferenceAssetIdentifiers)
		{
			if (RedirectorName.IsEmpty())
			{
				const FString AssetPackageString = AssetIdentifier.PackageName.ToString();
				FString AssetObjectString = FString::Printf(
					TEXT("%s.%s"),
					*AssetPackageString,
					*FPackageName::GetLongPackageAssetName(AssetPackageString));

				const FName ObjectPath(*AssetObjectString);
				FAssetData DependentAsset = AssetRegistry.GetAssetByObjectPath(ObjectPath);

				if (RedirectorName.IsEmpty() && DependentAsset.IsRedirector())
				{
					RedirectorName = DependentAsset.AssetName.ToString();
				}
			}
		}

		if (!RedirectorName.IsEmpty())
		{
			DescriptionResult += FString::Printf(TEXT("Found redirector. Name: '%s' "), *RedirectorName);
		}

		OutInspectionResult.bInspectionFailed = !RedirectorName.IsEmpty();
		OutInspectionResult.ShortFailedDescription = FRedirectorExistLocal::ShortFailedDescription;
		OutInspectionResult.FullFailedDescription = *DescriptionResult;
		OutInspectionResult.bCriticalInspection = FRedirectorExistLocal::bCriticalInspection;

		OutInspectionResult.InspectionStatus = FInspectionResult::Completed;

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
