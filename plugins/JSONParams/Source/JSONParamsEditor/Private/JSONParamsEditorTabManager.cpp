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
#include "JSONParamsEditorTabManager.h"

#include "EditJSONParamWidget.h"
#include "JSONParamsCommands.h"
#include "JSONParamsEditorModule.h"
#include "JSONParamsStyle.h"
#include "ParamsRegistry.h"


FJSONParamsEditorTabManager* FJSONParamsEditorTabManager::EditorTabManager = nullptr;

FJSONParamsEditorTabManager* FJSONParamsEditorTabManager::Get()
{
	if (EditorTabManager == nullptr)
	{
		EditorTabManager = new FJSONParamsEditorTabManager();
		EditorTabManager->CommandListInit();
		FParamsRegistry::Get().OnParamsReloaded.AddRaw(
			EditorTabManager,
			&FJSONParamsEditorTabManager::OnParamsReloaded);
		FParamsRegistry::Get().OnParamsInitStarted.AddRaw(
			EditorTabManager,
			&FJSONParamsEditorTabManager::OnParamsInitStarted);
	}

	return EditorTabManager;
}

void FJSONParamsEditorTabManager::OpenJSONParamsEditorTab(UScriptStruct* ParamType, const FName& ParamName)
{
	const FString ParamTabId = CreateParamTabId(ParamType, ParamName);

	// Check already opened tab
	if (const TSharedRef<SDockTab>* ParamEditorTab = ParamsEditorTabs.Find(ParamTabId))
	{
		FGlobalTabmanager::Get()->DrawAttention(*ParamEditorTab);
		return;
	}

	// Get param
	const FParamRegistryDataPtr ParamPtr = FParamsRegistry::GetParam(ParamType, ParamName);
	const FParamRegistryMeta* ParamMeta = ParamPtr ? &ParamPtr->Meta : nullptr;
	if (!ParamPtr || !ParamMeta)
		return;

	//  Create editor widget
	const TSharedRef<SEditJSONParamWidget> ParamEditorWidget = SNew(SEditJSONParamWidget)
		.ParamData(ParamPtr->Data.GetData())
		.ParamInfo({ParamType, ParamName})
		.CommandList(CommandList)
		.OnParamChanged_Raw(this, &FJSONParamsEditorTabManager::OnTabParamChanged);
	ParamEditorWidget->SetEnabled(ParamMeta->bIsSavable);

	//Create and register new tab
	const TSharedPtr<FTabManager::FSearchPreference> SearchPreference =
		MakeShareable(new FTabManager::FLiveTabSearch());
	const TSharedPtr<SDockTab> NewMajorTab = SNew(SDockTab)
		.Label(FText::FromString(ParamName.ToString()))
		.LabelSuffix(CreateParamTabSuffix(ParamMeta->bIsChanged))
		.TabRole(ETabRole::MajorTab)
		.ToolTipText(FText::FromString(ParamName.ToString() + " (" + ParamType->GetName() + ")"))
		.Icon(FSlateIcon(FJSONParamsStyle::GetStyleSetName(), "JSONParams.DefaultIcon16").GetIcon())
		.OnTabClosed_Lambda([this, ParamTabId](TSharedRef<SDockTab>) { ParamsEditorTabs.Remove(ParamTabId); });
	NewMajorTab->SetContent(ParamEditorWidget);

	FGlobalTabmanager::Get()->InsertNewDocumentTab(TEXT("DockedToolkit"), *SearchPreference, NewMajorTab.ToSharedRef());
	ParamsEditorTabs.Add(ParamTabId, NewMajorTab.ToSharedRef());
}

TSharedPtr<SDockTab> FJSONParamsEditorTabManager::GetActiveTab()
{
	for (TTuple<FString, TSharedRef<SDockTab>> TabWithKey : ParamsEditorTabs)
	{
		SDockTab& Tab = TabWithKey.Value.Get();
		if (Tab.IsForeground() && Tab.GetParentWindow()->IsActive())
		{
			return TabWithKey.Value;
		}
	}

	return nullptr;
}

FString FJSONParamsEditorTabManager::CreateParamTabId(const UScriptStruct* ParamType, const FName& ParamName)
{
	return ParamType->GetName() + "_" + ParamName.ToString();
}

FText FJSONParamsEditorTabManager::CreateParamTabSuffix(bool bIsParamChanged)
{
	return bIsParamChanged ? FText::FromString(" *") : FText::FromString("");
}

void FJSONParamsEditorTabManager::OnParamsReloaded()
{
	for (TTuple<FString, TSharedRef<SDockTab>> OpenedParamTabWithKey : ParamsEditorTabs)
	{
		SDockTab& OpenedParamTab = OpenedParamTabWithKey.Value.Get();
		SEditJSONParamWidget& EditorWidget =
			StaticCastSharedRef<SEditJSONParamWidget>(OpenedParamTab.GetContent()).Get();
		const FParamRegistryInfo& OpenedParamInfo = EditorWidget.GetEditedParamInfo();

		const FParamRegistryDataPtr OpenedParamPtr =
			FParamsRegistry::GetParam(OpenedParamInfo.Type, OpenedParamInfo.Name);
		if (!OpenedParamPtr)
		{
			EditorWidget.ShowPlaceholderForLostData();
			continue;
		}

		const FParamRegistryMeta* OpenedParamMeta = &OpenedParamPtr->Meta;
		bool bIsParamChanged = OpenedParamMeta != nullptr ? OpenedParamMeta->bIsChanged : false;
		bool bIsParamSavable = OpenedParamMeta != nullptr ? OpenedParamMeta->bIsSavable : false;
		OpenedParamTab.SetTabLabelSuffix(CreateParamTabSuffix(bIsParamChanged));
		EditorWidget.SetEnabled(bIsParamSavable);
		EditorWidget.SetParamData(OpenedParamPtr->Data.GetData());
	}
}

