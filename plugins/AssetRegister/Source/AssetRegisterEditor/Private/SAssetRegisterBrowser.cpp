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
#include "SAssetRegisterBrowser.h"
#include "AssetRegisterCommands.h"
#include "AssetRegisterData.h"
#include "AssetRegisterSettings.h"
#include "SAssetRegisterEmptyRefsViewer.h"
#include "SAssetRegisterParamDialog.h"
#include "SAssetRegisterStatsViewer.h"

#include "Algo/Transform.h"
#include "AssetManagerEditorModule.h"
#include "ContentBrowser/Private/SAssetPicker.h"
#include "ContentBrowser/Private/SAssetView.h"
#include "ContentBrowserModule.h"
#include "DragAndDrop/AssetDragDropOp.h"
#include "Engine/AssetManager.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "IContentBrowserSingleton.h"
#include "Misc/ScopedSlowTask.h"
#include "SlateOptMacros.h"
#include "Toolkits/GlobalEditorCommonCommands.h"

#define LOCTEXT_NAMESPACE "AssetRegisterBrowser"

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

namespace FAssetRegisterBrowserLocal
{
	const FString AssetRegisterDot = TEXT("AssetRegister.");
	const FString DataEmptyMessage { "Data not collected" };
	const FString AllLevels { "All Levels" };
	const FName AssetRegisterStatsViewer = TEXT("AssetRegisterStatsViewer");

	const FName DependentActors = TEXT("DependentActors");
	const FName PrefabDependentActors = TEXT("PrefabDependentActors");
	const FName RegisterPrimaryTag = TEXT("RegisterPrimaryTag");
	FName EmtyRefsTabName = "EmtyRefsTabName";
}

AssetRegisterWorldReferenceGenerator::AssetRegisterWorldReferenceGenerator()
{
	if (UAssetRegisterSettings* AssetRegisterSettings = UAssetRegisterSettings::Get())
	{
		PrefActorClass = AssetRegisterSettings->PrefabActorClass;
		PrefAssetClass = AssetRegisterSettings->PrefabAssetClass;
		ClassesForIgnore = AssetRegisterSettings->AdditionalClassesForIgnore;
	}
}

void AssetRegisterWorldReferenceGenerator::BuildReferencingData()
{
	MarkAllObjects();

	const int32 MaxRecursionDepth = 0;
	const bool bIncludeClasses = true;
	const bool bIncludeDefaults = false;
	const bool bReverseReferenceGraph = true;

	UWorld* World = GWorld;

	FReferencedAssets* WorldReferencer = new (Referencers) FReferencedAssets(World);
	FFindAssetsArchive(
		World,
		WorldReferencer->AssetList,
		&ReferenceGraph,
		MaxRecursionDepth,
		bIncludeClasses,
		bIncludeDefaults,
		bReverseReferenceGraph);

	for (ULevelStreaming* StreamingLevel : World->GetStreamingLevels())
	{
		if (StreamingLevel)
		{
			if (ULevel* Level = StreamingLevel->GetLoadedLevel())
			{
				FReferencedAssets* LevelReferencer = new (Referencers) FReferencedAssets(Level);
				FFindAssetsArchive(
					Level,
					LevelReferencer->AssetList,
					&ReferenceGraph,
					MaxRecursionDepth,
					bIncludeClasses,
					bIncludeDefaults,
					bReverseReferenceGraph);
			}
		}
	}

	TArray<UObject*> ReferencedObjects;
	for (AActor* Actor : FActorRange(World))
	{
		ReferencedObjects.Reset();
		Actor->GetReferencedContentObjects(ReferencedObjects);
		for (UObject* Reference : ReferencedObjects)
		{
			TSet<UObject*>& Objects = ReferenceGraph.FindOrAdd(Reference);
			Objects.Add(Actor);
		}
	}
}

void AssetRegisterWorldReferenceGenerator::MarkAllObjects()
{
	for (FThreadSafeObjectIterator It; It; ++It)
	{
		It->Mark(OBJECTMARK_TagExp);
	}
}

