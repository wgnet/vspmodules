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
#include "AssetRegisterData.h"
#include "LODDIstancesTable.h"

#include "AssetRegisterEditor.generated.h"
DECLARE_LOG_CATEGORY_EXTERN(LogAssetRegister, Log, All)

class FUICommandList;
class SAssetRegisterBrowser;
class IAssetRegistry;
struct FAssetRegisterMarkTagsSettings;

class FAssetRegisterEditorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void AddToolbarExtension(FToolBarBuilder& Builder);
	void AddMenuExtension(FMenuBuilder& Builder);

	void CreateContentBrowserHooks();
	void RemoveContentBrowserHooks();

	TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& InSelectedAssets);
	void CreateAssetRegisterBrowserAssetMenu(FMenuBuilder& InMenuBuilder, TArray<FAssetData> InSelectedAssets);
	void FillSubmenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> InSelectedAssets);
	void CreateWindowWithSettings(TArray<FAssetData> InSelectedAssets);
	TArray<FString> CollectColumnNamesForHiding();

	static bool SetLODsToStaticMesh(UStaticMesh* AssetStaticMesh, const UAssetRegisterMark* InTagsSettings);
	static void GetLODsValue(const FLodDistancesStruct* CurrentRow, TArray<float>& LODsValues);

	static void SetMeshLODScreenSize(UStaticMesh* StaticMesh, int32 LODIndex, float ScreenSize);

private:
	FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
	TSharedPtr<FUICommandList> PluginCommands;
	TWeakPtr<SDockTab> AssetRegisterTab;
	TWeakPtr<SAssetRegisterBrowser> AssetRegisterUI;

	void PluginButtonClicked();
	TSharedRef<SDockTab> SpawnAssetRegisterBrowser(const FSpawnTabArgs& Args);
	bool CreateAssetRegisterMark(const FAssetData& InAssetData, const FAssetRegisterMarkTagsSettings* InTagsSettings);
	static void ShowNotification(FString InNotification, float InDuration = 2.0f, bool IsFailed = false);
};

UCLASS()
class UStaticMeshToolset : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void ReloadStaticMeshLODsByTag(TArray<UObject*> SelectedObjects);
	static UAssetRegisterMark* FindAssetRegisterMark(const FAssetData& InAssetData);
};
