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
#include "Inspections/Common/Common_InvalidLink.h"

#include "ContentInspectorSettings.h"

#include "AssetRegistryModule.h"
#include "IAssetRegistry.h"

namespace FInvalidLinkLocal
{
	const static FName ShortFailedDescription = TEXT("Found Invalid References");
	const static bool bCriticalInspection = true;
}

UCommon_InvalidLink::UCommon_InvalidLink()
{
	bAssetLoadingRequired = false;
}

void UCommon_InvalidLink::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_InvalidLink;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_InvalidLink::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		const IAssetRegistry& AssetRegistry =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry").Get();

		TArray<FAssetIdentifier> ReferenceAssetIdentifiers;
		AssetRegistry.GetDependencies(
			InAssetData.PackageName,
			ReferenceAssetIdentifiers,
			UE::AssetRegistry::EDependencyCategory::All);
		TArray<FString> InvalidReferences {};

		for (const FAssetIdentifier& AssetIdentifier : ReferenceAssetIdentifiers)
		{
			if (AssetIdentifier.PackageName.ToString().StartsWith("/Game/"))
			{
				const FString AssetPackageString = AssetIdentifier.PackageName.ToString();
				FString AssetObjectString = FString::Printf(
					TEXT("%s.%s"),
					*AssetPackageString,
					*FPackageName::GetLongPackageAssetName(AssetPackageString));

				FAssetData DependentAsset = AssetRegistry.GetAssetByObjectPath(*AssetObjectString);
				if (!DependentAsset.IsValid())
				{
					InvalidReferences.Add(AssetObjectString);
				}
			}
		}

		OutInspectionResult.bInspectionFailed = InvalidReferences.Num() > 0 ? true : false;
		OutInspectionResult.bCriticalInspection = FInvalidLinkLocal::bCriticalInspection;

		if (OutInspectionResult.bInspectionFailed)
		{
			OutInspectionResult.ShortFailedDescription = FInvalidLinkLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = *FString::Printf(
				TEXT("Asset have %d invalid references. First invalid reference %s"),
				InvalidReferences.Num(),
				*InvalidReferences[0]);
		}

		OutInspectionResult.InspectionStatus = FInspectionResult::Completed;

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
