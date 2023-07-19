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

#include "Input/Reply.h"


class JSONPARAMSEDITOR_API FJSONParamsEditorModule : public IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

	/** Static delegate for browsing to param in Content browser from any editor widget */
	DECLARE_DELEGATE_TwoParams(FOnBrowseToParamDelegate, const UScriptStruct*, const FName&);
	inline static FOnBrowseToParamDelegate OnBrowseToParamDelegate;

private:
	void RegisterMenus();

	TSharedRef<class SDockTab> OnSpawnPluginTab(const class FSpawnTabArgs& SpawnTabArgs);

	TSharedPtr<SEditableText> TextBlock;

private:
	TSharedPtr<class FUICommandList> PluginCommands;

	void OnBeginPIE(bool bIsSimulating);
	FReply OnReInit();
};
