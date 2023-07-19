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
#include "ParamCustomization/ParamRegistryInfoCustomization.h"

#include "Async/Async.h"
#include "DetailWidgetRow.h"
#include "EditorWidgets/Public/SSearchableComboBox.h"
#include "IDetailChildrenBuilder.h"
#include "JSONParamsEditorModule.h"
#include "JSONParamsEditorTabManager.h"
#include "ParamsRegistry.h"
#include "SlateOptMacros.h"
#include "UObject/UnrealTypePrivate.h"

TSharedRef<IPropertyTypeCustomization> FParamRegistryInfoCustomization::MakeInstance()
{
	return MakeShareable(new FParamRegistryInfoCustomization());
}

BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION

void FParamRegistryInfoCustomization::CustomizeHeader(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	FDetailWidgetRow& HeaderRow,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	ThisPropertyHandle = StructPropertyHandle;
	TypePropertyHandle = ThisPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FParamRegistryInfo, Type));
	NamePropertyHandle = ThisPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FParamRegistryInfo, Name));

	const FText ThisName = FText::FromString(ThisPropertyHandle->GetProperty()->GetName());

	const TSharedRef<SWidget> NoneWidget = SNew(STextBlock).Text(FText::FromString("None"));

	ParamRegistryInfo = nullptr;
	void* ParamRegistryInfoData;
	FString SelectionName = "None";
	if (ThisPropertyHandle->GetValueData(ParamRegistryInfoData) == FPropertyAccess::Success)
	{
		ParamRegistryInfo = static_cast<FParamRegistryInfo*>(ParamRegistryInfoData);
		SelectionName = ParamRegistryInfo->Name.ToString();
		UpdateValueAndDropDown(ParamRegistryInfo->Type);
	}

	TSharedRef<SWidget> TypeWidget = NoneWidget;
	if (TypePropertyHandle)
	{
		TypeWidget = SNew(SHorizontalBox)
		             + SHorizontalBox::Slot()
		             .FillWidth(1)
		             [
			             TypePropertyHandle->CreatePropertyValueWidget(false)
		             ]
		             + SHorizontalBox::Slot()
		             .AutoWidth()
		             [
			             TypePropertyHandle->CreateDefaultPropertyButtonWidgets()
		             ];
	}
	else
	{
		if (ParamRegistryInfo && ParamRegistryInfo->Type)
			TypeWidget = SNew(STextBlock)
			             .Text(FText::FromString(ParamRegistryInfo->Type->GetName()));
	}

	HeaderRow.NameContent()
		.VAlign(VAlign_Fill)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			  .FillWidth(1)
			  .VAlign(VAlign_Center)
			[
				SNew(STextBlock)
				.Text(ThisName)
			]
			+ SHorizontalBox::Slot()
			.AutoWidth()
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				  .FillHeight(1)
				  .VAlign(VAlign_Center)
				  .Padding(0.f, 0.f, 0.f, 2.f)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Type:"))
				]
				+ SVerticalBox::Slot()
				  .FillHeight(1)
				  .VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString("Name:"))
				]
			]
		]
		.ValueContent()
		.MinDesiredWidth(512.f)
		[
			SNew(SVerticalBox)
			+ SVerticalBox::Slot()
			  .FillHeight(1)
			  .Padding(0.f, 0.f, 0.f, 2.f)
			[
				SNew(SBox)
				.MinDesiredHeight(24.f)
				.VAlign(VAlign_Center)
				[
					TypeWidget
				]
			]
			+ SVerticalBox::Slot()
			  .FillHeight(1)
			  .VAlign(VAlign_Center)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(1)
				[
					SAssignNew(NameOptionComboBox, SSearchableComboBox)
					.OptionsSource(&NameOptionList)
					.OnGenerateWidget(this, &FParamRegistryInfoCustomization::MakeNameOptionComboWidget)
					.OnSelectionChanged(this, &FParamRegistryInfoCustomization::OnNameOptionSelectionChanged)
					.InitiallySelectedItem(MakeShared<FString>(MoveTemp(SelectionName)))
					.ContentPadding(2)
					.Content()
					[
						SNew(STextBlock)
						.Text(this, &FParamRegistryInfoCustomization::GetNameOptionComboBoxContent)
					]
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .HAlign(HAlign_Center)
				  .VAlign(VAlign_Center)
				  .Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
					.OnClicked(this, &FParamRegistryInfoCustomization::OnEditClicked)
					.ToolTipText(this, &FParamRegistryInfoCustomization::GetEditParamToolTipText)
					.IsEnabled(this, &FParamRegistryInfoCustomization::IsSelectedParamExist)
					.ContentPadding(4.0f)
					.ForegroundColor(FSlateColor::UseForeground())
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Edit"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
				]
				+ SHorizontalBox::Slot()
				  .AutoWidth()
				  .HAlign(HAlign_Center)
				  .VAlign(VAlign_Center)
				  .Padding(2.0f, 0.0f)
				[
					SNew(SButton)
					.ButtonStyle(FEditorStyle::Get(), "HoverHintOnly")
					.OnClicked(this, &FParamRegistryInfoCustomization::OnBrowseClicked)
					.ToolTipText(this, &FParamRegistryInfoCustomization::GetBrowseParamToolTipText)
					.IsEnabled(this, &FParamRegistryInfoCustomization::IsSelectedParamExist)
					.ContentPadding(4.0f)
					.ForegroundColor(FSlateColor::UseForeground())
					[
						SNew(SImage)
						.Image(FEditorStyle::GetBrush("PropertyWindow.Button_Browse"))
						.ColorAndOpacity(FSlateColor::UseForeground())
					]
				]
				+ SHorizontalBox::Slot()
				.AutoWidth()
				[
					NamePropertyHandle->CreateDefaultPropertyButtonWidgets()
				]
			]
		];

	if (TypePropertyHandle)
	{
		// OnTypeChanged(TypePropertyHandle);
		TypePropertyHandle->SetOnPropertyValueChanged(
			FSimpleDelegate::CreateSP(this, &FParamRegistryInfoCustomization::OnTypeChanged, TypePropertyHandle));
	}
}

