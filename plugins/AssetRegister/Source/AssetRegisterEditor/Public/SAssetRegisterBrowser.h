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
#include "Editor/ContentBrowser/Public/ContentBrowserDelegates.h"
#include "EngineUtils.h"
#include "GameplayTagContainer.h"
#include "ReferencedAssetsUtils.h"
#include "Widgets/SCompoundWidget.h"

class SAssetRegisterEmptyRefsViewer;
class IAssetRegistry;
class UAssetManager;
class IAssetManagerEditorModule;
class SAssetPicker;
class IStatsViewer;
class SAssetRegisterStatsViewer;
struct FAssetPickerConfig;

struct FColumnsData
{
	bool bDataCollected { false };

	FAssetData SourceAssetData {};
	TArray<AActor*> DependentActors {};
	TArray<AActor*> PrefabDependentActors {};
	FString RegisterPrimaryTag { "Not assigned" };

	TWeakPtr<SDockTab> StatsViewerRegisterTab;
	TWeakPtr<SAssetRegisterStatsViewer> StatsViewerUI;

	void RegisterStatsViewerTab();
	TSharedRef<SDockTab> SpawnStatsViewer(const FSpawnTabArgs& Args);
	void ShowStatsViewer();

	void FillColumnData(const FName& InColumnName, FText& InOutColumnData) const;
};

//	Copy paste with modifications from AssetFileContextMenu.cpp
struct AssetRegisterWorldReferenceGenerator : public FFindReferencedAssets
{
	FAssetData SourceAsset {};
	UClass* PrefAssetClass {};
	UClass* PrefActorClass {};
	TSet<FGameplayTag> FilterAssetRegisterTags {};
	TSet<UClass*> ClassesForIgnore {};

	AssetRegisterWorldReferenceGenerator();

	void BuildReferencingData();
	void MarkAllObjects();
	void FindAssetsWithRegisterMark(UObject* InCheckedObject, TSet<FAssetData>& OutObjects);
	void FillColumnsData(UObject* InCheckedObject, const FString& InLevelName, FColumnsData& InColumnData);
};

class ASSETREGISTEREDITOR_API SAssetRegisterBrowser : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SAssetRegisterBrowser) : _NativeColumnsNames()
	{
	}

	SLATE_ARGUMENT(TArray<FString>, NativeColumnsNames)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TSharedPtr<FUICommandList> Commands;
	TSharedPtr<SAssetPicker> AssetPicker;

	TWeakPtr<SDockTab> StatsViewerRegisterTab;
	TWeakPtr<SAssetRegisterEmptyRefsViewer> StatsViewerUI;

	TMap<FAssetData, FColumnsData> CollectedData {};
	TSet<FName> AssetRegisterTagsToIgnore {};
	TArray<TSharedPtr<FString>> AllAvailableLevels {};
	int32 SelectedLevel {};

	FSyncToAssetsDelegate SyncToAssetsDelegate;
	FGetCurrentSelectionDelegate GetCurrentSelectionDelegate;
	FSetARFilterDelegate SetFilterDelegate;

	//~ Begin SWidget Interface
	virtual FReply OnDrop(const FGeometry& InMyGeometry, const FDragDropEvent& InDragDropEvent) override;
	//~ End SWidget Interface

	TSharedRef<SWidget> ComboBoxDetailFilterWidget(TSharedPtr<FString> InItem);
	void ComboBoxDetailFilterWidgetSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	void CollectLevelNamesInWorld(UWorld* World, TArray<FString>& LevelNames);
	FText GetSelectedComboBoxDetailFilterTextLabel() const;
	ULevel* GetSelectedLevel();

	void FindInContentBrowser() const;
	bool IsAnythingSelected() const;
	void FindReferencesForSelectedAssets() const;
	void ShowSizeMapForSelectedAssets() const;
	void RemoveSelectedAssetsFromList();
	void ClearDataWithoutDependentActors();
	void ShowDependedActors(TArray<FAssetData> SelectedAssets);
	void RegisterUICommands();
	void FillAvailableLevels();
	void RefreshAssetView();
	void AddAssetsToList(const TArray<FAssetData>& InAssetsToView, bool bInReplaceExisting);
	void RemoveAssetsFromList(const TArray<FAssetData>& InAssetsToView);

	FReply CollectData();
	FReply FindEmptyRefs();
	TSharedRef<SDockTab> SpawnEmptyRefsWidget(const FSpawnTabArgs& Args);
	FReply CollectDataFromAssetWithRegisterMark();
	FReply ClearAssets();
	TSharedRef<SWidget> CreateAssetPicker(const FAssetPickerConfig& InConfig);
	TSharedPtr<SWidget> OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets);
	void OnRequestOpenAsset(const FAssetData& AssetData) const;
	FString GetStringValueForCustomColumn(FAssetData& AssetData, FName InColumnName) const;
	FText GetDisplayTextForCustomColumn(FAssetData& AssetData, FName InColumnName) const;
};
