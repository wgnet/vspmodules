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
#include "EditJSONParamWidget.h"

#include "BlueprintCompilationManager.h"
#include "ISinglePropertyView.h"
#include "JSONParamsCommands.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "Kismet2/KismetEditorUtilities.h"

namespace SEditJSONParamWidget_Local
{
	const FName PropertyFName = "UparamProperty";
	const FName BP_BaseFName = "BP_UparamContainer";
	const FName BP_BaseObjectFName = "BP_ObjectName";
	const char* BP_TempObjectPrefix = "BP_TEMP_OBJECT_";
	const char* BP_TempPrefix = "BP_TEMP_";
}

void SEditJSONParamWidget::Construct(const FArguments& InArgs)
{
	ParamInfo = InArgs._ParamInfo;
	OnParamChanged = InArgs._OnParamChanged;
	CommandList = InArgs._CommandList;

	ConstructToolbar();

	PlaceholderText = SNew(STextBlock)
					  .Visibility(EVisibility::Collapsed)
					  .Font(FCoreStyle::GetDefaultFontStyle("Bold", 12));

	ConstructBlueprint();
	ConstructDetailsView();
	ConstructWidgetStructure();
	SetParamData(InArgs._ParamData);
}

const FParamRegistryInfo& SEditJSONParamWidget::GetEditedParamInfo() const
{
	return ParamInfo;
}

void SEditJSONParamWidget::GetParamData(TArray<uint8>& OutData)
{
	FStructProperty* Property = FindParamProperty();
	if (!Property)
		return;

	void* Source = Property->ContainerPtrToValuePtr<void>(BlueprintObject.Get());
	Property->CopySingleValue(OutData.GetData(), Source);
}

void SEditJSONParamWidget::SetParamData(const void* InData)
{
	FStructProperty* Property = FindParamProperty();
	if (!Property)
		return;

	void* Target = Property->ContainerPtrToValuePtr<void>(BlueprintObject.Get());
	Property->CopySingleValue(Target, InData);

	DetailsView->AsShared()->SetVisibility(EVisibility::Visible);
	PlaceholderText->SetVisibility(EVisibility::Collapsed);
	ToolbarWidget->SetEnabled(true);
	DetailsView->ForceRefresh();
}

void SEditJSONParamWidget::ShowPlaceholderForInit() const
{
	PlaceholderText->SetText(
		FText::FromString("Unavailable to show JSON param data: Params Registry is initializing. Please wait."));
	PlaceholderText->SetColorAndOpacity(FColor::Yellow);
	ShowPlaceholder();
}

void SEditJSONParamWidget::ShowPlaceholderForLostData() const
{
	const FString PlaceholderString = FString::Format(
		TEXT("Unavailable to show JSON param data: param {0} of type {1} is not exist."),
		{ ParamInfo.Name.ToString(), ParamInfo.Type->GetName() });
	PlaceholderText->SetText(FText::FromString(PlaceholderString));
	PlaceholderText->SetColorAndOpacity(FColor::Red);
	ShowPlaceholder();
}

FReply SEditJSONParamWidget::OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent)
{
	return CommandList->ProcessCommandBindings(InKeyEvent) ? FReply::Handled() : FReply::Unhandled();
}

FStructProperty* SEditJSONParamWidget::FindParamProperty()
{
	for (TFieldIterator<FStructProperty> It(Blueprint->GeneratedClass); It; ++It)
	{
		FStructProperty* Property = *It;
		FName VariableName = Property->GetFName();

		if (VariableName == SEditJSONParamWidget_Local::PropertyFName)
		{
			return Property;
		}
	}

	return nullptr;
}

void SEditJSONParamWidget::ShowPlaceholder() const
{
	DetailsView->AsShared()->SetVisibility(EVisibility::Collapsed);
	PlaceholderText->SetVisibility(EVisibility::Visible);
	ToolbarWidget->SetEnabled(false);
}

void SEditJSONParamWidget::ConstructToolbar()
{
	FSlateIcon SaveIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), TEXT("AssetEditor.SaveAsset"));
	FSlateIcon RevertIcon = FSlateIcon(FEditorStyle::GetStyleSetName(), TEXT("PhysicsAssetEditor.Undo"));
	FSlateIcon ExploreIcon =
		FSlateIcon(FEditorStyle::GetStyleSetName(), TEXT("SystemWideCommands.FindInContentBrowser"));

	FToolBarBuilder ToolBarBuilder(CommandList, FMultiBoxCustomization::None);
	ToolBarBuilder.AddToolBarButton(
		FJSONParamsCommands::Get().SaveOpenedParam,
		NAME_None,
		FText::FromString("Save"),
		FText::FromString("Save param to JSON"),
		SaveIcon);

	ToolBarBuilder.AddToolBarButton(
		FJSONParamsCommands::Get().RevertOpenedParam,
		NAME_None,
		FText::FromString("Revert"),
		FText::FromString("Reload param from JSON"),
		RevertIcon);

	ToolBarBuilder.AddToolBarButton(
		FJSONParamsCommands::Get().BrowseToOpenedParam,
		NAME_None,
		FText::FromString("Browse"),
		FText::FromString("Browses to the param and selects it in Content Browser"),
		ExploreIcon);

	ToolbarWidget = ToolBarBuilder.MakeWidget();
}