void FParamRegistryInfoCustomization::CustomizeChildren(
	TSharedRef<IPropertyHandle> StructPropertyHandle,
	IDetailChildrenBuilder& StructBuilder,
	IPropertyTypeCustomizationUtils& StructCustomizationUtils)
{
	return;
}

END_SLATE_FUNCTION_BUILD_OPTIMIZATION


void FParamRegistryInfoCustomization::OnTypeChanged(TSharedPtr<IPropertyHandle> InTypePropertyHandle)
{
	if (!InTypePropertyHandle)
		return;

	if (ParamRegistryInfo->Type == CachedType)
	{
		return;
	}

	CachedType = ParamRegistryInfo->Type;
	NameOptionComboBox->SetSelectedItem(NoneOption);

	UObject* TypeObject = nullptr;
	InTypePropertyHandle->GetValue(TypeObject);
	const auto NewType = Cast<UScriptStruct>(TypeObject);
	UpdateValueAndDropDown(NewType);
}

FReply FParamRegistryInfoCustomization::OnEditClicked()
{
	FJSONParamsEditorTabManager::Get()->OpenJSONParamsEditorTab(ParamRegistryInfo->Type, ParamRegistryInfo->Name);

	return FReply::Handled();
}

FReply FParamRegistryInfoCustomization::OnBrowseClicked()
{
	FJSONParamsEditorModule::OnBrowseToParamDelegate.ExecuteIfBound(ParamRegistryInfo->Type, ParamRegistryInfo->Name);
	return FReply::Handled();
}

void FParamRegistryInfoCustomization::UpdateValueAndDropDown(UScriptStruct* NewType)
{
	TArray<FName> Names = FParamsRegistry::Get().GetRegisteredNames(NewType);
	NameOptionList.Reset(Names.Num());

	if (!ParamRegistryInfo)
	{
		return;
	}

	if (!NoneOption)
		NoneOption = MakeShareable(new FString(FName().ToString()));
	NameOptionList.Add(NoneOption);
	for (const FName& Name : Names)
	{
		NameOptionList.Add(MakeShareable(new FString(Name.ToString())));
	}

	if (NameOptionComboBox)
		NameOptionComboBox->RefreshOptions();
}

TSharedRef<SWidget> FParamRegistryInfoCustomization::MakeNameOptionComboWidget(TSharedPtr<FString> InItem)
{
	return SNew(STextBlock).Text(FText::FromString(*InItem));
}

void FParamRegistryInfoCustomization::OnNameOptionSelectionChanged(
	TSharedPtr<FString> NewSelection,
	ESelectInfo::Type SelectInfo)
{
	if (SelectInfo != ESelectInfo::Direct)
	{
		if (ParamRegistryInfo)
		{
			NamePropertyHandle->SetValue(**NewSelection);
		}
	}
}

FText FParamRegistryInfoCustomization::GetNameOptionComboBoxContent() const
{
	if (ParamRegistryInfo)
	{
		return FText::FromString(ParamRegistryInfo->Name.ToString());
	}
	return FText::FromString(FName().ToString());
}

FText FParamRegistryInfoCustomization::GetEditParamToolTipText() const
{
	if (!IsSelectedParamExist())
		return FText::FromString("Can't edit invalid property!");

	FString ParamName = ParamRegistryInfo->Name.ToString();
	FString ParamType = ParamRegistryInfo->Type->GetName();

	return FText::FromString(FString::Format(TEXT("Edit param {0} (type: {1})"), { ParamName, ParamType }));
}

FText FParamRegistryInfoCustomization::GetBrowseParamToolTipText() const
{
	if (!IsSelectedParamExist())
		return FText::FromString("Can't browse to invalid property!");

	FString ParamName = ParamRegistryInfo->Name.ToString();
	FString ParamType = ParamRegistryInfo->Type->GetName();

	return FText::FromString(
		FString::Format(TEXT("Browse to {0} (type: {1}) in content browser"), { ParamName, ParamType }));
}

bool FParamRegistryInfoCustomization::IsSelectedParamExist() const
{
	return ParamRegistryInfo && ParamRegistryInfo->Type && !ParamRegistryInfo->Name.IsNone();
}