void AssetRegisterWorldReferenceGenerator::FindAssetsWithRegisterMark(
	UObject* InCheckedObject,
	TSet<FAssetData>& OutObjects)
{
	if (!InCheckedObject->HasAnyMarks(OBJECTMARK_TagExp))
	{
		return;
	}

	InCheckedObject->UnMark(OBJECTMARK_TagExp);

	if (InCheckedObject->IsAsset())
	{
		if (UAssetRegisterMark* AssetRegisterMark = UAssetRegisterMark::GetAssetRegisterMark(InCheckedObject))
		{
			if (FilterAssetRegisterTags.Num() == 0)
			{
				OutObjects.Add(InCheckedObject);
			}
			else
			{
				TSet<FGameplayTag> AssetTags {};

				AssetTags.Add(AssetRegisterMark->PrimaryTag);
				AssetTags.Append(AssetRegisterMark->AdditionalTags);

				if (FilterAssetRegisterTags.Num() <= AssetTags.Num())
				{
					if (AssetTags.Includes(FilterAssetRegisterTags))
					{
						OutObjects.Add(InCheckedObject);
						return;
					}
				}
			}
		}
	}

	TSet<UObject*>* ReferencingObjects = ReferenceGraph.Find(InCheckedObject);
	if (ReferencingObjects)
	{
		for (TSet<UObject*>::TConstIterator SetIt(*ReferencingObjects); SetIt; ++SetIt)
		{
			FindAssetsWithRegisterMark(*SetIt, OutObjects);
		}
	}
}

void AssetRegisterWorldReferenceGenerator::FillColumnsData(
	UObject* InCheckedObject,
	const FString& InLevelName,
	FColumnsData& InColumnData)
{
	if (!InCheckedObject->HasAnyMarks(OBJECTMARK_TagExp))
	{
		return;
	}

	InCheckedObject->UnMark(OBJECTMARK_TagExp);

	const bool bSourceAssetIsPrefabAsset = SourceAsset.GetClass() == PrefAssetClass;
	const bool bCheckedObjectIsPrefabActor = InCheckedObject->GetClass() == PrefActorClass;
	const bool bCheckedObjectIsIgnoredClass = ClassesForIgnore.Contains(InCheckedObject->GetClass());

	if ((bSourceAssetIsPrefabAsset && bCheckedObjectIsPrefabActor) || !bCheckedObjectIsIgnoredClass)
	{
		if (AActor* CheckedActor = Cast<AActor>(InCheckedObject))
		{
			if (InLevelName != FAssetRegisterBrowserLocal::AllLevels)
			{
				const FString ActorLevelName =
					FPackageName::GetShortName(CheckedActor->GetLevel()->GetOutermost()->GetName());
				const FString LevelToSearch = InLevelName;

				if (LevelToSearch != ActorLevelName)
					return;
			}

			if (CheckedActor->GetOwner() && CheckedActor->GetOwner()->GetClass() == PrefActorClass)
			{
				InColumnData.PrefabDependentActors.AddUnique(CheckedActor->GetOwner());
			}

			InColumnData.DependentActors.AddUnique(CheckedActor);
			return;
		}
	}

	TSet<UObject*>* ReferencingObjects = ReferenceGraph.Find(InCheckedObject);
	if (ReferencingObjects)
	{
		for (TSet<UObject*>::TConstIterator SetIt(*ReferencingObjects); SetIt; ++SetIt)
		{
			FillColumnsData(*SetIt, InLevelName, InColumnData);
		}
	}
}

void FColumnsData::RegisterStatsViewerTab()
{
	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(
			SourceAssetData.AssetName,
			FOnSpawnTab::CreateRaw(this, &FColumnsData::SpawnStatsViewer))
		.SetDisplayName(
			FText::Format(LOCTEXT("StatsViewerTitle", "{0}"), FText::FromString(SourceAssetData.AssetName.ToString())));
}

TSharedRef<SDockTab> FColumnsData::SpawnStatsViewer(const FSpawnTabArgs& Args)
{
	return SAssignNew(StatsViewerRegisterTab, SDockTab)
		.TabRole(ETabRole::NomadTab)
			[SAssignNew(StatsViewerUI, SAssetRegisterStatsViewer, DependentActors, PrefabDependentActors)];
}

void FColumnsData::ShowStatsViewer()
{
	FGlobalTabmanager::Get()->TryInvokeTab(SourceAssetData.AssetName);
}

void FColumnsData::FillColumnData(const FName& InColumnName, FText& InOutColumnData) const
{
	if (bDataCollected)
	{
		if (InColumnName == FAssetRegisterBrowserLocal::DependentActors)
		{
			InOutColumnData = FText::FromString(FString::Printf(TEXT("%d"), DependentActors.Num()));
		}
		else if (InColumnName == FAssetRegisterBrowserLocal::PrefabDependentActors)
		{
			InOutColumnData = FText::FromString(FString::Printf(TEXT("%d"), PrefabDependentActors.Num()));
		}
		else if (InColumnName == FAssetRegisterBrowserLocal::RegisterPrimaryTag)
		{
			InOutColumnData = FText::FromString(RegisterPrimaryTag);
		}
	}
}

TSharedRef<SDockTab> SAssetRegisterBrowser::SpawnEmptyRefsWidget(const FSpawnTabArgs& Args)
{
	return SAssignNew(StatsViewerRegisterTab, SDockTab)
		.TabRole(ETabRole::NomadTab)[SAssignNew(
			StatsViewerUI,
			SAssetRegisterEmptyRefsViewer,
			TArray<AActor*>(),
			TArray<UStaticMeshComponent*>())];
}

