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
#include "SAssetRegisterActorButton.h"

#include "SlateOptMacros.h"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SAssetRegisterActorButton::Construct(const FArguments& InArgs)
{
	ActorInWorld = InArgs._ActorInWorld;

	SButton::Construct(SButton::FArguments()
		.Text(FText::FromName(ActorInWorld->GetFName()))
		.OnClicked_Raw(this, &SAssetRegisterActorButton::ActorClicked));
}

void SAssetRegisterActorButton::Construct(const FArguments& InArgs, UStaticMeshComponent* EmptyStaticMeshComponent)
{
	ActorInWorld = InArgs._ActorInWorld;

	FString Name =
		ActorInWorld->GetFName().ToString() + FString(" ---> ") + EmptyStaticMeshComponent->GetFName().ToString();

	SButton::Construct(SButton::FArguments()
		.Text(FText::FromString(Name))
		.OnClicked_Raw(this, &SAssetRegisterActorButton::ActorClicked));
}

FReply SAssetRegisterActorButton::ActorClicked()
{
	if (ActorInWorld)
	{
		GEditor->SelectNone(false, false);
		GEditor->SelectActor(ActorInWorld, true, true, true);
		GEditor->MoveViewportCamerasToActor(*ActorInWorld, false);
	}

	return FReply::Handled();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
