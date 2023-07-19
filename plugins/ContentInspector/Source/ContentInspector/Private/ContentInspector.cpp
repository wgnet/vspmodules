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

#include "ContentInspector.h"
#include "ContentInspectorStyle.h"

#include "Algo/Copy.h"
#include "AssetRegistryModule.h"
#include "ContentBrowserMenuContexts.h"
#include "ContentBrowserModule.h"
#include "ContentInspectorSettings.h"
#include "MessageLogModule.h"
#include "Misc/ScopedSlowTask.h"
#include "ToolMenus.h"
#include "UObject/ConstructorHelpers.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/SSInspectionsWindow.h"

#define LOCTEXT_NAMESPACE "FContentInspectorModule"

void FContentInspectorModule::StartupModule()
{
	FCoreDelegates::OnPostEngineInit.AddRaw(this, &FContentInspectorModule::PostEngineInit);

	FContentInspectorStyle::Initialize();
	FContentInspectorStyle::ReloadTextures();

	ThumbnailPool = MakeShareable(new FAssetThumbnailPool(24));

	CreateContentBrowserHooks();
}

void FContentInspectorModule::ShutdownModule()
{
	FCoreDelegates::OnPostEngineInit.RemoveAll(this);

	//TODO:: Attempt to delete too late
	//RemoveInspectionManager();
	RemoveContentBrowserHooks();
	RemovePackageEventsHook();
}

void FContentInspectorModule::PostEngineInit()
{
	if (IsRunningCommandlet())
		return;

	GEditor->OnWorldAdded().AddRaw(this, &FContentInspectorModule::WorldAdded);
	CreatePackageEventsHook();
}

void FContentInspectorModule::WorldAdded(UWorld* NewWorld)
{
	CreateInspectionsManager();
}

void FContentInspectorModule::CreateContentBrowserHooks()
{
	if (FSlateApplication::IsInitialized())
	{
		FContentBrowserModule& ContentBrowserModule =
			FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates =
			ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

		CBAssetMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(
			this,
			&FContentInspectorModule::OnExtendContentBrowserAssetSelectionMenu));
		ContentBrowserAssetExtenderDelegateHandle = CBAssetMenuExtenderDelegates.Last().GetHandle();

		TArray<FContentBrowserMenuExtender_SelectedPaths>& CBFolderMenuExtenderDelegates =
			ContentBrowserModule.GetAllPathViewContextMenuExtenders();

		CBFolderMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedPaths::CreateRaw(
			this,
			&FContentInspectorModule::OnExtendContentBrowserPathSelectionMenu));
		ContentBrowserPathExtenderDelegateHandle = CBFolderMenuExtenderDelegates.Last().GetHandle();

		// Create inspection indicator
		const FAssetViewExtraStateGenerator InspectionIndicator(
			FOnGenerateAssetViewExtraStateIndicators::CreateRaw(
				this,
				&FContentInspectorModule::GenerateAssetViewExtraStateIndicators),
			nullptr);
		ContentBrowserAssetExtraStateDelegateHandles.Emplace(
			ContentBrowserModule.AddAssetViewExtraStateGenerator(InspectionIndicator));
	}
}

void FContentInspectorModule::RemoveContentBrowserHooks()
{
	FContentBrowserModule* ContentBrowserModule =
		FModuleManager::GetModulePtr<FContentBrowserModule>(TEXT("ContentBrowser"));
	if (ContentBrowserModule)
	{
		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBMenuExtenderDelegates =
			ContentBrowserModule->GetAllAssetViewContextMenuExtenders();
		CBMenuExtenderDelegates.RemoveAll(
			[this](const FContentBrowserMenuExtender_SelectedAssets& Delegate)
			{
				return Delegate.GetHandle() == ContentBrowserAssetExtenderDelegateHandle;
			});

		TArray<FContentBrowserMenuExtender_SelectedPaths>& CBFolderMenuExtenderDelegates =
			ContentBrowserModule->GetAllPathViewContextMenuExtenders();
		CBFolderMenuExtenderDelegates.RemoveAll(
			[this](const FContentBrowserMenuExtender_SelectedPaths& Delegate)
			{
				return Delegate.GetHandle() == ContentBrowserPathExtenderDelegateHandle;
			});

		for (const FDelegateHandle& DelegateHandle : ContentBrowserAssetExtraStateDelegateHandles)
		{
			if (DelegateHandle.IsValid())
				ContentBrowserModule->RemoveAssetViewExtraStateGenerator(DelegateHandle);
		}
	}
}