void SAssetRegisterBrowser::Construct(const FArguments& InArgs)
{
	RegisterUICommands();
	FillAvailableLevels();

	FAssetPickerConfig Config;
	Config.InitialAssetViewType = EAssetViewType::Column;
	Config.bAddFilterUI = true;
	Config.bSortByPathInColumnView = true;
	Config.bCanShowClasses = false;

	Config.OnAssetDoubleClicked = FOnAssetDoubleClicked::CreateSP(this, &SAssetRegisterBrowser::OnRequestOpenAsset);
	Config.OnGetAssetContextMenu =
		FOnGetAssetContextMenu::CreateSP(this, &SAssetRegisterBrowser::OnGetAssetContextMenu);
	Config.SyncToAssetsDelegates.Add(&SyncToAssetsDelegate);
	Config.GetCurrentSelectionDelegates.Add(&GetCurrentSelectionDelegate);
	Config.SetFilterDelegates.Add(&SetFilterDelegate);
	Config.bFocusSearchBoxWhenOpened = false;
	Config.bPreloadAssetsForContextMenu = false;

	Config.HiddenColumnNames.Append(InArgs._NativeColumnsNames);

	Config.CustomColumns.Emplace(
		FAssetRegisterBrowserLocal::DependentActors,
		LOCTEXT("DependentActors", "Dependent Actors"),
		LOCTEXT("ADependentActorsTooltip", "How many actors used this asset"),
		UObject::FAssetRegistryTag::TT_Numerical,
		FOnGetCustomAssetColumnData::CreateSP(this, &SAssetRegisterBrowser::GetStringValueForCustomColumn),
		FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAssetRegisterBrowser::GetDisplayTextForCustomColumn));
	Config.CustomColumns.Emplace(
		FAssetRegisterBrowserLocal::PrefabDependentActors,
		LOCTEXT("PrefabDependentActors", "Prefab Dependent Actors"),
		LOCTEXT("PrefabDependentActorsTooltip", "How many Prefab actors used this asset"),
		UObject::FAssetRegistryTag::TT_Numerical,
		FOnGetCustomAssetColumnData::CreateSP(this, &SAssetRegisterBrowser::GetStringValueForCustomColumn),
		FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAssetRegisterBrowser::GetDisplayTextForCustomColumn));
	Config.CustomColumns.Emplace(
		FAssetRegisterBrowserLocal::RegisterPrimaryTag,
		LOCTEXT("RegisterPrimaryTag", "Register Primary Tag"),
		LOCTEXT("RegisterPrimaryTagTooltip", "Primary tag, if set"),
		UObject::FAssetRegistryTag::TT_Alphabetical,
		FOnGetCustomAssetColumnData::CreateSP(this, &SAssetRegisterBrowser::GetStringValueForCustomColumn),
		FOnGetCustomAssetColumnDisplayText::CreateSP(this, &SAssetRegisterBrowser::GetDisplayTextForCustomColumn));

	ChildSlot
		[SNew(SVerticalBox)
		 + SVerticalBox::Slot().AutoHeight()
			   [SNew(SBorder)
					.Padding(FMargin(3))
					.BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))
						[SNew(SHorizontalBox)
						 + SHorizontalBox::Slot().AutoWidth()[SNew(SButton)
																  .HAlign(HAlign_Center)
																  .VAlign(VAlign_Center)
																  .Text(LOCTEXT("ClearAssets", "Clear Assets"))
																  .OnClicked(this, &SAssetRegisterBrowser::ClearAssets)]
						 + SHorizontalBox::Slot().AutoWidth()
							   [SNew(SButton)
									.HAlign(HAlign_Center)
									.VAlign(VAlign_Center)
									.Text(LOCTEXT("Get Assets", "Collect Data With Register Mark"))
									.OnClicked(this, &SAssetRegisterBrowser::CollectDataFromAssetWithRegisterMark)]
						 + SHorizontalBox::Slot().AutoWidth()[SNew(SButton)
																  .HAlign(HAlign_Center)
																  .VAlign(VAlign_Center)
																  .Text(LOCTEXT("Collect Data", "Collect Data"))
																  .OnClicked(this, &SAssetRegisterBrowser::CollectData)]
						 + SHorizontalBox::Slot()
							   .AutoWidth()[SNew(SButton)
												.HAlign(HAlign_Center)
												.VAlign(VAlign_Center)
												.Text(LOCTEXT("Find Empty Refs", "Find Empty Refs"))
												.OnClicked(this, &SAssetRegisterBrowser::FindEmptyRefs)]
						 + SHorizontalBox::Slot().HAlign(HAlign_Fill)[SNew(SSpacer)]
						 + SHorizontalBox::Slot().AutoWidth()
							   [SNew(SComboBox<TSharedPtr<FString>>)
									.OptionsSource(&AllAvailableLevels)
									.OnSelectionChanged(
										this,
										&SAssetRegisterBrowser::ComboBoxDetailFilterWidgetSelectionChanged)
									.OnGenerateWidget(this, &SAssetRegisterBrowser::ComboBoxDetailFilterWidget)
										[SNew(STextBlock)
											 .Text(
												 this,
												 &SAssetRegisterBrowser::GetSelectedComboBoxDetailFilterTextLabel)]]]]
		 + SVerticalBox::Slot().FillHeight(
			 1.f)[SNew(SBorder)
					  .Padding(FMargin(3))
					  .BorderImage(FEditorStyle::GetBrush("ToolPanel.GroupBorder"))[CreateAssetPicker(Config)]]];

	RefreshAssetView();
}

void SAssetRegisterBrowser::RegisterUICommands()
{
	Commands = MakeShareable(new FUICommandList());

	Commands->MapAction(
		FGlobalEditorCommonCommands::Get().FindInContentBrowser,
		FUIAction(
			FExecuteAction::CreateSP(this, &SAssetRegisterBrowser::FindInContentBrowser),
			FCanExecuteAction::CreateSP(this, &SAssetRegisterBrowser::IsAnythingSelected)));

	Commands->MapAction(
		FAssetRegisterCommands::Get().RemoveAssetsFromList,
		FUIAction(
			FExecuteAction::CreateSP(this, &SAssetRegisterBrowser::RemoveSelectedAssetsFromList),
			FCanExecuteAction::CreateSP(this, &SAssetRegisterBrowser::IsAnythingSelected)));

	Commands->MapAction(
		FAssetRegisterCommands::Get().ViewReferences,
		FUIAction(
			FExecuteAction::CreateSP(this, &SAssetRegisterBrowser::FindReferencesForSelectedAssets),
			FCanExecuteAction::CreateSP(this, &SAssetRegisterBrowser::IsAnythingSelected)));

	Commands->MapAction(
		FAssetRegisterCommands::Get().ViewSizeMap,
		FUIAction(
			FExecuteAction::CreateSP(this, &SAssetRegisterBrowser::ShowSizeMapForSelectedAssets),
			FCanExecuteAction::CreateSP(this, &SAssetRegisterBrowser::IsAnythingSelected)));
}

