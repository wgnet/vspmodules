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
#include "Inspections/Sound/SoundWave_LoadingBehaviorOverride.h"
#include "AssetRegistryModule.h"
#include "ContentInspectorSettings.h"
#include "Internationalization/PackageLocalizationUtil.h"
#include "Runtime/Engine/Classes/Sound/SoundWaveLoadingBehavior.h"

#include "Sound/SoundWave.h"

namespace FCheckLoadingBehavior
{
	const static FName ShortFailedDescription = TEXT("Incorrect Loading Behavior Overridden");
	const static FName FullFailedDescriptionRetainOnLoad =
		TEXT("Incorrect Loading Behavior Overridden. Set 'Loading Behavior Override' to 'Retain On Load'");
	const static FName FullFailedDescriptionForceInline =
		TEXT("Incorrect Loading Behavior Overridden. Set 'Loading Behavior Override' to 'Force Inline'");

	const static bool bCriticalInspection = true;
}

void USoundWave_LoadingBehaviorOverride::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FSound_LoadingBehaviorOverride;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FSound_LoadingBehaviorOverride::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		FString SourceObjectPath;
		CheckLocalization(InAssetData, SourceObjectPath);

		const FAssetRegistryModule& AssetRegistryModule =
			FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		const FAssetData LocalizedAsset = AssetRegistryModule.Get().GetAssetByObjectPath(*SourceObjectPath);

		if (const USoundWave* SoundWave = Cast<USoundWave>(InAssetData.GetAsset()))
		{
			FString SoundWavePath;
			if (SourceObjectPath.IsEmpty())
			{
				SoundWavePath = SoundWave->GetPathName();
			}
			else
			{
				SoundWavePath = LocalizedAsset.GetPackage()->GetPathName();
			}

			bool bInRetainOnLoadInExclude = false;

			const TArray<FDirectoryPath> RetainOnLoadIncludePathsArray =
				UContentInspectorSettings::Get()->GetRetainOnLoadIncludePathsArray();
			for (const FDirectoryPath& DirectoryPath : RetainOnLoadIncludePathsArray)
			{
				if (SoundWavePath.Find(DirectoryPath.Path) >= 0)
				{
					bInRetainOnLoadInExclude = true;
					break;
				}
			}

			bool bLoadingBehaviorOverriddenFailed;
			if (bInRetainOnLoadInExclude)
			{
				bLoadingBehaviorOverriddenFailed =
					SoundWave->LoadingBehavior != ESoundWaveLoadingBehavior::RetainOnLoad;
			}
			else
			{
				bLoadingBehaviorOverriddenFailed = SoundWave->LoadingBehavior != ESoundWaveLoadingBehavior::ForceInline;
			}

			OutInspectionResult.bInspectionFailed = bLoadingBehaviorOverriddenFailed;

			OutInspectionResult.bCriticalInspection = FCheckLoadingBehavior::bCriticalInspection;
			OutInspectionResult.ShortFailedDescription = FCheckLoadingBehavior::ShortFailedDescription;
			if (bInRetainOnLoadInExclude)
			{
				OutInspectionResult.FullFailedDescription = FCheckLoadingBehavior::FullFailedDescriptionRetainOnLoad;
			}
			else
			{
				OutInspectionResult.FullFailedDescription = FCheckLoadingBehavior::FullFailedDescriptionForceInline;
			}

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}

void FSound_LoadingBehaviorOverride::CheckLocalization(const FAssetData& Asset, FString& SourceObjectPath)
{
	const FString ObjectPath = Asset.ObjectPath.ToString();

	TSet<FName> SourceAssetsState;
	if (FPackageName::IsLocalizedPackage(ObjectPath))
	{
		FString LocalObjectPath;
		if (FPackageLocalizationUtil::ConvertLocalizedToSource(ObjectPath, LocalObjectPath))
		{
			SourceObjectPath = LocalObjectPath;
		}
	}
}