void FContentInspectorModule::CreatePackageEventsHook()
{
	// Back up 'package ok to save delegate' and install ours
	OkToSaveBackupDelegate = FCoreUObjectDelegates::IsPackageOKToSaveDelegate;
	FCoreUObjectDelegates::IsPackageOKToSaveDelegate.BindRaw(this, &FContentInspectorModule::CanSavePackage);

	// Register Asset Registry Events
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>("AssetRegistry");
	AssetRegistryModule.Get().OnInMemoryAssetCreated().AddRaw(this, &FContentInspectorModule::AssetCreated);
	AssetRegistryModule.Get().OnAssetRenamed().AddRaw(this, &FContentInspectorModule::AssetRenamed);
}

void FContentInspectorModule::RemovePackageEventsHook()
{
	// Restore 'is ok to save package' delegate
	if (OkToSaveBackupDelegate.IsBound())
	{
		FCoreUObjectDelegates::IsPackageOKToSaveDelegate = OkToSaveBackupDelegate;
		OkToSaveBackupDelegate.Unbind();
	}
}

bool FContentInspectorModule::CanSavePackage(UPackage* InPackage, const FString& InFilename, FOutputDevice* ErrorLog)
{
	bool bResult = true;

	if (UContentInspectorSettings::Get()->CanRejectSavingAsset())
	{
		if (const UObject* AssetForSave = LoadObject<UObject>(nullptr, *InPackage->GetName()))
		{
			//	Ignore saving redirectors
			if (!AssetForSave->IsA(UObjectRedirector::StaticClass()))
			{
				const FAssetData& AssetDataForCheck { AssetForSave };

				// Synchronous inspect the asset
				InspectionsManager->SendInspectionsRequest(AssetDataForCheck);

				//	Trying to save a new asset?
				if (NewAssets.Contains(AssetForSave))
				{
					int32 CriticalFailed = 0;

					// Reject saving the asset if critical inspections failed
					if (InspectionsManager->HaveFailedInspections(AssetDataForCheck.PackageName, CriticalFailed)
						&& CriticalFailed > 0)
					{
						ErrorLog->Log(
							TEXT("ContentInspector"),
							ELogVerbosity::Warning,
							FString::Printf(
								TEXT("Package %s cannot be saved. %d Critical inspections failed."),
								*InPackage->GetName(),
								CriticalFailed));
						bResult = false;
					}
				}
			}
		}
	}

	return bResult;
}

void FContentInspectorModule::AssetCreated(UObject* InAsset)
{
	if (const auto ObjectRedirector = Cast<UObjectRedirector>(InAsset))
	{
		NewAssets.AddUnique(ObjectRedirector->DestinationObject);
	}
	else
	{
		NewAssets.AddUnique(InAsset);
	}
}

void FContentInspectorModule::AssetRenamed(const FAssetData& InAssetData, const FString& InOldName)
{
	auto IsNameMatch = [InAssetData](const TSoftObjectPtr<UObject> InAsset)
	{
		return FAssetData { InAsset.Get() }.AssetName == InAssetData.AssetName;
	};

	const int32& FoundAssetIndex = NewAssets.FindLastByPredicate(IsNameMatch);
	if (FoundAssetIndex != INDEX_NONE)
	{
		NewAssets.RemoveAtSwap(FoundAssetIndex);
		NewAssets.Add(InAssetData.GetAsset());
	}
}

