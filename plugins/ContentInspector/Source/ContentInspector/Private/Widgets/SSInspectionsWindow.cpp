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

#include "Widgets/SSInspectionsWindow.h"
#include "Widgets/SInspectionResult.h"

#include "Algo/Copy.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SScrollBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SSInspectionsWindow::Construct(const FArguments& InArgs)
{
	const TSharedRef<SScrollBox> ScrollBoxWithResults = SNew(SScrollBox);

	for (const FAssetData& AssetData : InArgs._AssetsData)
	{
		auto IsPackageNameMatch = [AssetData](const FInspectionResult& InInspectionData)
		{
			return AssetData.PackageName == InInspectionData.AssetPackageName;
		};

		TArray<FInspectionResult> ResultsForCurrentAsset;
		Algo::CopyIf(InArgs._InspectionResults, ResultsForCurrentAsset, IsPackageNameMatch);

		//	Get thumbnail for current asset
		TSharedRef<FAssetThumbnail> AssetThumbnail =
			MakeShared<FAssetThumbnail>(AssetData, 100, 100, InArgs._AssetThumbnailPool);
		const TSharedPtr<SWidget> ThumbnailWidget = AssetThumbnail->MakeThumbnailWidget();

		ScrollBoxWithResults->AddSlot()
		[
			SNew(SInspectionResult)
			.AssetData(AssetData)
			.AssetThumbnail(ThumbnailWidget)
			.InspectionResults(ResultsForCurrentAsset)
		];
	}

	ChildSlot[ScrollBoxWithResults];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION