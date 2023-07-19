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


#include "InspectionsManager.h"
#include "Algo/Copy.h"
#include "Algo/Transform.h"
#include "Async/Async.h"
#include "ContentInspectorSettings.h"
#include "ContentInspectorStyle.h"
#include "Inspections/InspectionBase.h"

#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

void UInspectionsManager::SendInspectionsRequest(const FAssetData& InAssetData, bool bAsynchronous /*false*/)
{
	if (PathInExcludeFolder(InAssetData))
	{
		return;
	}
	TArray<TSubclassOf<UInspectionBase>> AvailableInspectionsForAsset {};
	GetInspectionsArrayFromConfig(InAssetData.AssetClass, AvailableInspectionsForAsset);

	if (AvailableInspectionsForAsset.Num() == 0)
	{
		return;
	}

	const FName& AssetPackageName = InAssetData.PackageName;

	//	If the asset is being inspected for the first time
	if (!InspectionResults.Contains(AssetPackageName))
	{
		//	Create new element in map and reserve memory for inspection results
		TArray<FInspectionResult> Inspections;
		Inspections.Reserve(AvailableInspectionsForAsset.Num());
		InspectionResults.Add(AssetPackageName, Inspections);

		//	Adding elements for future inspection results
		for (TSubclassOf<UInspectionBase> AvailableInspectionClass : AvailableInspectionsForAsset)
		{
			if (const UClass* InspectionClass = AvailableInspectionClass.Get())
			{
				TArray<FInspectionResult>& ExistingInspectionsResultsForAsset =
					InspectionResults.FindChecked(AssetPackageName);

				FInspectionResult IncompleteInspectionResult;
				IncompleteInspectionResult.AssetPackageName = AssetPackageName;
				IncompleteInspectionResult.InspectionClass = InspectionClass;

				ExistingInspectionsResultsForAsset.Add(IncompleteInspectionResult);
			}
		}
	}

	//	Create  inspection task for each type from config
	for (TSubclassOf<UInspectionBase> AvailableInspectionClass : AvailableInspectionsForAsset)
	{
		if (const UClass* InspectionClass = AvailableInspectionClass.Get())
		{
			TArray<FInspectionResult>& ExistingInspectionsResultsForAsset =
				InspectionResults.FindChecked(AssetPackageName);

			auto IsInspectionNeedStarted = [InspectionClass](const FInspectionResult& InInspectionResult)
			{
				//	Do not create new inspection task if same in progress
				return InInspectionResult.InspectionClass == InspectionClass
					&& InInspectionResult.InspectionStatus != FInspectionResult::InProgress;
			};

			const int32 FoundInspectionIndex =
				ExistingInspectionsResultsForAsset.FindLastByPredicate(IsInspectionNeedStarted);

			if (FoundInspectionIndex != INDEX_NONE)
			{
				FInspectionResult& InspectionResultForUpdate = ExistingInspectionsResultsForAsset[FoundInspectionIndex];

				if (auto InspectionType = InspectionClass->GetDefaultObject<UInspectionBase>())
					InspectionType->CreateInspectionTask(
						InAssetData,
						InspectionResultForUpdate,
						bAsynchronous,
						bCommandLetMode);
			}
		}
	}
}

void UInspectionsManager::GetInspectionsArrayFromConfig(
	const FName& InAssetClassName,
	TArray<TSubclassOf<UInspectionBase>>& OutInspections) const
{
	const int32 IndexResult = GetInspectionsStructFromConfig(InAssetClassName);
	if (IndexResult != INDEX_NONE)
	{
		if (const UContentInspectorSettings* ContentInspectorSettings = UContentInspectorSettings::Get())
		{
			const TArray<FInspectionsForClass>& AllClassesAndInspections =
				ContentInspectorSettings->GetClassesAndInspections();
			const FInspectionsForClass& InspectionsForCurrentClass = AllClassesAndInspections[IndexResult];

			OutInspections = InspectionsForCurrentClass.Inspections;
		}
	}
}

int32 UInspectionsManager::GetInspectionsStructFromConfig(const FName& InAssetClassName) const
{
	int32 IndexResult = INDEX_NONE;

	if (IsSafeTryingLoadAsset())
	{
		if (const UContentInspectorSettings* ContentInspectorSettings = UContentInspectorSettings::Get())
		{
			auto IsInspectionsForClassAvailable = [InAssetClassName](const FInspectionsForClass& InInspectionsForClass)
			{
				if (const UObject* AssetClass = InInspectionsForClass.ClassForInspection.TryLoad())
				{
					return InAssetClassName == AssetClass->GetFName();
				}

				return false;
			};

			IndexResult = ContentInspectorSettings->GetClassesAndInspections().FindLastByPredicate(
				IsInspectionsForClassAvailable);
		}
	}

	return IndexResult;
}

bool UInspectionsManager::IsNeedLoadAsset(const FName& InAssetClassName) const
{
	TArray<TSubclassOf<UInspectionBase>> AllInspectionsForAsset;
	GetInspectionsArrayFromConfig(InAssetClassName, AllInspectionsForAsset);

	auto IsNeedLoadAsset = [](const TSubclassOf<UInspectionBase>& InInspectionClass)
	{
		return InInspectionClass.GetDefaultObject()->bAssetLoadingRequired;
	};

	const int32 FoundInspectionIndex = AllInspectionsForAsset.FindLastByPredicate(IsNeedLoadAsset);
	if (FoundInspectionIndex == INDEX_NONE)
	{
		return false;
	}
	return true;
}

bool UInspectionsManager::HaveFailedInspections(const FName& InAssetPackageName, int32& OutCriticalInspectionFailed)
	const
{
	bool bResult = false;
	OutCriticalInspectionFailed = 0;

	TArray<FInspectionResult> AssetInspectionResults;
	if (GetInspectionResults(InAssetPackageName, AssetInspectionResults))
	{
		auto IsInspectionFailed = [](const FInspectionResult& InInspectionResult)
		{
			return InInspectionResult.bInspectionFailed;
		};

		const int32& FailedInspectionIndex = AssetInspectionResults.FindLastByPredicate(IsInspectionFailed);
		if (FailedInspectionIndex != INDEX_NONE)
		{
			bResult = true;
		}

		//	If failed inspections exist, check on critical
		if (bResult)
		{
			for (const FInspectionResult& InspectionResult : AssetInspectionResults)
			{
				if (InspectionResult.bInspectionFailed && InspectionResult.bCriticalInspection)
					OutCriticalInspectionFailed++;
			}
		}
	}
	return bResult;
}

EVisibility UInspectionsManager::GetIndicatorVisibility(FName InAssetPackageName) const
{
	EVisibility VisibilityResult = EVisibility::Collapsed;

	const int32& InspectionsStructIndex = GetInspectionsStructFromConfig(InAssetPackageName);
	TArray<TSubclassOf<UInspectionBase>> OutInspections;
	GetInspectionsArrayFromConfig(InAssetPackageName, OutInspections);

	if (InspectionsStructIndex != INDEX_NONE && OutInspections.Num() > 0)
	{
		VisibilityResult = EVisibility::Visible;
	}

	return VisibilityResult;
}

const FSlateBrush* UInspectionsManager::GetIndicatorBrush(FName InAssetPackageName) const
{
	FName Brush = TEXT("ContentInspector.BookmarkGray");

	if (AllInspectionsCompleted(InAssetPackageName))
	{
		Brush = TEXT("ContentInspector.BookmarkGreen");

		int32 CriticalFailed = 0;
		const bool bHaveFailedInspections = HaveFailedInspections(InAssetPackageName, CriticalFailed);

		if (bHaveFailedInspections)
		{
			Brush = TEXT("ContentInspector.BookmarkOrange");
		}

		if (CriticalFailed > 0)
		{
			Brush = TEXT("ContentInspector.BookmarkRed");
		}
	}

	return FContentInspectorStyle::Get().GetBrush(Brush);
}

