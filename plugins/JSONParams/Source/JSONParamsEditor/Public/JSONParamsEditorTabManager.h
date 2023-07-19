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

#include "JSONParamsEditorTabManager.generated.h"


struct FParamRegistryInfo;

class JSONPARAMSEDITOR_API FJSONParamsEditorTabManager
{
public:
	static FJSONParamsEditorTabManager* Get();
	void OpenJSONParamsEditorTab(UScriptStruct* ParamType, const FName& ParamName);

private:
	static FJSONParamsEditorTabManager* EditorTabManager;
	TMap<FString, TSharedRef<SDockTab>> ParamsEditorTabs;
	TSharedPtr<class FUICommandList> CommandList;

	static FString CreateParamTabId(const UScriptStruct* ParamType, const FName& ParamName);
	static FText CreateParamTabSuffix(bool bIsParamChanged);
	TSharedPtr<SDockTab> GetActiveTab();
	void OnParamsReloaded();
	void OnParamsInitStarted();
	void OnTabParamChanged(const FPropertyChangedEvent& InEvent, const FParamRegistryInfo& ParamInfo);
	void CommandListInit();
	void SaveOpenedParam();
	void RevertOpenedParam();
	void BrowseToOpenedParam();
};

UCLASS()
class JSONPARAMSEDITOR_API UParamsEditorTabManagerLib : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable)
	static void OpenJSONParamsEditorTab(UObject* Type, const FName& ParamName);
};
