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
#include "AssetRegisterEditor.h"
#include "AssetRegisterCommands.h"
#include "AssetRegisterSettings.h"
#include "AssetRegisterStyle.h"
#include "LODTablesManager.h"
#include "SAssetRegisterBrowser.h"
#include "SAssetRegisterParamDialog.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "ContentBrowserModule.h"
#include "Framework/Notifications/NotificationManager.h"
#include "LevelEditor.h"
#include "Widgets/Notifications/SNotificationList.h"
DEFINE_LOG_CATEGORY(LogAssetRegister)

#define LOCTEXT_NAMESPACE "AssetRegisterEditorModule"

namespace FAssetRegisterEditorModuleLocal
{
	const FName AssetBrowserTabName = TEXT("AssetRegisterBrowser");
}

void FAssetRegisterEditorModule::StartupModule()
{
	FAssetRegisterStyle::Initialize();
	FAssetRegisterStyle::ReloadTextures();
	FAssetRegisterCommands::Register();

	CreateContentBrowserHooks();

	PluginCommands = MakeShareable(new FUICommandList);
	PluginCommands->MapAction(
		FAssetRegisterCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FAssetRegisterEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");

	TSharedPtr<FExtender> MenuExtender = MakeShareable(new FExtender());
	MenuExtender->AddMenuExtension(
		"WindowLayout",
		EExtensionHook::After,
		PluginCommands,
		FMenuExtensionDelegate::CreateRaw(this, &FAssetRegisterEditorModule::AddMenuExtension));
	LevelEditorModule.GetMenuExtensibilityManager()->AddExtender(MenuExtender);

	TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
	ToolbarExtender->AddToolBarExtension(
		"Settings",
		EExtensionHook::After,
		PluginCommands,
		FToolBarExtensionDelegate::CreateRaw(this, &FAssetRegisterEditorModule::AddToolbarExtension));
	LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
}

void FAssetRegisterEditorModule::ShutdownModule()
{
	RemoveContentBrowserHooks();
	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(FAssetRegisterEditorModuleLocal::AssetBrowserTabName);
	FAssetRegisterStyle::Shutdown();
	FAssetRegisterCommands::Unregister();
}

void FAssetRegisterEditorModule::PluginButtonClicked()
{
	if (!AssetRegisterTab.Pin().IsValid())
	{
		FGlobalTabmanager::Get()
			->RegisterNomadTabSpawner(
				FAssetRegisterEditorModuleLocal::AssetBrowserTabName,
				FOnSpawnTab::CreateRaw(this, &FAssetRegisterEditorModule::SpawnAssetRegisterBrowser))
			.SetDisplayName(LOCTEXT("Asset Register Title", "Asset Register Browser"));
	}

	FGlobalTabmanager::Get()->TryInvokeTab(FAssetRegisterEditorModuleLocal::AssetBrowserTabName);
}

TSharedRef<SDockTab> FAssetRegisterEditorModule::SpawnAssetRegisterBrowser(const FSpawnTabArgs& Args)
{
	return SAssignNew(AssetRegisterTab, SDockTab)
	.TabRole(ETabRole::NomadTab)
	[
		SAssignNew(AssetRegisterUI, SAssetRegisterBrowser)
		.NativeColumnsNames(CollectColumnNamesForHiding())
	];
}

bool FAssetRegisterEditorModule::CreateAssetRegisterMark(
	const FAssetData& InAssetData,
	const FAssetRegisterMarkTagsSettings* InTagsSettings)
{
	bool bResult = false;

	if (UObject* AssetObject = InAssetData.GetAsset())
	{
		if (IInterface_AssetUserData* Interface = Cast<IInterface_AssetUserData>(AssetObject))
		{
			if (InTagsSettings && InTagsSettings->PrimaryTag.IsValid())
			{
				Interface->RemoveUserDataOfClass(UAssetRegisterMark::StaticClass());

				auto AssetRegisterMark = NewObject<UAssetRegisterMark>(AssetObject);
				AssetRegisterMark->PrimaryTag = InTagsSettings->PrimaryTag;
				AssetRegisterMark->AdditionalTags = InTagsSettings->AdditionalTags;
				Interface->AddAssetUserData(AssetRegisterMark);
				AssetObject->MarkPackageDirty();
				bResult = true;
			}
		}
	}

	return bResult;
}

void FAssetRegisterEditorModule::ShowNotification(FString InNotification, float InDuration, bool IsFailed)
{
	FNotificationInfo Info(FText::FromString(InNotification));
	Info.ExpireDuration = InDuration;
	IsFailed ? Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.FailImage"))
			 : Info.Image = FEditorStyle::GetBrush(TEXT("NotificationList.SuccessImage"));

	FSlateNotificationManager::Get().AddNotification(Info);
}

void FAssetRegisterEditorModule::AddToolbarExtension(FToolBarBuilder& Builder)
{
	Builder.AddToolBarButton(FAssetRegisterCommands::Get().OpenPluginWindow);
}

void FAssetRegisterEditorModule::AddMenuExtension(FMenuBuilder& Builder)
{
	Builder.AddMenuEntry(FAssetRegisterCommands::Get().OpenPluginWindow);
}

void FAssetRegisterEditorModule::CreateContentBrowserHooks()
{
	if (FSlateApplication::IsInitialized())
	{
		FContentBrowserModule& ContentBrowserModule =
			FModuleManager::LoadModuleChecked<FContentBrowserModule>(TEXT("ContentBrowser"));

		TArray<FContentBrowserMenuExtender_SelectedAssets>& CBAssetMenuExtenderDelegates =
			ContentBrowserModule.GetAllAssetViewContextMenuExtenders();

		CBAssetMenuExtenderDelegates.Add(FContentBrowserMenuExtender_SelectedAssets::CreateRaw(
			this,
			&FAssetRegisterEditorModule::OnExtendContentBrowserAssetSelectionMenu));
		ContentBrowserAssetExtenderDelegateHandle = CBAssetMenuExtenderDelegates.Last().GetHandle();
	}
}

void FAssetRegisterEditorModule::RemoveContentBrowserHooks()
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
	}
}