void SAssetRegisterBrowser::FillAvailableLevels()
{
	AllAvailableLevels.Add(MakeShared<FString>(FAssetRegisterBrowserLocal::AllLevels));

	TArray<FString> LevelsNames {};
	CollectLevelNamesInWorld(GWorld, LevelsNames);

	for (const FString& LevelName : LevelsNames)
	{
		AllAvailableLevels.Add(MakeShared<FString>(LevelName));
	}
}

void SAssetRegisterBrowser::FindInContentBrowser() const
{
	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	if (CurrentSelection.Num() > 0)
	{
		FContentBrowserModule& ContentBrowserModule =
			FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
		ContentBrowserModule.Get().SyncBrowserToAssets(CurrentSelection);
	}
}

bool SAssetRegisterBrowser::IsAnythingSelected() const
{
	const TArray<FAssetData> CurrentSelection = GetCurrentSelectionDelegate.Execute();
	return CurrentSelection.Num() > 0;
}

void SAssetRegisterBrowser::FindReferencesForSelectedAssets() const
{
	const TArray<FAssetData> Assets = GetCurrentSelectionDelegate.Execute();
	TArray<FAssetIdentifier> AssetIdentifiers;
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(Assets, AssetIdentifiers);

	if (AssetIdentifiers.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenReferenceViewerUI(AssetIdentifiers);
	}
}

void SAssetRegisterBrowser::ShowSizeMapForSelectedAssets() const
{
	const TArray<FAssetData> Assets = GetCurrentSelectionDelegate.Execute();
	TArray<FAssetIdentifier> AssetIdentifiers;
	IAssetManagerEditorModule::ExtractAssetIdentifiersFromAssetDataList(Assets, AssetIdentifiers);

	if (AssetIdentifiers.Num() > 0)
	{
		IAssetManagerEditorModule::Get().OpenSizeMapUI(AssetIdentifiers);
	}
}

void SAssetRegisterBrowser::RemoveSelectedAssetsFromList()
{
	const TArray<FAssetData> SelectedAssets = GetCurrentSelectionDelegate.Execute();
	RemoveAssetsFromList(SelectedAssets);
}

FReply SAssetRegisterBrowser::CollectData()
{
	if (CollectedData.Num() > 0)
	{
		FScopedSlowTask SlowTask(
			CollectedData.Num(),
			NSLOCTEXT("AssetRegister", "AssetRegisterCollectingData", "Asset register collecting data..."));
		SlowTask.MakeDialog();

		AssetRegisterWorldReferenceGenerator ObjRefGenerator;

		if (UAssetRegisterSettings* AssetRegisterSettings = UAssetRegisterSettings::Get())
		{
			ObjRefGenerator.PrefActorClass = AssetRegisterSettings->PrefabActorClass;
			ObjRefGenerator.ClassesForIgnore = AssetRegisterSettings->AdditionalClassesForIgnore;
			ObjRefGenerator.PrefAssetClass = AssetRegisterSettings->PrefabAssetClass;
		}

		ObjRefGenerator.BuildReferencingData();
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

		for (TTuple<FAssetData, FColumnsData>& CurrentCollectedData : CollectedData)
		{
			FText ValidatingMessage = FText::Format(
				LOCTEXT("AssetRegisterCollectingData", "Collecting data: {0}"),
				FText::FromName(CurrentCollectedData.Key.AssetName));
			SlowTask.EnterProgressFrame(1, ValidatingMessage);

			CurrentCollectedData.Value = {};
			CurrentCollectedData.Value.SourceAssetData = CurrentCollectedData.Key;
			ObjRefGenerator.SourceAsset = CurrentCollectedData.Key;
			;

			UObject* AssetToCheck = CurrentCollectedData.Key.GetAsset();

			//	Get primary tag
			if (UAssetRegisterMark* AssetRegisterMark = UAssetRegisterMark::GetAssetRegisterMark(AssetToCheck))
			{
				FString PrimaryTag = AssetRegisterMark->PrimaryTag.GetTagName().ToString();
				PrimaryTag.RemoveFromStart(FAssetRegisterBrowserLocal::AssetRegisterDot);

				CurrentCollectedData.Value.RegisterPrimaryTag = PrimaryTag;
			}

			ObjRefGenerator.MarkAllObjects();
			ObjRefGenerator.FillColumnsData(
				AssetToCheck,
				*AllAvailableLevels[SelectedLevel],
				CurrentCollectedData.Value);

			CurrentCollectedData.Value.RegisterStatsViewerTab();
			CurrentCollectedData.Value.bDataCollected = true;
		}
	}

	RefreshAssetView();

	return FReply::Handled();
}

