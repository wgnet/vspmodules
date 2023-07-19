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

#include "HitboxTool.h"

#include "Animation/DebugSkelMeshComponent.h"
#include "AssetViewerSettings.h"
#include "Engine/SkeletalMeshEditorData.h"
#include "HitboxToolStyle.h"
#include "HitboxViewer.h"
#include "IPersonaToolkit.h"
#include "ISkeletalMeshEditorModule.h"
#include "ISkeletonEditorModule.h"
#include "PersonaModule.h"
#include "SHitboxPreviewWidget.h"
#include "SkeletalMeshEditor/Private/SkeletalMeshEditor.h"
#include "ToolMenus.h"
#include "Widgets/Layout/SScrollBox.h"
#include "WorkflowOrientedApp/ApplicationMode.h"
#include "WorkflowOrientedApp/WorkflowTabManager.h"


#define LOCTEXT_NAMESPACE "FHitboxToolModule"
TMap<ISkeletalMeshEditor*, TWeakObjectPtr<UHitboxViewer> > FHitboxToolModule::EditorTabMap;


struct FHitboxToolTabSummoner : public FWorkflowTabFactory
{
public:
	/** Tab ID name */
	static const FName TabName;

	FHitboxToolTabSummoner(TSharedPtr<class FAssetEditorToolkit> InHostingApp)
		: FWorkflowTabFactory(TabName, InHostingApp)
	{
		TabLabel = LOCTEXT("HitboxToolTabLabel", "HitboxTool");
		TabIcon = FSlateIcon(FHitboxToolStyle::GetStyleSetName(), TEXT("SkeletalMeshEditor.HitboxTool"));
		UAssetViewerSettings::Get()->OnAssetViewerProfileAddRemoved().AddRaw(
			this,
			&FHitboxToolTabSummoner::RebuildHitboxes);
	}

	~FHitboxToolTabSummoner()
	{
		UAssetViewerSettings::Get()->OnAssetViewerProfileAddRemoved().RemoveAll(this);
	}

	virtual TSharedRef<SWidget> CreateTabBody(const FWorkflowTabSpawnInfo& Info) const override;

	virtual FText GetTabToolTipText(const FWorkflowTabSpawnInfo& Info) const override
	{
		return LOCTEXT("HitboxToolTabToolTip", "Tab for HitboxViewer properties");
	}

	static TSharedPtr<FWorkflowTabFactory> CreateFactory(TSharedPtr<FAssetEditorToolkit> InAssetEditor)
	{
		return MakeShareable(new FHitboxToolTabSummoner(InAssetEditor));
	}

	void RebuildHitboxes()
	{
		ISkeletalMeshEditor* SkeletalMeshEditor = static_cast<ISkeletalMeshEditor*>(HostingApp.Pin().Get());

		TSharedPtr<IPersonaToolkit> PersonaToolkit = SkeletalMeshEditor->GetPersonaToolkit();

		if (PersonaToolkit.IsValid())
		{
			if (UDebugSkelMeshComponent* PreviewMeshComponent = PersonaToolkit->GetPreviewMeshComponent())
			{
				if (UHitboxViewer* HitboxEditorSettings = FHitboxToolModule::GetInstanceToolSetting(SkeletalMeshEditor))
					HitboxEditorSettings->BuildHitboxes(PreviewMeshComponent);
			}
		}
	}

	virtual TSharedRef<SDockTab> SpawnTab(const FWorkflowTabSpawnInfo& Info) const override;

	void OnTabClosed(TSharedRef<SDockTab> Tab) const;
};