void SEditJSONParamWidget::ConstructWidgetStructure()
{
	TSharedPtr<SWidget> DetailsViewWidget = DetailsView;

	if (!DetailsViewWidget.IsValid())
	{
		DetailsViewWidget = SNew(STextBlock).Text(FText::FromString("DetailsViewWidget is invalid"));
	}

	ChildSlot
	[
		SNew(SBorder)
		.BorderImage(FEditorStyle::GetBrush("ToolBar.Background"))
		.HAlign(HAlign_Fill)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Center)
			.AutoHeight()
			[
				ToolbarWidget.ToSharedRef()
			]

			+ SVerticalBox::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Top)
			[
				PlaceholderText.ToSharedRef()
			]

			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.VAlign(VAlign_Fill)
			[
				SNew(SBox)
				[
					SNew(SVerticalBox)
					+ SVerticalBox::Slot()
					.HAlign(HAlign_Fill)
					.VAlign(VAlign_Fill)
					.FillHeight(1.f)
					[
						DetailsViewWidget.ToSharedRef()
					]
				]
			]
		]
	];
}

void SEditJSONParamWidget::ConstructDetailsView()
{
	FDetailsViewArgs DetailsViewArgs;
	{
		DetailsViewArgs.bAllowSearch = true;
		DetailsViewArgs.bHideSelectionTip = true;
		DetailsViewArgs.bLockable = false;
		DetailsViewArgs.bSearchInitialKeyFocus = true;
		DetailsViewArgs.bUpdatesFromSelection = false;
		DetailsViewArgs.bShowOptions = true;
		DetailsViewArgs.bShowModifiedPropertiesOption = true;
		DetailsViewArgs.bShowActorLabel = false;
		DetailsViewArgs.bForceHiddenPropertyVisibility = true;
		DetailsViewArgs.bShowScrollBar = false;
	}

	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::Get().LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);
	DetailsView->OnFinishedChangingProperties().AddLambda(
		[this](const FPropertyChangedEvent& InEvent)
		{
			OnParamChanged.ExecuteIfBound(InEvent, ParamInfo);
		});
	DetailsView->SetObject(BlueprintObject.Get());
}

void SEditJSONParamWidget::ConstructBlueprint()
{
	using namespace SEditJSONParamWidget_Local;

	FName NewBlueprintName = MakeUniqueObjectName(
		GetTransientPackage(),
		UBlueprint::StaticClass(),
		FName(*(BP_TempPrefix + ParamInfo.Name.ToString())));
	UBlueprint* NewBlueprint = FKismetEditorUtilities::CreateBlueprint(
		UObject::StaticClass(),
		GetTransientPackage(),
		NewBlueprintName,
		BPTYPE_Normal,
		UBlueprint::StaticClass(),
		UBlueprintGeneratedClass::StaticClass(),
		NAME_None);

	Blueprint.Reset(NewBlueprint);

	FEdGraphPinType PinType;

	PinType.ResetToDefaults();
	PinType.ContainerType = EPinContainerType::None;
	PinType.PinCategory = UEdGraphSchema_K2::PC_Struct;
	PinType.PinSubCategoryObject = ParamInfo.Type;

	FBlueprintEditorUtils::AddMemberVariable(Blueprint.Get(), SEditJSONParamWidget_Local::PropertyFName, PinType);
	const int32 VarIndex =
		FBlueprintEditorUtils::FindNewVariableIndex(Blueprint.Get(), SEditJSONParamWidget_Local::PropertyFName);
	if (VarIndex != INDEX_NONE)
	{
		Blueprint->NewVariables[VarIndex].Category = FText::FromString("Data");
		Blueprint->NewVariables[VarIndex].SetMetaData("ShowOnlyInnerProperties", "true");
	}

	FBlueprintCompilationManager::CompileSynchronously(
		FBPCompileRequest(Blueprint.Get(), EBlueprintCompileOptions::None, nullptr));

	FName ObjectFName = MakeUniqueObjectName(
		GetTransientPackage(),
		Blueprint->GeneratedClass,
		FName(*(BP_TempObjectPrefix + ParamInfo.Name.ToString())));
	UObject* Object =
		NewObject<UObject>(GetTransientPackage(), Blueprint->GeneratedClass, ObjectFName, RF_Transactional);
	BlueprintObject.Reset(Object);
}
