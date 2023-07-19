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

#include "TerrainTools.h"

#include "IPlacementModeModule.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "TerraformerBrushManager.h"
#include "TerraformerGroup.h"
#include "TerraformerSplineBrush.h"
#include "TerraformerStampBrush.h"
#include "TerrainMaterialProxy/TerrainMaterialProxy.h"
#include "TerrainMaterialProxy/TerrainMaterialProxyDetails.h"
#include "WeightmapManager.h"

#define LOCTEXT_NAMESPACE "FTerrainToolsModule"

namespace TerrainToolsLocal
{
	const struct FPlacementCategoryInfo Info =
		FPlacementCategoryInfo(FText::FromString("Terrain"), "TerrainHandle", "Terrain");
	TOptional<FPlacementModeID> PlacementModeID;
}

void FTerrainToolsModule::StartupModule()
{
	RegisterPlacementCategory();
	RegisterDetailsPanels();
	RegisterSlateStyle();
}

void FTerrainToolsModule::ShutdownModule()
{
	UnregisterPlacementCategory();
	UnregisterDetailsPanels();
	UnregisterSlateStyle();
}

void FTerrainToolsModule::RegisterPlacementCategory() const
{
	int32 SortOrder = 0;

	IPlacementModeModule::Get().RegisterPlacementCategory(TerrainToolsLocal::Info);
	TerrainToolsLocal::PlacementModeID = IPlacementModeModule::Get().RegisterPlaceableItem(
		TerrainToolsLocal::Info.UniqueHandle,
		MakeShareable(new FPlaceableItem(nullptr, FAssetData(ATerraformerBrushManager::StaticClass()), SortOrder++)));

	TerrainToolsLocal::PlacementModeID = IPlacementModeModule::Get().RegisterPlaceableItem(
		TerrainToolsLocal::Info.UniqueHandle,
		MakeShareable(new FPlaceableItem(nullptr, FAssetData(ATerraformerSplineBrush::StaticClass()), SortOrder++)));

	TerrainToolsLocal::PlacementModeID = IPlacementModeModule::Get().RegisterPlaceableItem(
		TerrainToolsLocal::Info.UniqueHandle,
		MakeShareable(new FPlaceableItem(nullptr, FAssetData(ATerraformerStampBrush::StaticClass()), SortOrder++)));

	TerrainToolsLocal::PlacementModeID = IPlacementModeModule::Get().RegisterPlaceableItem(
		TerrainToolsLocal::Info.UniqueHandle,
		MakeShareable(new FPlaceableItem(nullptr, FAssetData(ATerraformerGroup::StaticClass()), SortOrder++)));

	TerrainToolsLocal::PlacementModeID = IPlacementModeModule::Get().RegisterPlaceableItem(
		TerrainToolsLocal::Info.UniqueHandle,
		MakeShareable(new FPlaceableItem(nullptr, FAssetData(AWeightmapManager::StaticClass()), SortOrder++)));

	IPlacementModeModule::Get().RegisterPlacementCategory(TerrainToolsLocal::Info);
	TerrainToolsLocal::PlacementModeID = IPlacementModeModule::Get().RegisterPlaceableItem(
		TerrainToolsLocal::Info.UniqueHandle,
		MakeShareable(new FPlaceableItem(nullptr, FAssetData(ATerrainMaterialProxy::StaticClass()), SortOrder++)));
}

void FTerrainToolsModule::UnregisterPlacementCategory() const
{
	if (IPlacementModeModule::IsAvailable())
	{
		IPlacementModeModule::Get().UnregisterPlacementCategory(TerrainToolsLocal::Info.UniqueHandle);
		IPlacementModeModule::Get().UnregisterPlaceableItem(TerrainToolsLocal::PlacementModeID.GetValue());
	}
}

void FTerrainToolsModule::RegisterDetailsPanels() const
{
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		ATerrainMaterialProxy::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FTerrainMaterialProxyDetails::MakeInstance));
}

void FTerrainToolsModule::UnregisterDetailsPanels() const
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		auto& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(ATerrainMaterialProxy::StaticClass()->GetFName());
	}
}

void FTerrainToolsModule::RegisterComponentVisualizers()
{
}

void FTerrainToolsModule::UnregisterComponentVisualizers()
{
}

#define IMAGE_PLUGIN_BRUSH(RelativePath, ...) \
	FSlateImageBrush(FInstanceToolStyle::InContent(RelativePath, ".png"), __VA_ARGS__)
#define IMAGE_BRUSH(RelativePath, ...) \
	FSlateImageBrush(StyleSet->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
TSharedPtr<FSlateStyleSet> FTerrainToolsModule::StyleSet = nullptr;

void FTerrainToolsModule::RegisterSlateStyle() const
{
	// Const icon sizes
	const FVector2D Icon8x8(8.0f, 8.0f);
	const FVector2D Icon16x16(16.0f, 16.0f);
	const FVector2D Icon20x20(20.0f, 20.0f);
	const FVector2D Icon40x40(40.0f, 40.0f);
	const FVector2D Icon64x64(64.0f, 64.0f);

	if (StyleSet.IsValid())
	{
		return;
	}

	StyleSet = MakeShareable(new FSlateStyleSet(TEXT("TerraformerUIStyle")));

	StyleSet->SetContentRoot(IPluginManager::Get().FindPlugin(TEXT("TerrainTools"))->GetContentDir());
	StyleSet->SetCoreContentRoot(FPaths::EngineContentDir() / TEXT("Slate"));

	StyleSet->Set(TEXT("ClassIcon.TerraformerSplineBrush"), new IMAGE_BRUSH("Icons/Island_16x", Icon16x16));
	StyleSet->Set(TEXT("ClassThumbnail.TerraformerSplineBrush"), new IMAGE_BRUSH("Icons/Island_64x", Icon64x64));

	StyleSet->Set(TEXT("ClassIcon.TerraformerStampBrush"), new IMAGE_BRUSH("Icons/icon_ShowLandscape_16x", Icon16x16));
	StyleSet->Set(
		TEXT("ClassThumbnail.TerraformerStampBrush"),
		new IMAGE_BRUSH("Icons/icon_ShowLandscape_40x", Icon64x64));

	StyleSet->Set(TEXT("ClassIcon.TerraformerGroup"), new IMAGE_BRUSH("Icons/GroupIcon_x16", Icon16x16));
	StyleSet->Set(TEXT("ClassThumbnail.TerraformerGroup"), new IMAGE_BRUSH("Icons/GroupIcon_x64", Icon64x64));

	FSlateStyleRegistry::RegisterSlateStyle(*StyleSet.Get());
}

void FTerrainToolsModule::UnregisterSlateStyle() const
{
	if (StyleSet.IsValid())
	{
		FSlateStyleRegistry::UnRegisterSlateStyle(*StyleSet.Get());
		ensure(StyleSet.IsUnique());
		StyleSet.Reset();
	}
}

#undef IMAGE_PLUGIN_BRUSH
#undef IMAGE_BRUSH
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FTerrainToolsModule, TerrainTools)