TSharedRef<SWidget> FHitboxToolTabSummoner::CreateTabBody(const FWorkflowTabSpawnInfo& Info) const
{
	auto SkeletalMeshEditor = static_cast<ISkeletalMeshEditor*>(HostingApp.Pin().Get());

	auto GetSettingsView = [SkeletalMeshEditor]()
	{
		FDetailsViewArgs DetailsViewArgs(false, false, true, FDetailsViewArgs::HideNameArea, false);
		DetailsViewArgs.bShowActorLabel = false;
		DetailsViewArgs.bShowOptions = false;

		TSharedRef<IDetailsView> SettingsView =
			FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor")
				.CreateDetailView(DetailsViewArgs);

		if (UHitboxViewer* HitboxEditorSettings = FHitboxToolModule::GetInstanceToolSetting(SkeletalMeshEditor))
		{
			if (UDebugSkelMeshComponent* PreviewMeshComponent =
					SkeletalMeshEditor->GetPersonaToolkit()->GetPreviewMeshComponent())
			{
				HitboxEditorSettings->BuildHitboxes(PreviewMeshComponent);
			}

			SettingsView->SetObject(HitboxEditorSettings);
		}

		return SettingsView->AsShared();
	};

	const TSharedPtr<SBox> TabContent = SNew( SBox )
		.ForceVolatile( true )
		.Content()
		[
			SNew( SBorder )
			.HAlign( HAlign_Center )
			.VAlign( VAlign_Top )
			.Padding( FMargin( 0, 3, 0, 0 ) )
			[
				SNew( SVerticalBox )
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				.Padding( 2 )
				[
					SNew( SHitboxPreviewWidget )
				]
				+ SVerticalBox::Slot()
				[
					GetSettingsView()
				]
			]
		];

	return TabContent.ToSharedRef();
}

TSharedRef<SDockTab> FHitboxToolTabSummoner::SpawnTab(const FWorkflowTabSpawnInfo& Info) const
{
	TSharedRef<SDockTab> Tab = FWorkflowTabFactory::SpawnTab(Info);
	Tab->SetOnTabClosed(SDockTab::FOnTabClosedCallback::CreateRaw(this, &FHitboxToolTabSummoner::OnTabClosed));

	return Tab;
}

void FHitboxToolTabSummoner::OnTabClosed(TSharedRef<SDockTab> Tab) const
{
	auto SkeletalMeshEditor = static_cast<ISkeletalMeshEditor*>(HostingApp.Pin().Get());

	if (UHitboxViewer* HitboxViewer = FHitboxToolModule::GetInstanceToolSetting(SkeletalMeshEditor))
		HitboxViewer->ClearAndMarkPendingKill();
}

const FName FHitboxToolTabSummoner::TabName = TEXT("HitboxTool");


void FHitboxToolModule::StartupModule()
{
	FHitboxToolStyle::Initialize();
	FHitboxToolStyle::ReloadTextures();

	if (GEditor)
	{
		UAssetEditorSubsystem* AssetEditorSubsystem = GEditor->GetEditorSubsystem<UAssetEditorSubsystem>();
		if (AssetEditorSubsystem)
		{
			AssetEditorSubsystem->OnAssetOpenedInEditor().AddLambda(
				[this](UObject* InAsset, IAssetEditorInstance* AssetEditorInstance)
				{
					if (USkeletalMesh* Asset = Cast<USkeletalMesh>(InAsset))
					{
						if (ISkeletalMeshEditor* SkeletalMeshEditorInstance =
								static_cast<ISkeletalMeshEditor*>(AssetEditorInstance))
						{
							SkeletalMeshEditorInstance->GetTabManager()->RegisterDefaultTabWindowSize(
								FHitboxToolTabSummoner::TabName,
								InitialWindowSize);

							TSharedRef<FExtender> Extender = MakeShared<FExtender>();

							TSharedRef<FUICommandList> CommandList = SkeletalMeshEditorInstance->GetToolkitCommands();
							Extender->AddToolBarExtension(
								"SkeletalMesh",
								EExtensionHook::After,
								CommandList,
								FToolBarExtensionDelegate::CreateRaw(
									this,
									&FHitboxToolModule::CreateToolbar,
									CommandList,
									SkeletalMeshEditorInstance));
							SkeletalMeshEditorInstance->AddToolbarExtender(Extender);
						}
					}
				});
		}
	}

	// Add application mode extender
	AppExtender = FWorkflowApplicationModeExtender::CreateRaw(this, &FHitboxToolModule::ExtendApplicationMode);
	FWorkflowCentricApplication::GetModeExtenderList().Add(AppExtender);
}

TSharedRef<FApplicationMode> FHitboxToolModule::ExtendApplicationMode(
	const FName ModeName,
	TSharedRef<FApplicationMode> InMode)
{
	// For skeleton and animation editor modes add our custom tab factory to it
	if (ModeName == TEXT("SkeletalMeshEditorMode"))
	{
		InMode->AddTabFactory(FCreateWorkflowTabFactory::CreateStatic(&FHitboxToolTabSummoner::CreateFactory));
		RegisteredApplicationModes.Add(InMode);
	}

	return InMode;
}