FReply SAssetRegisterBrowser::FindEmptyRefs()
{
	UWorld* World = GWorld;
	bool bOnlyActiveComponents = false;

	TArray<UObject*> OutComponents;
	TSubclassOf<UActorComponent> ComponentClass = UStaticMeshComponent::StaticClass();
	GetObjectsOfClass(ComponentClass, OutComponents, true, RF_ClassDefaultObject, EInternalObjectFlags::PendingKill);

	OutComponents.RemoveAll(
		[World, bOnlyActiveComponents](UObject* Obj)
		{
			UActorComponent* Comp = CastChecked<UActorComponent>(Obj);
			return (Comp->GetWorld() != World) || (bOnlyActiveComponents && !Comp->IsActive());
		});

	TArray<AActor*> EmptyActors;
	TArray<UStaticMeshComponent*> EmptyStaticMeshComponents;
	for (UObject* Comp : OutComponents)
	{
		UStaticMeshComponent* SmComp = Cast<UStaticMeshComponent>(Comp);
		UStaticMesh* MeshPtr = SmComp->GetStaticMesh();
		if (!MeshPtr)
		{
			AActor* Actor = SmComp->GetOwner();
			EmptyActors.Add(Actor);
			EmptyStaticMeshComponents.Add(SmComp);
		}
	}

	if (!StatsViewerRegisterTab.Pin().IsValid())
	{
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(
				FAssetRegisterBrowserLocal::EmtyRefsTabName,
				FOnSpawnTab::CreateRaw(this, &SAssetRegisterBrowser::SpawnEmptyRefsWidget))
			.SetDisplayName(LOCTEXT("Empty Refs", "Empty Refs"));
	}

	FGlobalTabmanager::Get()->TryInvokeTab(FAssetRegisterBrowserLocal::EmtyRefsTabName);

	TSharedPtr<SAssetRegisterEmptyRefsViewer> ContentWidget;
	StatsViewerRegisterTab.Pin()->SetContent(
		SAssignNew(StatsViewerUI, SAssetRegisterEmptyRefsViewer, EmptyActors, EmptyStaticMeshComponents));
	return FReply::Handled();
}

void SAssetRegisterBrowser::ClearDataWithoutDependentActors()
{
	TSet<FAssetData> DataWithoutDependentActors {};

	for (const TTuple<FAssetData, FColumnsData>& Data : CollectedData)
	{
		if (Data.Value.DependentActors.Num() == 0)
		{
			DataWithoutDependentActors.Add(Data.Key);
		}
	}

	RemoveAssetsFromList(DataWithoutDependentActors.Array());
}

void SAssetRegisterBrowser::ShowDependedActors(TArray<FAssetData> SelectedAssets)
{
	for (const FAssetData& AssetData : SelectedAssets)
	{
		if (CollectedData.Contains(AssetData))
		{
			CollectedData[AssetData].ShowStatsViewer();
		}
	}
}

TSharedRef<SWidget> SAssetRegisterBrowser::ComboBoxDetailFilterWidget(TSharedPtr<FString> InItem)
{
	FString ItemString;
	if (InItem.IsValid())
	{
		ItemString = *InItem;
	}
	return SNew(STextBlock).Text(FText::FromString(*ItemString));
}

void SAssetRegisterBrowser::ComboBoxDetailFilterWidgetSelectionChanged(
	TSharedPtr<FString> NewSelection,
	ESelectInfo::Type SelectInfo)
{
	for (int32 i = 0; i < AllAvailableLevels.Num(); ++i)
	{
		if (AllAvailableLevels[i] == NewSelection)
		{
			SelectedLevel = i;
			break;
		}
	}
}

void SAssetRegisterBrowser::CollectLevelNamesInWorld(UWorld* World, TArray<FString>& LevelNames)
{
	if (!World)
		return;

	auto WorldToName = [](const ULevel* Level)
	{
		return FPackageName::GetShortName(Level->GetOutermost()->GetName());
	};

	Algo::Transform(World->GetLevels(), LevelNames, WorldToName);
}

FText SAssetRegisterBrowser::GetSelectedComboBoxDetailFilterTextLabel() const
{
	FText Result { FText::FromString("Select Level") };

	if (AllAvailableLevels.IsValidIndex(SelectedLevel))
	{
		Result = FText::FromString(*AllAvailableLevels[SelectedLevel]);
	}

	return Result;
}

ULevel* SAssetRegisterBrowser::GetSelectedLevel()
{
	return SelectedLevel != 0 ? GWorld->GetLevel(SelectedLevel) : nullptr;
}

void SAssetRegisterBrowser::AddAssetsToList(const TArray<FAssetData>& InAssetsToView, bool bInReplaceExisting)
{
	if (bInReplaceExisting)
	{
		CollectedData.Empty();
	}

	for (const FAssetData& AssetData : InAssetsToView)
	{
		CollectedData.Add(AssetData);
	}

	RefreshAssetView();
}

void SAssetRegisterBrowser::RemoveAssetsFromList(const TArray<FAssetData>& InAssetsToView)
{
	for (const FAssetData& Data : InAssetsToView)
	{
		CollectedData.Remove(Data);
	}

	RefreshAssetView();
}

FReply SAssetRegisterBrowser::OnDrop(const FGeometry& InMyGeometry, const FDragDropEvent& InDragDropEvent)
{
	bool bWasDropHandled = false;
	const TSharedPtr<FDragDropOperation> Operation = InDragDropEvent.GetOperation();

	if (Operation.IsValid())
	{
		if (Operation->IsOfType<FAssetDragDropOp>())
		{
			const FAssetDragDropOp& DragDropOp = *StaticCastSharedPtr<FAssetDragDropOp>(Operation);

			AddAssetsToList(DragDropOp.GetAssets(), false);
			bWasDropHandled = true;
		}
	}

	return bWasDropHandled ? FReply::Handled() : FReply::Unhandled();
}

TSharedRef<SWidget> SAssetRegisterBrowser::CreateAssetPicker(const FAssetPickerConfig& InConfig)
{
	FContentBrowserModule& ContentBrowserModule =
		FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));
	AssetPicker = StaticCastSharedRef<SAssetPicker>(ContentBrowserModule.Get().CreateAssetPicker(InConfig));

	return AssetPicker.ToSharedRef();
}

void SAssetRegisterBrowser::RefreshAssetView()
{
	FARFilter Filter;

	for (const TTuple<FAssetData, FColumnsData>& CurrentData : CollectedData)
	{
		Filter.PackageNames.Add(CurrentData.Key.PackageName);
	}

	if (Filter.PackageNames.Num() == 0)
	{
		// Add a bad name to force it to display nothing
		Filter.PackageNames.Add("/Temp/FakePackageNameToMakeNothingShowUp");
	}

	SetFilterDelegate.Execute(Filter);
}

FReply SAssetRegisterBrowser::ClearAssets()
{
	CollectedData.Empty();
	RefreshAssetView();

	return FReply::Handled();
}

