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

#include "Widgets/SInspectionResult.h"


#include "ContentInspectorSettings.h"
#include "ContentInspectorStyle.h"
#include "SlateOptMacros.h"
#include "Widgets/Layout/SScrollBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SInspectionResult::Construct(const FArguments& InArgs)
{
	const TSharedRef<SScrollBox> ScrollBoxWithInspectionsResults = SNew(SScrollBox);

	for (const FInspectionResult& InspectionResult : InArgs._InspectionResults)
	{
		if (InspectionResult.bInspectionFailed)
		{
			ScrollBoxWithInspectionsResults.Get().AddSlot()
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(SBorder)
					[
						SNew(SBorder)
						.BorderImage(this, &SInspectionResult::GetBackgroundColor, InspectionResult.bCriticalInspection)
						[
							SNew(STextBlock)
							.Text(FText::FromName(InspectionResult.FullFailedDescription))
						]
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SButton)
					.IsEnabled(this, &SInspectionResult::IsOpenURLButtonEnable, InspectionResult.InspectionClass)
					.ToolTipText(this, &SInspectionResult::GetButtonTooltipText,InspectionResult.InspectionClass)
					.OnClicked(this, &SInspectionResult::OpenConfluenceURL,InspectionResult.InspectionClass)
					.Content()
					[
						SNew(STextBlock)
						.Text(FText::FromString("Open confluence for details"))
					]
				]

			];
		}
	}

	ChildSlot
	[
		SNew(SBox)
		.HeightOverride(120)
		.Content()
		[
			SNew(SBorder)
			.Padding(10.f)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					SNew(SBox)
					.WidthOverride(100)
					.HeightOverride(100)
					[
						SNew(SButton)
						.ContentPadding(0.f)
						.OnClicked(this, &SInspectionResult::FocusContentBrowserOnAsset, InArgs._AssetData)
						[
							InArgs._AssetThumbnail.ToSharedRef()
						]
					]
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					ScrollBoxWithInspectionsResults
				]
			]
		]
	];
}

bool SInspectionResult::IsOpenURLButtonEnable(TSoftClassPtr<UInspectionBase> InInspectionClass) const
{
	const FName ConfluenceURL = UContentInspectorSettings::Get()->GetConfluenceURL(InInspectionClass);
	return ConfluenceURL != NAME_None;
}

FReply SInspectionResult::FocusContentBrowserOnAsset(FAssetData InAssetData)
{
	TArray<FAssetData> AssetToFocus { InAssetData };

	GEditor->SyncBrowserToObjects(AssetToFocus);
	return FReply::Handled();
}

FReply SInspectionResult::OpenConfluenceURL(TSoftClassPtr<UInspectionBase> InInspectionClass) const
{
	const FName ConfluenceURL = UContentInspectorSettings::Get()->GetConfluenceURL(InInspectionClass);

	FPlatformProcess::LaunchURL(*ConfluenceURL.ToString(), nullptr, nullptr);
	return FReply::Handled();
}

FText SInspectionResult::GetButtonTooltipText(TSoftClassPtr<UInspectionBase> InInspectionClass) const
{
	FText ResultText = FText::FromName(TEXT("Open confluence page"));
	const FName ConfluenceURL = UContentInspectorSettings::Get()->GetConfluenceURL(InInspectionClass);

	if (ConfluenceURL == NAME_None)
		ResultText = FText::FromName(TEXT("There is no page in confluence "));

	return ResultText;
}

const FSlateBrush* SInspectionResult::GetBackgroundColor(bool bInCriticalFailed) const
{
	const FName BrushName = bInCriticalFailed ? "ContentInspector.RedColor" : "ContentInspector.OrangeColor";
	return FContentInspectorStyle::Get().GetBrush(BrushName);
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