FText UInspectionsManager::GetShortDescription(FName InAssetPackageName) const
{
	FText ShortDescriptionResult = FText::FromString("The inspection is still pending");

	TArray<FInspectionResult> AssetInspectionResults;
	if (GetInspectionResults(InAssetPackageName, AssetInspectionResults))
	{
		auto IsInspectionFailed = [](const FInspectionResult& InInspectionResult)
		{
			return InInspectionResult.bInspectionFailed;
		};

		auto TransformToDescription = [](const FInspectionResult& InInspectionResult)
		{
			return InInspectionResult.ShortFailedDescription;
		};

		TArray<FName> FailedShortDescriptions {};
		Algo::TransformIf(AssetInspectionResults, FailedShortDescriptions, IsInspectionFailed, TransformToDescription);

		if (FailedShortDescriptions.Num() > 0)
		{
			ShortDescriptionResult = {};
			FString ResultString = ShortDescriptionResult.ToString();



			for (FName FailedShortDescription : FailedShortDescriptions)
			{
				ResultString += "- ";
				ResultString += FailedShortDescription.ToString() += "\n";
			}
			ShortDescriptionResult = FText::FromString(ResultString);
		}
		else
		{
			ShortDescriptionResult = FText::FromString("Everything is OK");
		}
	}

	return ShortDescriptionResult;
}

bool UInspectionsManager::GetInspectionResults(
	const FName& InAssetPackageName,
	TArray<FInspectionResult>& OutInspectionResults) const
{
	if (InspectionResults.Contains(InAssetPackageName))
	{
		OutInspectionResults = InspectionResults[InAssetPackageName];
		return true;
	}

	return false;
}

bool UInspectionsManager::AllInspectionsCompleted(const FName& InAssetPackageName) const
{
	bool bResult;

	if (InspectionResults.Contains(InAssetPackageName))
	{
		const TArray<FInspectionResult>& InspectionResultsForAsset = InspectionResults[InAssetPackageName];

		auto IsInspectionNotCompleted = [](const FInspectionResult& InspectionResult)
		{
			return InspectionResult.InspectionStatus != FInspectionResult::Completed;
		};

		const bool bHasNonCompletedInspections =
			InspectionResultsForAsset.ContainsByPredicate(IsInspectionNotCompleted);
		bResult = !bHasNonCompletedInspections;
	}
	else
	{
		bResult = false;
	}

	return bResult;
}

void UInspectionsManager::AssetAsyncLoadComplete(FAssetData InAssetData)
{
	SendInspectionsRequest(InAssetData, true);
}

bool UInspectionsManager::IsSafeTryingLoadAsset() const
{
	const bool bGarbageCollectingInProgress { IsInGameThread() && IsGarbageCollecting() };
	return !(GIsSavingPackage || bGarbageCollectingInProgress);
}

void UInspectionsManager::StartAssetAsyncLoad(const FAssetData& InAssetData)
{
	if (UAssetManager* Manager = UAssetManager::GetIfValid())
	{
		FStreamableManager& StreamableManager = Manager->GetStreamableManager();
		StreamableManager.RequestAsyncLoad(
			InAssetData.ToSoftObjectPath(),
			FStreamableDelegate::CreateUObject(this, &UInspectionsManager::AssetAsyncLoadComplete, InAssetData));
	}
}

bool UInspectionsManager::PathInExcludeFolder(const FAssetData& InAssetData)
{
	bool bInExcludeFolder = false;

	if (const UContentInspectorSettings* ContentInspectorSettings = UContentInspectorSettings::Get())
	{
		const FString AssetPath = InAssetData.ToSoftObjectPath().ToString();
		for (const FDirectoryPath& ExcludePath : ContentInspectorSettings->GetInspectExcludeFolders())
		{
			if (AssetPath.Contains(ExcludePath.Path))
			{
				bInExcludeFolder = true;
				break;
			}
		}
	}

	return bInExcludeFolder;
}

void UInspectionsManager::SetCommandLetMode(bool bMode)
{
	bCommandLetMode = bMode;
}

TArray<TSubclassOf<UInspectionBase>> UInspectionsManager::ClearInvalidInspections(
	const TArray<TSubclassOf<UInspectionBase>> Inspections) const
{
	TArray<TSubclassOf<UInspectionBase>> ValidInspections;
	for (TSubclassOf<UInspectionBase> InspectionBase : Inspections)
	{
		if (IsValid(InspectionBase))
		{
			ValidInspections.Add(InspectionBase);
		}
	}
	return ValidInspections;
}
