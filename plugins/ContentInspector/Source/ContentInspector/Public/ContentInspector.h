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

#include "InspectionsManager.h"

class UObjectLibrary;

class FContentInspectorModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	void InspectSelectionAsset(TArray<FAssetData> SelectedAssets);
	void InspectAssetsInSelectionFolder(TArray<FString> InSelectedPaths);

	void InspectFolders(const TArray<FString>& InSelectedPaths);

private:
	UInspectionsManager* InspectionsManager = nullptr;

	FDelegateHandle ContentBrowserAssetExtenderDelegateHandle;
	FDelegateHandle ContentBrowserPathExtenderDelegateHandle;
	FCoreUObjectDelegates::FIsPackageOKToSaveDelegate OkToSaveBackupDelegate;

	// Delegate handles for asset state indicator extensions
	TArray<FDelegateHandle> ContentBrowserAssetExtraStateDelegateHandles;

	//	Assets created during the session. They are checked for saving
	TArray<TSoftObjectPtr<UObject>> NewAssets;

	TSharedPtr<FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<SWindow> InspectionWindow;

	void PostEngineInit();
	void WorldAdded(UWorld* NewWorld);

	void CreateContentBrowserHooks();
	void RemoveContentBrowserHooks();

	void CreatePackageEventsHook();
	void RemovePackageEventsHook();

	bool CanSavePackage(UPackage* InPackage, const FString& InFilename, FOutputDevice* ErrorLog);
	void AssetCreated(UObject* InAsset);
	void AssetRenamed(const struct FAssetData& InAssetData, const FString& InOldName);

	TSharedRef<FExtender> OnExtendContentBrowserAssetSelectionMenu(const TArray<FAssetData>& InSelectedAssets);
	TSharedRef<FExtender> OnExtendContentBrowserPathSelectionMenu(const TArray<FString>& InSelectedPaths);

	// Delegate to generate inspection state indicator on content browser assets
	TSharedRef<SWidget> GenerateAssetViewExtraStateIndicators(const FAssetData& AssetData);

	FReply CreateInspectionResultWindow(TArray<FAssetData> InAssetsData);

	void CreateContentInspectorBrowserAssetMenu(FMenuBuilder& InMenuBuilder, TArray<FAssetData> InSelectedAssets);
	void CreateContentInspectorBrowserPathSelectionMenu(FMenuBuilder& InMenuBuilder, TArray<FString> InSelectedPaths);

	void CreateInspectionsManager();
	void RemoveInspectionManager();
};
