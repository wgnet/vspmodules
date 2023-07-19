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


struct FParamRegistryInfo;
class SSearchableComboBox;

class FParamRegistryInfoCustomization : public IPropertyTypeCustomization
{
public:
	/**
	 * It is just a convenient helpers which will be used
	 * to register our customization. When the propertyEditor module
	 * find our FMyStruct property, it will use this static method
	 * to instanciate our customization object.
	 */
	static TSharedRef<IPropertyTypeCustomization> MakeInstance();

	// BEGIN IPropertyTypeCustomization interface
	virtual void CustomizeHeader(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		class FDetailWidgetRow& HeaderRow,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	virtual void CustomizeChildren(
		TSharedRef<IPropertyHandle> StructPropertyHandle,
		class IDetailChildrenBuilder& StructBuilder,
		IPropertyTypeCustomizationUtils& StructCustomizationUtils) override;
	// END IPropertyTypeCustomization interface

protected:
	void OnTypeChanged(TSharedPtr<IPropertyHandle> TypePropertyHandle);


	void UpdateValueAndDropDown(UScriptStruct* NewType);

	/** Widget used to display the list of variable option list*/
	TSharedPtr<SSearchableComboBox> NameOptionComboBox;
	TArray<TSharedPtr<FString>> NameOptionList;
	TSharedPtr<FString> NoneOption;

	TSharedRef<SWidget> MakeNameOptionComboWidget(TSharedPtr<FString> InItem);
	void OnNameOptionSelectionChanged(TSharedPtr<FString> NewSelection, ESelectInfo::Type SelectInfo);
	FText GetNameOptionComboBoxContent() const;

	FReply OnEditClicked();
	FReply OnBrowseClicked();
	FText GetEditParamToolTipText() const;
	FText GetBrowseParamToolTipText() const;
	bool IsSelectedParamExist() const;

	TSharedPtr<IPropertyHandle> ThisPropertyHandle;
	TSharedPtr<IPropertyHandle> TypePropertyHandle;
	TSharedPtr<IPropertyHandle> NamePropertyHandle;
	FParamRegistryInfo* ParamRegistryInfo = nullptr;
	UScriptStruct* CachedType = nullptr;
};