TSharedRef<SWidget> FContentInspectorModule::GenerateAssetViewExtraStateIndicators(const FAssetData& AssetData)
{
	if (InspectionsManager)
	{
		const UClass* AssetClass = AssetData.GetClass();
		if (IsValid(AssetClass))
		{
			const FName& AssetClassName = AssetClass->GetFName();
			const FName& AssetPackageName = AssetData.PackageName;

			if (InspectionsManager->GetInspectionsStructFromConfig(AssetClassName) != INDEX_NONE)
			{
				if (InspectionsManager->IsNeedLoadAsset(AssetClassName))
				{
					InspectionsManager->StartAssetAsyncLoad(AssetData);
				}
				else
				{
					InspectionsManager->AssetAsyncLoadComplete(AssetData);
				}
			}

			return SNew(SButton)
			.ButtonStyle(FCoreStyle::Get(), "NoBorder")
			.ContentPadding(0.f)
			.Visibility_UObject(InspectionsManager, &UInspectionsManager::GetIndicatorVisibility, AssetClassName)
			.OnClicked_Raw(this, &FContentInspectorModule::CreateInspectionResultWindow, TArray<FAssetData> {AssetData})
			.ButtonColorAndOpacity(FLinearColor::Transparent)
			[
				SNew(SImage)
				.Image_UObject(InspectionsManager, &UInspectionsManager::GetIndicatorBrush, AssetPackageName)
				.ToolTipText_UObject(InspectionsManager, &UInspectionsManager::GetShortDescription, AssetPackageName)
			];
		}
	}
	{
		return SNew( SButton )
		.Visibility( EVisibility::Hidden );
	}
}

FReply FContentInspectorModule::CreateInspectionResultWindow(TArray<FAssetData> InAssetsData)
{
	//	Fill progress bar
	FScopedSlowTask ExploreInspectionTask(InAssetsData.Num());
	ExploreInspectionTask.MakeDialog();

	if (InspectionsManager)
	{
		for (const FAssetData& AssetData : InAssetsData)
		{
			FText ValidatingMessage =
				FText::Format(LOCTEXT("InspectingAssetName", "Inspecting: {0}"), FText::FromName(AssetData.AssetName));
			ExploreInspectionTask.EnterProgressFrame(1, ValidatingMessage);

			InspectionsManager->SendInspectionsRequest(AssetData);
		}
	}

	TArray<FAssetData> AssetsWithFailedInspections;
	UInspectionsManager* Manager = InspectionsManager;

	auto IsAnyInspectionFailed = [Manager](const FAssetData& InAsset)
	{
		int32 CriticalFailed = 0;
		return Manager->HaveFailedInspections(InAsset.PackageName, CriticalFailed);
	};

	Algo::CopyIf(InAssetsData, AssetsWithFailedInspections, IsAnyInspectionFailed);

	TArray<FInspectionResult> AllInspectionsResults;

	for (const FAssetData& AssetData : AssetsWithFailedInspections)
	{
		TArray<FInspectionResult> InspectionResultForCurrentAsset;
		InspectionsManager->GetInspectionResults(AssetData.PackageName, InspectionResultForCurrentAsset);

		AllInspectionsResults.Append(InspectionResultForCurrentAsset);
	}

	if (AssetsWithFailedInspections.Num() > 0)
	{
		if (InspectionWindow.IsValid())
			InspectionWindow->RequestDestroyWindow();

		const float WindowHeight = FMath::Clamp(AssetsWithFailedInspections.Num() * 120.f, 120.f, 1080.f);

		InspectionWindow = FSlateApplication::Get().AddWindow(
			SNew(SWindow)
			.ClientSize(FVector2D(900.f, WindowHeight))
			.IsTopmostWindow(true)
			.Title(LOCTEXT("ContentInspector", "Inspection result"))
			[
				SNew(SSInspectionsWindow)
				.AssetsData(AssetsWithFailedInspections)
				.InspectionResults(AllInspectionsResults)
				.AssetThumbnailPool(ThumbnailPool)
			]
		);
	}

	return FReply::Handled();
}

void FContentInspectorModule::InspectSelectionAsset(TArray<FAssetData> SelectedAssets)
{
	CreateInspectionResultWindow(SelectedAssets);
}