void FJSONParamsEditorTabManager::OnParamsInitStarted()
{
	for (TTuple<FString, TSharedRef<SDockTab>> OpenedParamTabWithKey : ParamsEditorTabs)
	{
		SDockTab& OpenedParamTab = OpenedParamTabWithKey.Value.Get();
		SEditJSONParamWidget& EditorWidget =
			StaticCastSharedRef<SEditJSONParamWidget>(OpenedParamTab.GetContent()).Get();
		EditorWidget.ShowPlaceholderForInit();
	}
}

void FJSONParamsEditorTabManager::OnTabParamChanged(
	const FPropertyChangedEvent& InEvent,
	const FParamRegistryInfo& ParamInfo)
{
	if (const TSharedRef<SDockTab>* ChangedParamTabPtr =
			ParamsEditorTabs.Find(CreateParamTabId(ParamInfo.Type, ParamInfo.Name)))
	{
		SDockTab& ChangedParamTab = ChangedParamTabPtr->Get();
		FParamsRegistry::ModifyParamChangedState(ParamInfo.Type, ParamInfo.Name);

		SEditJSONParamWidget& EditorWidget =
			StaticCastSharedRef<SEditJSONParamWidget>(ChangedParamTab.GetContent()).Get();
		FParamRegistryDataPtr Param = FParamsRegistry::GetParam(ParamInfo.Type, ParamInfo.Name);
		EditorWidget.GetParamData(Param->Data);

		ChangedParamTab.SetTabLabelSuffix(CreateParamTabSuffix(true));
	}
}

void FJSONParamsEditorTabManager::CommandListInit()
{
	const FJSONParamsCommands& Commands = FJSONParamsCommands::Get();
	CommandList = MakeShareable(new FUICommandList);
	CommandList->MapAction(
		Commands.SaveOpenedParam,
		FExecuteAction::CreateRaw(this, &FJSONParamsEditorTabManager::SaveOpenedParam),
		FCanExecuteAction());
	CommandList->MapAction(
		Commands.RevertOpenedParam,
		FExecuteAction::CreateRaw(this, &FJSONParamsEditorTabManager::RevertOpenedParam),
		FCanExecuteAction());
	CommandList->MapAction(
		Commands.BrowseToOpenedParam,
		FExecuteAction::CreateRaw(this, &FJSONParamsEditorTabManager::BrowseToOpenedParam),
		FCanExecuteAction());
}

void FJSONParamsEditorTabManager::SaveOpenedParam()
{
	const TSharedPtr<SDockTab>& ActiveTab = GetActiveTab();
	if (ActiveTab.IsValid())
	{
		const FParamRegistryInfo& ParamInfo =
			StaticCastSharedRef<SEditJSONParamWidget>(ActiveTab->GetContent()).Get().GetEditedParamInfo();
		if (FParamsRegistry::SaveParamOnDisk(ParamInfo.Type, ParamInfo.Name))
		{
			ActiveTab->SetTabLabelSuffix(CreateParamTabSuffix(false));
		}
	}
}

void FJSONParamsEditorTabManager::RevertOpenedParam()
{
	const TSharedPtr<SDockTab>& ActiveTab = GetActiveTab();

	if (ActiveTab.IsValid())
	{
		const FParamRegistryInfo& ParamInfo =
			StaticCastSharedRef<SEditJSONParamWidget>(ActiveTab->GetContent()).Get().GetEditedParamInfo();

		FParamRegistryDataPtr ParamPtr = FParamsRegistry::GetParam(ParamInfo.Type, ParamInfo.Name);
		const FParamRegistryMeta* ParamMeta = ParamPtr ? &ParamPtr->Meta : nullptr;
		if (FParamsRegistry::LoadParamFromDisk(
				ParamInfo.Type,
				ParamInfo.Name,
				ParamMeta->FilePath,
				ParamMeta->ParamIndex))
		{
			ActiveTab->SetTabLabelSuffix(CreateParamTabSuffix(false));
		}
	}
}

void FJSONParamsEditorTabManager::BrowseToOpenedParam()
{
	const TSharedPtr<SDockTab>& ActiveTab = GetActiveTab();
	if (ActiveTab.IsValid())
	{
		const FParamRegistryInfo& ParamInfo =
			StaticCastSharedRef<SEditJSONParamWidget>(ActiveTab->GetContent()).Get().GetEditedParamInfo();
		FJSONParamsEditorModule::OnBrowseToParamDelegate.ExecuteIfBound(ParamInfo.Type, ParamInfo.Name);
	}
}

void UParamsEditorTabManagerLib::OpenJSONParamsEditorTab(UObject* Type, const FName& ParamName)
{
	UScriptStruct* RealType = Cast<UScriptStruct>(Type);
	if (!RealType)
		return;

	FJSONParamsEditorTabManager::Get()->OpenJSONParamsEditorTab(RealType, ParamName);
}
