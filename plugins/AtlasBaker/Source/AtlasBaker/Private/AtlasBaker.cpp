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

#include "AtlasBaker.h"
#include "AtlasBakerActions.h"
#include "SlateAtlasTileThumbnailRenderer.h"

#define LOCTEXT_NAMESPACE "FAtlasBakerModule"

void FAtlasBakerModule::StartupModule()
{
	IAssetTools& AssetTools = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	EAssetTypeCategories::Type EditorAssetCategory = AssetTools.RegisterAdvancedAssetCategory(
		FName(TEXT("CustomCategory")),
		LOCTEXT("CustomCategory", "CustomCategory"));
	TSharedPtr<IAssetTypeActions> ActionType = MakeShareable(new FAtlasBakerActions(EditorAssetCategory));
	AssetTools.RegisterAssetTypeActions(ActionType.ToSharedRef());

	TSharedRef<IAssetTypeActions> SlateAtlasTileActions = MakeShareable(new FSlateAtlasTileActions());
	AssetTools.RegisterAssetTypeActions(SlateAtlasTileActions);

	UThumbnailManager::Get().RegisterCustomRenderer(
		USlateAtlasTile::StaticClass(),
		USlateAtlasTileThumbnailRenderer::StaticClass());
}

void FAtlasBakerModule::ShutdownModule()
{
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAtlasBakerModule, AtlasBaker)