void FContentInspectorModule::InspectAssetsInSelectionFolder(TArray<FString> InSelectedPaths)
{
	FString FormattedSelectedPaths;

	const int32 LastIndex = InSelectedPaths.Num() - 1;
	for (int32 i = 0; i < LastIndex; ++i)
	{
		FormattedSelectedPaths.Append(InSelectedPaths[i]);
		FormattedSelectedPaths.Append(LINE_TERMINATOR);
	}
	FormattedSelectedPaths.Append(InSelectedPaths[LastIndex]);

	FFormatNamedArguments Args;
	Args.Add(TEXT("Paths"), FText::FromString(FormattedSelectedPaths));
	const EAppReturnType::Type Result = FMessageDialog::Open(
		EAppMsgType::YesNo,
		FText::Format(
			LOCTEXT(
				"ContentInspectorConfirmation",
				"Are you sure you want to inspect the content inside the folders?\n\n{Paths}"),
			Args));
	if (Result == EAppReturnType::Yes)
	{
		InspectFolders(InSelectedPaths);
	}
}

void FContentInspectorModule::InspectFolders(const TArray<FString>& InSelectedPaths)
{
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

	// Form a filter from the paths
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	for (const FString& Folder : InSelectedPaths)
	{
		Filter.PackagePaths.Emplace(*Folder);
	}

	// Query for a list of assets in the selected paths
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	InspectSelectionAsset(AssetList);
}

TSharedRef<FExtender> FContentInspectorModule::OnExtendContentBrowserAssetSelectionMenu(
	const TArray<FAssetData>& InSelectedAssets)
{
	TSharedRef<FExtender> Extender(new FExtender());

	// Add the sprite actions sub-menu extender
	Extender->AddMenuExtension(
		"GetAssetActions",
		EExtensionHook::Before,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(
			this,
			&FContentInspectorModule::CreateContentInspectorBrowserAssetMenu,
			InSelectedAssets));

	return Extender;
}

TSharedRef<FExtender> FContentInspectorModule::OnExtendContentBrowserPathSelectionMenu(
	const TArray<FString>& InSelectedPaths)
{
	TSharedRef<FExtender> Extender(new FExtender());

	// Add the sprite actions sub-menu extender
	Extender->AddMenuExtension(
		"Section",
		EExtensionHook::Before,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(
			this,
			&FContentInspectorModule::CreateContentInspectorBrowserPathSelectionMenu,
			InSelectedPaths));

	return Extender;
}

void FContentInspectorModule::CreateContentInspectorBrowserAssetMenu(
	FMenuBuilder& InMenuBuilder,
	TArray<FAssetData> InSelectedAssets)
{
	InMenuBuilder.BeginSection(NAME_None, LOCTEXT("ContentInspector_MenuSection", "Content Inspector"));
	{
		InMenuBuilder.AddMenuEntry(
			NSLOCTEXT("AssetTypeActions", "ObjectContext_ContentInspector", "Inspect Asset"),
			NSLOCTEXT("AssetTypeActions", "ObjectContext_ContentInspector", "Inspect Asset"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateRaw(this, &FContentInspectorModule::InspectSelectionAsset, InSelectedAssets)));
	}
	InMenuBuilder.EndSection();
}

void FContentInspectorModule::CreateContentInspectorBrowserPathSelectionMenu(
	FMenuBuilder& InMenuBuilder,
	TArray<FString> InSelectedPaths)
{
	InMenuBuilder.BeginSection(NAME_None, LOCTEXT("ContentInspector_MenuSection", "Content Inspector"));
	{
		InMenuBuilder.AddMenuEntry(
			NSLOCTEXT("AssetTypeActions", "ObjectContext_ContentInspector", "Inspect Asset"),
			NSLOCTEXT("AssetTypeActions", "ObjectContext_ContentInspector", "Inspect Asset"),
			FSlateIcon(),
			FUIAction(FExecuteAction::CreateRaw(
				this,
				&FContentInspectorModule::InspectAssetsInSelectionFolder,
				InSelectedPaths)));
	}
	InMenuBuilder.EndSection();
}

void FContentInspectorModule::CreateInspectionsManager()
{
	if (!InspectionsManager)
	{
		InspectionsManager = NewObject<UInspectionsManager>();
		InspectionsManager->AddToRoot();
	}
}

void FContentInspectorModule::RemoveInspectionManager()
{
	if (IsValid(InspectionsManager))
		InspectionsManager->RemoveFromRoot();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FContentInspectorModule, ContentInspector)
