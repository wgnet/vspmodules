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

#include "Widgets/SCompoundWidget.h"

DECLARE_DELEGATE(FOnPickedStructChanged);

class JSONPARAMSEDITOR_API SStructPicker : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SStructPicker)
	{
	}

	SLATE_EVENT(FOnPickedStructChanged, OnPickedStructChanged)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	TWeakObjectPtr<UScriptStruct> GetSelectedStruct() const;

private:
	TWeakObjectPtr<UScriptStruct> PickedStruct = nullptr;
	TSharedPtr<SComboButton> ComboPicker;
	TSharedPtr<STextBlock> ComboText;
	FOnPickedStructChanged OnPickedStructChanged;

	TSharedRef<SWidget> GenerateStructPicker();
	void OnStructPicked(const UScriptStruct* InStruct);
};

class JSONPARAMSEDITOR_API SCreateJSONParamWidget : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SCreateJSONParamWidget)
	{
	}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	FReply OnCreateButtonClicked() const;

private:
	TSharedPtr<SStructPicker> StructPicker;
	TSharedPtr<SEditableTextBox> NameText;

	void OnPickedStructChanged() const;
};
