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

#include "Inspections/InspectionBase.h"
#include "Widgets/SCompoundWidget.h"

class SInspectionResult : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SInspectionResult) : _AssetData(), _InspectionResults(), _AssetThumbnail()
	{
	}

	SLATE_ARGUMENT(FAssetData, AssetData)
	SLATE_ARGUMENT(TArray<FInspectionResult>, InspectionResults)
	SLATE_ARGUMENT(TSharedPtr<SWidget>, AssetThumbnail)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	bool IsOpenURLButtonEnable(TSoftClassPtr<UInspectionBase> InInspectionClass) const;
	FReply OpenConfluenceURL(TSoftClassPtr<UInspectionBase> InInspectionClass) const;
	FText GetButtonTooltipText(TSoftClassPtr<UInspectionBase> InInspectionClass) const;
	const FSlateBrush* GetBackgroundColor(bool bInCriticalFailed) const;

	FReply FocusContentBrowserOnAsset(FAssetData InAssetData);
};
