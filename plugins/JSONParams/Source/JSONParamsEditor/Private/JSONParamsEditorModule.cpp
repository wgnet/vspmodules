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
#include "JSONParamsEditorModule.h"

#include "CreateJSONParamWidget.h"
#include "JSONParamsCommands.h"
#include "JSONParamsStyle.h"
#include "ParamCustomization/ParamRegistryInfoCustomization.h"
#include "ParamsRegistry.h"

#include "SlateOptMacros.h"
#include "ToolMenus.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Text/STextBlock.h"


static const FName JSONParamsTabName("CreateJSONParams");


void FJSONParamsEditorModule::StartupModule()
{
	FJSONParamsStyle::Initialize();
	FJSONParamsStyle::ReloadTextures();

	FJSONParamsCommands::Register();

	PluginCommands = MakeShareable(new FUICommandList);

	PluginCommands->MapAction(
		FJSONParamsCommands::Get().OpenPluginWindow,
		FExecuteAction::CreateRaw(this, &FJSONParamsEditorModule::PluginButtonClicked),
		FCanExecuteAction());

	UToolMenus::RegisterStartupCallback(
		FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FJSONParamsEditorModule::RegisterMenus));

	FGlobalTabmanager::Get()
		->RegisterNomadTabSpawner(
			JSONParamsTabName,
			FOnSpawnTab::CreateRaw(this, &FJSONParamsEditorModule::OnSpawnPluginTab))
		.SetDisplayName(FText::FromString("Create JSON Params"))
		.SetMenuType(ETabSpawnerMenuType::Hidden);

	FEditorDelegates::BeginPIE.AddRaw(this, &FJSONParamsEditorModule::OnBeginPIE);

	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomPropertyTypeLayout(
		FParamRegistryInfo::StaticStruct()->GetFName(),
		FOnGetPropertyTypeCustomizationInstance::CreateStatic(&FParamRegistryInfoCustomization::MakeInstance));

	PropertyModule.NotifyCustomizationModuleChanged();
}

void FJSONParamsEditorModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback(this);

	UToolMenus::UnregisterOwner(this);

	FJSONParamsStyle::Shutdown();

	FJSONParamsCommands::Unregister();

	FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(JSONParamsTabName);

	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule =
			FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomPropertyTypeLayout(FParamRegistryInfo::StaticStruct()->GetFName());

		PropertyModule.NotifyCustomizationModuleChanged();
	}
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SDockTab> FJSONParamsEditorModule::OnSpawnPluginTab(const FSpawnTabArgs& SpawnTabArgs)
{
	TSharedPtr<SDockTab> DockTab;
	SAssignNew(DockTab, SDockTab)
		.TabRole(ETabRole::NomadTab)
		[
			SNew(SBox)
	       .HAlign(HAlign_Fill)
	       .VAlign(VAlign_Fill)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew(SCreateJSONParamWidget)
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .Padding(FMargin(4.0f, 8.0f, 4.0f, 0.0f))
				[
					SNew(STextBlock)
					.Text(FText::FromString("JSON Registry info:"))
					.TextStyle(FEditorStyle::Get(), "RichTextBlock.Bold")
				]
				+ SVerticalBox::Slot()
				  .FillHeight(1.f)
				  .Padding(FMargin(4.0f, 2.0f, 4.0f, 0.0f))
				[
					SNew(SScrollBox)
					.ScrollBarAlwaysVisible(false)
					+ SScrollBox::Slot()
					  .HAlign(HAlign_Fill)
					  .VAlign(VAlign_Fill)
					[
						SAssignNew(TextBlock, SEditableText)
						.IsReadOnly(true)
						.Text(FText::FromString("Initializing..."))
					]
				]
				+ SVerticalBox::Slot()
				  .AutoHeight()
				  .VAlign(VAlign_Fill)
				  .HAlign(HAlign_Left)
				[
					SNew(SButton)
					.OnClicked_Raw(this, &FJSONParamsEditorModule::OnReInit)
					.Content()
					[
						SNew(STextBlock)
						.Text(FText::FromString("Re-init"))
					]
				]
			]
		];

	auto UpdateText = [this]()
	{
		TextBlock->SetText(FParamsRegistry::Get().GetRegistryInfoText());
	};
	FParamsRegistry::Get().CallWhenInitialized(UpdateText);
	FParamsRegistry::Get().OnParamsReloaded.AddLambda(UpdateText);

	return DockTab.ToSharedRef();
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FJSONParamsEditorModule::OnBeginPIE(bool bIsSimulating)
{
	if (!bIsSimulating)
		FParamsRegistry::Get().Init();
}

FReply FJSONParamsEditorModule::OnReInit()
{
	TextBlock->SetText(FText::FromString("Re-initializing..."));

	FParamsRegistry::Get().Init();

	return FReply::Handled();
}

void FJSONParamsEditorModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab(JSONParamsTabName);
}

void FJSONParamsEditorModule::RegisterMenus()
{
	// Owner will be used for cleanup in call to UToolMenus::UnregisterOwner
	FToolMenuOwnerScoped OwnerScoped(this);

	UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Window");
	{
		FToolMenuSection& Section = Menu->AddSection("JSONParams", FText::FromString("JSONParams"));
		Section.AddSubMenu(
			"JSONParams",
			FText::FromString("JSONParams"),
			FText(),
			FNewToolMenuDelegate::CreateLambda(
				[this](UToolMenu* AlignmentMenu)
				{
					FToolMenuSection& InSection =
						AlignmentMenu->AddSection("JSONParams", FText::FromString("JSONParams"));
					InSection.AddMenuEntryWithCommandList(FJSONParamsCommands::Get().OpenPluginWindow, PluginCommands);
				}),
			false,
			FSlateIcon(FJSONParamsStyle::GetStyleSetName(), "JSONParams.DefaultIcon40"));
	}
}

IMPLEMENT_MODULE(FJSONParamsEditorModule, JSONParamsEditor)
