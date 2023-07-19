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
#include "CreateJSONParamWidget.h"

#include "DesktopPlatformModule.h"
#include "EditorDirectories.h"
#include "Engine/UserDefinedStruct.h"
#include "ParamsRegistry.h"
#include "ParamsSettings.h"
#include "SlateOptMacros.h"
#include "StructViewerFilter.h"
#include "StructViewerModule.h"


DEFINE_LOG_CATEGORY_STATIC(LogParamsWidget, Log, All)

class FAnyCppStructFilter : public IStructViewerFilter
{
public:
	virtual bool IsStructAllowed(
		const FStructViewerInitializationOptions& InInitOptions,
		const UScriptStruct* InStruct,
		TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override
	{
		for (const UScriptStruct* RestrictedStruct : RestrictedStructs)
		{
			if (InStruct->IsChildOf(RestrictedStruct))
				return false;
		}

		return true;
	}

	virtual bool IsUnloadedStructAllowed(
		const FStructViewerInitializationOptions& InInitOptions,
		const FName InStructPath,
		TSharedRef<FStructViewerFilterFuncs> InFilterFuncs) override
	{
		return true;
	}

private:
	const TSet<const UScriptStruct*> RestrictedStructs = {
		FParamRegistryInfo::StaticStruct(),
		FParamRegistryData::StaticStruct(),
		FParamRegistryMeta::StaticStruct()
	};
};

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SStructPicker::Construct(const FArguments& InArgs)
{
	OnPickedStructChanged = InArgs._OnPickedStructChanged;

	ChildSlot
	[
		SAssignNew(ComboPicker, SComboButton)
		.ContentPadding(FMargin(2, 2, 2, 1))
		.MenuPlacement(MenuPlacement_BelowAnchor)
		.ButtonContent()
		[
			SAssignNew(ComboText, STextBlock)
			.Text(PickedStruct.IsValid() ? PickedStruct->GetDisplayNameText() : FText::FromString("None"))
		]
		.OnGetMenuContent(this, &SStructPicker::GenerateStructPicker)
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

TWeakObjectPtr<UScriptStruct> SStructPicker::GetSelectedStruct() const
{
	return PickedStruct;
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

TSharedRef<SWidget> SStructPicker::GenerateStructPicker()
{
	FStructViewerModule& StructViewerModule = FModuleManager::LoadModuleChecked<FStructViewerModule>("StructViewer");

	TSharedRef<FAnyCppStructFilter> StructFilter = MakeShared<FAnyCppStructFilter>();

	// Fill in options
	FStructViewerInitializationOptions Options;
	Options.Mode = EStructViewerMode::StructPicker;
	Options.StructFilter = StructFilter;
	Options.ExtraPickerCommonStructs = {};
	Options.bShowUnloadedStructs = true;
	Options.bShowNoneOption = true;
	Options.NameTypeToDisplay = EStructViewerNameTypeToDisplay::DisplayName;
	Options.DisplayMode = EStructViewerDisplayMode::TreeView;
	Options.bExpandRootNodes = true;
	Options.bAllowViewOptions = false;

	return
		SNew(SBox)
		.WidthOverride(330)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.0f)
			.MaxHeight(500)
			[
				SNew(SBorder)
				.Padding(4)
				.BorderImage(FEditorStyle::GetBrush("Menu.Background"))
				[
					StructViewerModule.CreateStructViewer(
						Options,
						FOnStructPicked::CreateSP(this, &SStructPicker::OnStructPicked))
				]
			]
		];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SStructPicker::OnStructPicked(const UScriptStruct* InStruct)
{
	PickedStruct = MakeWeakObjectPtr(const_cast<UScriptStruct*>(InStruct));

	ComboPicker->SetIsOpen(false);
	ComboText->SetText(PickedStruct.IsValid() ? PickedStruct->GetDisplayNameText() : FText::FromString("None"));

	OnPickedStructChanged.ExecuteIfBound();
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void SCreateJSONParamWidget::Construct(const FArguments& InArgs)
{
	ChildSlot
	[
		SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.FillWidth(5.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(2)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Type:"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(6.f)
				[
					SAssignNew(StructPicker, SStructPicker)
					.OnPickedStructChanged(
						FOnPickedStructChanged::CreateSP(this, &SCreateJSONParamWidget::OnPickedStructChanged))
				]
			]
			+ SVerticalBox::Slot()
			.FillHeight(1.f)
			.Padding(3)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Name:"))
				]
				+ SHorizontalBox::Slot()
				.FillWidth(6.f)
				[
					SAssignNew(NameText, SEditableTextBox)
					.Text(FText::FromString("None"))
				]
			]
		]
		+ SHorizontalBox::Slot()
		.FillWidth(1.f)
		.Padding(2)
		[
			SNew(SButton)
			.OnClicked_Raw(this, &SCreateJSONParamWidget::OnCreateButtonClicked)
			.VAlign(VAlign_Center)
			.HAlign(HAlign_Center)
			.Content()
			[
				SNew(STextBlock)
				.Text(FText::FromString("Create"))
			]
		]
	];
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION

FReply SCreateJSONParamWidget::OnCreateButtonClicked() const
{
	if (!StructPicker->GetSelectedStruct().IsValid())
		return FReply::Handled();

	UScriptStruct* SelectedStruct = StructPicker->GetSelectedStruct().Get();

	// Prompt the user for the filenames
	TArray<FString> SaveFilenames;
	IDesktopPlatform* DesktopPlatform = FDesktopPlatformModule::Get();
	bool bFileSelected = false;
	int32 FilterIndex = -1;

	// Open file dialog
	if (DesktopPlatform)
	{
		const void* ParentWindowWindowHandle = FSlateApplication::Get().FindBestParentWindowHandleForDialogs(nullptr);

		bFileSelected = DesktopPlatform->SaveFileDialog(
			ParentWindowWindowHandle,
			"Create JSON param from " + SelectedStruct->GetDisplayNameText().ToString(),
			FEditorDirectories::Get().GetLastDirectory(ELastDirectory::GENERIC_EXPORT),
			SelectedStruct->GetName(),
			"JSON Param file |" + UParamsSettings::Get()->ParamFileNameWildcard,
			EFileDialogFlags::None,
			SaveFilenames);
	}

	if (bFileSelected && SaveFilenames.Num() > 0)
	{
		FEditorDirectories::Get().SetLastDirectory(ELastDirectory::GENERIC_EXPORT, FPaths::GetPath(SaveFilenames[0]));

		FString Name = NameText->GetText().ToString();
		if (Name.IsEmpty())
		{
			Name = SelectedStruct->GetName();
			NameText->SetText(FText::FromString(Name));
		}

		const bool Result = FParamsRegistry::CreateAndSaveParam(SelectedStruct, FName(Name), SaveFilenames[0]);

		UE_LOG(
			LogParamsWidget,
			Log,
			TEXT("JSON file creation [%s]: %s!"),
			SelectedStruct ? *SelectedStruct->GetName() : TEXT("NULL"),
			Result ? TEXT("succsess") : TEXT("failure"))
	}

	return FReply::Handled();
}

void SCreateJSONParamWidget::OnPickedStructChanged() const
{
	const bool bIsPickedStructValid = StructPicker->GetSelectedStruct().IsValid();
	const FString PickedStructName = bIsPickedStructValid ? StructPicker->GetSelectedStruct()->GetName() : TEXT("None");
	NameText->SetText(FText::FromString(PickedStructName));
}