FReply SAssetRegisterBrowser::CollectDataFromAssetWithRegisterMark()
{
	TSharedRef<FStructOnScope> TagsSettings =
		MakeShared<FStructOnScope>(FAssetRegisterMarkTagsSettings::StaticStruct());
	FAssetRegisterMarkTagsSettings* TagsSettingsRef =
		reinterpret_cast<FAssetRegisterMarkTagsSettings*>(TagsSettings->GetStructMemory());

	TSharedRef<SWindow> Window = SNew(SWindow)
									 .Title(FText::FromString(TEXT("Asset Register - Create Asset Register Mark")))
									 .ClientSize(FVector2D(500, 200));

	TSharedPtr<SAssetRegisterParamDialog> Dialog;
	Window->SetContent(
		SAssignNew(Dialog, SAssetRegisterParamDialog, Window, TagsSettings).OkButtonText(LOCTEXT("OKButton", "OK")));
	GEditor->EditorAddModalWindow(Window);

	if (Dialog->bOKPressed)
	{
		TSet<FGameplayTag> TagsForSearch {};

		if (TagsSettingsRef->PrimaryTag.IsValid())
		{
			TagsForSearch.Add(TagsSettingsRef->PrimaryTag);
		}

		TagsForSearch.Append(TagsSettingsRef->AdditionalTags);

		AssetRegisterWorldReferenceGenerator ObjRefGenerator;
		ObjRefGenerator.FilterAssetRegisterTags = TagsForSearch;

		ObjRefGenerator.BuildReferencingData();
		CollectGarbage(GARBAGE_COLLECTION_KEEPFLAGS);

		ObjRefGenerator.MarkAllObjects();

		TSet<FAssetData> FoundAssets {};
		for (FThreadSafeObjectIterator It; It; ++It)
		{
			ObjRefGenerator.FindAssetsWithRegisterMark(*It, FoundAssets);
		}

		AddAssetsToList(FoundAssets.Array(), false);

		CollectData();
		ClearDataWithoutDependentActors();
	}

	return FReply::Handled();
}

void SAssetRegisterBrowser::OnRequestOpenAsset(const FAssetData& AssetData) const
{
	FindInContentBrowser();
}

TSharedPtr<SWidget> SAssetRegisterBrowser::OnGetAssetContextMenu(const TArray<FAssetData>& SelectedAssets)
{
	FMenuBuilder MenuBuilder(true, Commands);

	MenuBuilder.BeginSection(TEXT(""), NSLOCTEXT("", "", "Actions"));
	{
		MenuBuilder.AddMenuEntry(
			LOCTEXT("Show Depended Actors", "ShowActors"),
			LOCTEXT("", "Show list of actors depended to selected asset"),
			FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.OpenLevel"),
			FUIAction(FExecuteAction::CreateSP(this, &SAssetRegisterBrowser::ShowDependedActors, SelectedAssets)));

		MenuBuilder.AddMenuEntry(FGlobalEditorCommonCommands::Get().FindInContentBrowser);
		MenuBuilder.AddMenuEntry(FAssetRegisterCommands::Get().ViewReferences);
		MenuBuilder.AddMenuEntry(FAssetRegisterCommands::Get().ViewSizeMap);
		MenuBuilder.AddMenuEntry(FAssetRegisterCommands::Get().RemoveAssetsFromList);
	}

	return MenuBuilder.MakeWidget();
}

FString SAssetRegisterBrowser::GetStringValueForCustomColumn(FAssetData& AssetData, FName InColumnName) const
{
	return GetDisplayTextForCustomColumn(AssetData, InColumnName).ToString();
}

FText SAssetRegisterBrowser::GetDisplayTextForCustomColumn(FAssetData& AssetData, FName InColumnName) const
{
	FText Result = { FText::FromString(FAssetRegisterBrowserLocal::DataEmptyMessage) };

	if (CollectedData.Contains(AssetData))
	{
		CollectedData[AssetData].FillColumnData(InColumnName, Result);
	}

	return Result;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION
