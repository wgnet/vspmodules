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

#include "ParamsRegistry.h"
#include "UObject/StrongObjectPtr.h"


class SEditJSONParamWidget : public SCompoundWidget
{
public:
	DECLARE_DELEGATE_TwoParams(FOnParamChanged, const FPropertyChangedEvent&, const FParamRegistryInfo&);

	SLATE_BEGIN_ARGS(SEditJSONParamWidget)
	{
	}

	SLATE_EVENT(FOnParamChanged, OnParamChanged)
	SLATE_ARGUMENT(FParamRegistryInfo, ParamInfo)
	SLATE_ARGUMENT(const void*, ParamData)
	SLATE_ARGUMENT(TSharedPtr<class FUICommandList>, CommandList)

	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);

	const FParamRegistryInfo& GetEditedParamInfo() const;

	void SetParamData(const void* InData);
	void GetParamData(TArray<uint8>& OutData);

	void ShowPlaceholderForInit() const;
	void ShowPlaceholderForLostData() const;

protected:
	void ConstructToolbar();
	void ConstructBlueprint();
	void ConstructDetailsView();
	void ConstructWidgetStructure();

	virtual FReply OnKeyDown(const FGeometry& MyGeometry, const FKeyEvent& InKeyEvent) override;

private:
	FParamRegistryInfo ParamInfo;

	FOnParamChanged OnParamChanged;
	TSharedPtr<FUICommandList> CommandList;
	TSharedPtr<SWidget> ToolbarWidget;
	TSharedPtr<STextBlock> PlaceholderText;
	TSharedPtr<IDetailsView> DetailsView;

	TStrongObjectPtr<UBlueprint> Blueprint;
	TStrongObjectPtr<UObject> BlueprintObject;

	FStructProperty* FindParamProperty();
	void ShowPlaceholder() const;
};
