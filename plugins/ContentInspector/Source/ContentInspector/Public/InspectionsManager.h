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

#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"

#include "Inspections/InspectionBase.h"
#include "InspectionsManager.generated.h"

//	Launches on-demand inspections. Stores inspection results and provides them upon request
UCLASS()
class CONTENTINSPECTOR_API UInspectionsManager : public UObject
{
	GENERATED_BODY()

public:
	void SendInspectionsRequest(const FAssetData& InAssetData, bool bAsynchronous = false);
	EVisibility GetIndicatorVisibility(FName InAssetPackageName) const;
	const FSlateBrush* GetIndicatorBrush(FName InAssetPackageName) const;
	FText GetShortDescription(FName InAssetPackageName) const;
	void GetInspectionsArrayFromConfig(
		const FName& InAssetClassName,
		TArray<TSubclassOf<UInspectionBase>>& OutInspections) const;
	int32 GetInspectionsStructFromConfig(const FName& InAssetClassName) const;

	//	Returns true if there is at least one inspection that requires the asset to be loaded
	bool IsNeedLoadAsset(const FName& InAssetClassName) const;
	bool HaveFailedInspections(const FName& InAssetPackageName, int32& OutCriticalInspectionFailed) const;
	bool GetInspectionResults(const FName& InAssetPackageName, TArray<FInspectionResult>& OutInspectionResults) const;

	bool AllInspectionsCompleted(const FName& InAssetPackageName) const;

	void StartAssetAsyncLoad(const FAssetData& InAssetData);
	void AssetAsyncLoadComplete(FAssetData InAssetData);
	bool IsSafeTryingLoadAsset() const;
	bool PathInExcludeFolder(const FAssetData& InAssetData);
	void SetCommandLetMode(bool bMode);
	TArray<TSubclassOf<UInspectionBase>> ClearInvalidInspections(
		const TArray<TSubclassOf<UInspectionBase>> Inspections) const;

private:
	bool bCommandLetMode = false;
	TMap<FName, TArray<FInspectionResult>> InspectionResults;
};