TSharedRef<FExtender> FAssetRegisterEditorModule::OnExtendContentBrowserAssetSelectionMenu(
	const TArray<FAssetData>& InSelectedAssets)
{
	TSharedRef<FExtender> Extender(new FExtender());

	Extender->AddMenuExtension(
		"GetAssetActions",
		EExtensionHook::Before,
		nullptr,
		FMenuExtensionDelegate::CreateRaw(
			this,
			&FAssetRegisterEditorModule::CreateAssetRegisterBrowserAssetMenu,
			InSelectedAssets));

	return Extender;
}

void FAssetRegisterEditorModule::CreateAssetRegisterBrowserAssetMenu(
	FMenuBuilder& InMenuBuilder,
	TArray<FAssetData> InSelectedAssets)
{
	// Create Section
	InMenuBuilder.BeginSection("Asset Register", LOCTEXT("AssetRegisterActions", "Asset Register"));
	{
		// Create a Submenu inside of the Section
		InMenuBuilder.AddSubMenu(
			FText::FromString("Asset Register"),
			FText::FromString("Asset Register tools"),
			FNewMenuDelegate::CreateRaw(this, &FAssetRegisterEditorModule::FillSubmenu, InSelectedAssets));
	}
	InMenuBuilder.EndSection();
}

void FAssetRegisterEditorModule::FillSubmenu(FMenuBuilder& MenuBuilder, TArray<FAssetData> InSelectedAssets)
{
	MenuBuilder.AddMenuEntry(
		FText::FromString("Create Asset Register Mark"),
		FText::FromString("Create asset register mark, and assign tags"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateRaw(this, &FAssetRegisterEditorModule::CreateWindowWithSettings, InSelectedAssets)));
}

