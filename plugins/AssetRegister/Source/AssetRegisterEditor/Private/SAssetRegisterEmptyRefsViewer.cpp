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
#include "SAssetRegisterEmptyRefsViewer.h"
#include "SAssetRegisterActorButton.h"

#include "SlateOptMacros.h"
#include "Widgets/Layout/SScrollBox.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAssetRegisterEmptyRefsViewer::Construct(
	const FArguments& InArgs,
	TArray<AActor*> EmptyActors,
	TArray<UStaticMeshComponent*> EmptyStaticMeshComponents)
{
	const TSharedRef<SScrollBox> ScrollBoxWithDependentActors = SNew(SScrollBox);
	int32 SmId = 0;
	for (AActor* Actor : EmptyActors)
	{
		ScrollBoxWithDependentActors->AddSlot()[SNew(SAssetRegisterActorButton, EmptyStaticMeshComponents[SmId++]).ActorInWorld(Actor)];
	}

	ChildSlot
		[SNew(SHorizontalBox)
		 + SHorizontalBox::Slot()
			 [SNew(SVerticalBox)
			  + SVerticalBox::Slot()
					.HAlign(EHorizontalAlignment::HAlign_Center)
					.AutoHeight()[SNew(STextBlock)

									  .Text(FText::FromString("Empty Actors"))]
			  + SVerticalBox::Slot()[ScrollBoxWithDependentActors]]];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
