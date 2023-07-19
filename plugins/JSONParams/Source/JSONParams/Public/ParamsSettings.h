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

#include "ParamsSettings.generated.h"


class UParamsEditorWidget;


UCLASS(Config = JSONParams, DefaultConfig)
class JSONPARAMS_API UParamsSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	// Sets default values for this actor's properties
	static const UParamsSettings* Get();

	/** Gets the settings container name for the settings, either Project or Editor */
	virtual FName GetContainerName() const override;
	/** Gets the category for the settings, some high level grouping like, Editor, Engine, Game...etc. */
	virtual FName GetCategoryName() const override;
	/** The unique name for your section of settings, uses the class's FName. */
	virtual FName GetSectionName() const override;

#if WITH_EDITOR
	/** Gets the section text, uses the classes DisplayName by default. */
	virtual FText GetSectionText() const override;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	bool GetEnableContentBrowserIntegration() const;
	static void SetEnableContentBrowserIntegration(bool bIsEnabled);

	FName GetContentBrowserVirtualRoot() const;
#endif

	UFUNCTION(Category = "Params Server")
	FString GetParamsServerURL() const;

	UFUNCTION(Category = "Params Paths")
	TArray<FString> GetParamsRootPaths(bool IncludeServerPath = false) const;

	UPROPERTY(EditAnywhere, Config, Category = "Base Settings")
	FString ParamFileNameWildcard = "*.uparam";

	UPROPERTY(EditAnywhere, Config, Category = "File Params")
	TArray<FDirectoryPath> ParamFilesRootPaths { {} };

	UPROPERTY(EditAnywhere, Config, Category = "Params Server")
	bool EnableParamsServer = false;

	// Value < 0 means we retry indefinitely on this type of error.
	UPROPERTY(EditAnywhere, Config, Category = "Params Server", AdvancedDisplay)
	int32 MaxConnectionErrors = 5;

	// Value < 0 means we retry indefinitely on this type of error.
	UPROPERTY(EditAnywhere, Config, Category = "Params Server", AdvancedDisplay)
	int32 MaxErrors = 5;

private:
	static bool IsEnvVariable(const FString& Variable);
	FString GetEnvVariable(const FString& Variable) const;

	UPROPERTY(EditAnywhere, Config, Category = "Params Server")
	FString ParamsServerURL = "";

#if WITH_EDITORONLY_DATA

	UPROPERTY(Config)
	bool EnableContentBrowserIntegration = false;

	UPROPERTY(Config)
	FName VirtualRoot = "/UParams";

#endif
};