void FAssetRegisterEditorModule::CreateWindowWithSettings(TArray<FAssetData> InSelectedAssets)
{
	TSharedRef<FStructOnScope> TagsSettings =
		MakeShared<FStructOnScope>(FAssetRegisterMarkTagsSettings::StaticStruct());
	FAssetRegisterMarkTagsSettings* TagsSettingsRef =
		reinterpret_cast<FAssetRegisterMarkTagsSettings*>(TagsSettings->GetStructMemory());

	UObject* FirstSelectedAsset = InSelectedAssets[0].GetAsset();
	if (UAssetRegisterMark* AssetRegisterMark = UAssetRegisterMark::GetAssetRegisterMark(FirstSelectedAsset))
	{
		TagsSettingsRef->PrimaryTag = AssetRegisterMark->PrimaryTag;
		TagsSettingsRef->AdditionalTags = AssetRegisterMark->AdditionalTags;
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
	.Title(FText::FromString(TEXT("Asset Register - Create Asset Register Mark")))
	.ClientSize(FVector2D(500, 200));

	TSharedPtr<SAssetRegisterParamDialog> Dialog;
	Window->SetContent(SAssignNew(Dialog, SAssetRegisterParamDialog, Window, TagsSettings).OkButtonText(LOCTEXT("OKButton", "OK")));
	GEditor->EditorAddModalWindow(Window);

	if (Dialog->bOKPressed)
	{
		for (const FAssetData& AssetData : InSelectedAssets)
		{
			const bool bSuccess = CreateAssetRegisterMark(AssetData, TagsSettingsRef);
			if (bSuccess)
			{
				UObject* AssetObject = AssetData.GetAsset();
				if (UStaticMesh* AssetStaticMesh = Cast<UStaticMesh>(AssetObject))
				{
					UAssetRegisterMark* AssetRegisterMark = UAssetRegisterMark::GetAssetRegisterMark(AssetObject);
					SetLODsToStaticMesh(AssetStaticMesh, AssetRegisterMark);
				}
			}
			const FString Notification =
				bSuccess ? "Register mark has been assigned" : "Register mark has not been assigned";
			ShowNotification(Notification, 2.f, !bSuccess);
		}
	}
}

TArray<FString> FAssetRegisterEditorModule::CollectColumnNamesForHiding()
{
	TArray<FString> Result {};
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	IAssetRegistry* AssetRegistry = &AssetRegistryModule.Get();

	TArray<FAssetData> AllAssetData;
	AssetRegistry->Get()->GetAllAssets(AllAssetData, true);

	for (const FAssetData& AssetData : AllAssetData)
	{
		if (AssetData.IsUAsset())
		{
			AssetData.TagsAndValues.ForEach(
				[&Result](TPair<FName, FAssetTagValueRef> Pair)
				{
					Result.Add(Pair.Key.ToString());
				});
		}
	}

	return Result;
}

bool FAssetRegisterEditorModule::SetLODsToStaticMesh(
	UStaticMesh* AssetStaticMesh,
	const UAssetRegisterMark* AssetRegisterMark)
{
	if (const UAssetRegisterSettings* AssetRegisterSettings = UAssetRegisterSettings::Get())
	{
		ULODTablesManager* DataAsset = Cast<ULODTablesManager>(AssetRegisterSettings->LODsDataAsset.TryLoad());

		if (!DataAsset->LODs.Contains(AssetRegisterMark->PrimaryTag))
		{
			return false;
		}

		UDataTable* Table = *DataAsset->LODs.Find(AssetRegisterMark->PrimaryTag);
		TArray<FName> RowNames = Table->GetRowNames();

		const FBoxSphereBounds AssetLocalBound = AssetStaticMesh->GetBounds();
		const float AssetSphereRadius = AssetLocalBound.SphereRadius;

		for (int32 Index = 0; Index < RowNames.Num(); Index++)
		{
			FString ContextString;
			if (Index == 0)
			{
				continue;
			}

			const FName CurrentRowName = RowNames[Index];
			const FName PreviewRowName = RowNames[Index - 1];
			const FLodDistancesStruct* PreviewRow = Table->FindRow<FLodDistancesStruct>(PreviewRowName, ContextString);
			const FLodDistancesStruct* CurrentRow = Table->FindRow<FLodDistancesStruct>(CurrentRowName, ContextString);
			const float PreviewRowSize = PreviewRow->BoundingSphereSize;
			const float CurrentRowSize = CurrentRow->BoundingSphereSize;

			if (PreviewRowSize <= AssetSphereRadius && AssetSphereRadius < CurrentRowSize)
			{
				const int32 NumLODs = AssetStaticMesh->GetNumLODs();
				TArray<float> LODsValues;
				GetLODsValue(PreviewRow, LODsValues);

				for (int32 LODIdx = 0; LODIdx < NumLODs; LODIdx++)
				{
					SetMeshLODScreenSize(AssetStaticMesh, LODIdx, LODsValues[LODIdx]);
				}
				return true;
			}
		}
	}
	return false;
}

void FAssetRegisterEditorModule::GetLODsValue(const FLodDistancesStruct* Row, TArray<float>& LODsValues)
{
	// hardcode
	LODsValues.Add(Row->LOD0ScreenSize);
	LODsValues.Add(Row->LOD1ScreenSize);
	LODsValues.Add(Row->LOD2ScreenSize);
	LODsValues.Add(Row->LOD3ScreenSize);
	LODsValues.Add(Row->LOD4ScreenSize);
	LODsValues.Add(Row->LOD5ScreenSize);
	LODsValues.Add(Row->LOD6ScreenSize);
	LODsValues.Add(Row->LOD7ScreenSize);
}

void FAssetRegisterEditorModule::SetMeshLODScreenSize(UStaticMesh* StaticMesh, int32 LODIndex, float ScreenSize)
{
	if (StaticMesh == nullptr)
	{
		return;
	}

	// If LOD 0 does not exist, warn and return
	if (StaticMesh->GetNumSourceModels() == 0)
	{
		UE_LOG(LogAssetRegister, Warning, TEXT("LOD 0 doesn't exist"));
		return;
	}

	// Close the mesh editor to prevent crashing. If changes are applied, reopen it after the mesh has been built.
	bool bStaticMeshIsEdited = false;
	UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
	if (AssetEditorSubsystem->FindEditorForAsset(StaticMesh, false))
	{
		AssetEditorSubsystem->CloseAllEditorsForAsset(StaticMesh);
		bStaticMeshIsEdited = true;
	}

	// Set up LOD
	StaticMesh->bAutoComputeLODScreenSize = false;
	StaticMesh->GetSourceModel(LODIndex).ScreenSize = ScreenSize;
	StaticMesh->PostEditChange();
	StaticMesh->MarkPackageDirty();

	// Reopen MeshEditor on this mesh if the MeshEditor was previously opened in it
	if (bStaticMeshIsEdited)
	{
		AssetEditorSubsystem->OpenEditorForAsset(StaticMesh);
	}
}

void UStaticMeshToolset::ReloadStaticMeshLODsByTag(TArray<UObject*> SelectedObjects)
{
	for (const FAssetData& AssetData : SelectedObjects)
	{
		UAssetRegisterMark* AssetRegisterMark = FindAssetRegisterMark(AssetData);
		UStaticMesh* AssetStaticMesh = Cast<UStaticMesh>(AssetData.GetAsset());
		if (IsValid(AssetRegisterMark) && IsValid(AssetStaticMesh))
		{
			FAssetRegisterEditorModule::SetLODsToStaticMesh(AssetStaticMesh, AssetRegisterMark);
		}
	}
}

UAssetRegisterMark* UStaticMeshToolset::FindAssetRegisterMark(const FAssetData& InAssetData)
{
	if (UObject* AssetObject = InAssetData.GetAsset())
	{
		UAssetRegisterMark* AssetRegisterMark = UAssetRegisterMark::GetAssetRegisterMark(AssetObject);

		if (IsValid(AssetRegisterMark) && AssetRegisterMark->PrimaryTag.IsValid())
		{
			return AssetRegisterMark;
		}
	}

	return nullptr;
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FAssetRegisterEditorModule, AssetRegisterEditor)