void FHitboxToolModule::ShutdownModule()
{
	ReleaseInstanceToolSetting();

	UnregisterEditorDelegates();

	FHitboxToolStyle::Shutdown();


	// Remove extender delegate
	FWorkflowCentricApplication::GetModeExtenderList().RemoveAll(
		[this](FWorkflowApplicationModeExtender& StoredExtender)
		{
			return StoredExtender.GetHandle() == AppExtender.GetHandle();
		});

	// During shutdown clean up all factories from any modes which are still active/alive
	for (TWeakPtr<FApplicationMode> WeakMode : RegisteredApplicationModes)
	{
		if (WeakMode.IsValid())
		{
			TSharedPtr<FApplicationMode> Mode = WeakMode.Pin();
			Mode->RemoveTabFactory(FHitboxToolTabSummoner::TabName);
		}
	}

	RegisteredApplicationModes.Empty();
}

void FHitboxToolModule::CreateToolbar(
	FToolBarBuilder& ToolbarBuilder,
	const TSharedRef<FUICommandList> CommandList,
	ISkeletalMeshEditor* InSkeletalMeshEditor)
{
	FSlateIcon SlateIcon = FSlateIcon(FHitboxToolStyle::GetStyleSetName(), TEXT("SkeletalMeshEditor.HitboxToolBig"));
	const FExecuteAction Action =
		FExecuteAction::CreateRaw(this, &FHitboxToolModule::ShowTabForEditor, InSkeletalMeshEditor);
	ToolbarBuilder.AddToolBarButton(
		Action,
		NAME_None,
		LOCTEXT("HitboxToolLabel", "Hitbox"),
		LOCTEXT("HitboxToolLabel", "Hitbox"),
		SlateIcon);
}

void FHitboxToolModule::ShowTabForEditor(ISkeletalMeshEditor* InSkeletalMeshEditor)
{
	InSkeletalMeshEditor->InvokeTab(FHitboxToolTabSummoner::TabName);
}

void FHitboxToolModule::OnLevelAddedToWorld(ULevel* Level, UWorld* World)
{
	// TODO OnLevelAddedToWorld
}

void FHitboxToolModule::OnMapOpened(const FString& Filename, bool bLoadAsTemplate)
{
	// TODO OnMapOpened
}

void FHitboxToolModule::OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources)
{
	// TODO OnWorldCleanup
}

UHitboxViewer* FHitboxToolModule::GetInstanceToolSetting(ISkeletalMeshEditor* InSkeletalMeshEditor)
{
	if (!EditorTabMap.FindOrAdd(InSkeletalMeshEditor).IsValid())
	{
		EditorTabMap.FindOrAdd(InSkeletalMeshEditor) =
			NewObject<UHitboxViewer>(GetTransientPackage(), NAME_None, RF_Standalone);
		EditorTabMap.FindOrAdd(InSkeletalMeshEditor)->ClearFlags(RF_Transactional);
	}

	check(EditorTabMap.FindOrAdd(InSkeletalMeshEditor).IsValid());
	return EditorTabMap.FindOrAdd(InSkeletalMeshEditor).Get();
}

void FHitboxToolModule::ReleaseInstanceToolSetting()
{
	for (TTuple<ISkeletalMeshEditor*, TWeakObjectPtr<UHitboxViewer> >& Iter : EditorTabMap)
	{
		if (Iter.Value.IsValid() && !Iter.Value->IsPendingKillOrUnreachable())
		{
			Iter.Value->ClearAndMarkPendingKill();
			Iter.Value->ClearFlags(RF_Standalone);
			Iter.Value.Reset();
		}
	}
	EditorTabMap.Empty();
}

void FHitboxToolModule::RegisterEditorDelegates()
{
	FWorldDelegates::LevelAddedToWorld.AddRaw(this, &FHitboxToolModule::OnLevelAddedToWorld);
	FEditorDelegates::OnMapOpened.AddRaw(this, &FHitboxToolModule::OnMapOpened);
	FWorldDelegates::OnWorldCleanup.AddRaw(this, &FHitboxToolModule::OnWorldCleanup);
}

void FHitboxToolModule::UnregisterEditorDelegates()
{
	FWorldDelegates::LevelAddedToWorld.RemoveAll(this);
	FEditorDelegates::OnMapOpened.RemoveAll(this);
	FWorldDelegates::OnWorldCleanup.RemoveAll(this);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FHitboxToolModule, HitboxTool)
