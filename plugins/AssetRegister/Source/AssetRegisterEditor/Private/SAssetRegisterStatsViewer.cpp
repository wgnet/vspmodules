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
#include "SAssetRegisterStatsViewer.h"
#include "SAssetRegisterActorButton.h"

#include "SlateOptMacros.h"
#include "Widgets/Layout/SScrollBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAssetRegisterStatsViewer::Construct(
	const FArguments& InArgs,
	TArray<AActor*> InDependentActors,
	TArray<AActor*> InDependentPrefabActors)
{
	const TSharedRef<SScrollBox> ScrollBoxWithDependentActors = SNew(SScrollBox);

	for (auto Actor : InDependentActors)
	{
		ScrollBoxWithDependentActors->AddSlot()
		[
			SNew(SAssetRegisterActorButton)
			.ActorInWorld(Actor)
		];
	}

	const TSharedRef<SScrollBox> ScrollBoxWithDependentPrefabActors = SNew(SScrollBox);

	for (auto Actor : InDependentPrefabActors)
	{
		ScrollBoxWithDependentPrefabActors->AddSlot()
		[
			SNew(SAssetRegisterActorButton)
			.ActorInWorld(Actor)
		];
	}

	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.AutoHeight()
			[
				SNew(STextBlock)
				
				.Text(FText::FromString("Dependent Actors"))
			]
			+ SVerticalBox::Slot()
			[
				ScrollBoxWithDependentActors
			]
		]
		+ SHorizontalBox::Slot()
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(EHorizontalAlignment::HAlign_Center)
			.AutoHeight()
			[
				SNew(STextBlock)
				.Text(FText::FromString("Prefab Dependent Actors"))
			]
			+ SVerticalBox::Slot()
			[
				ScrollBoxWithDependentPrefabActors
			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
