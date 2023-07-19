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

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"

#include "StringTablesCheckerSettings.generated.h"

UCLASS(Config = VSPEditor, meta = (DisplayName = "String tables checker settings"))
class STRINGTABLESCHECKER_API UStringTablesCheckerSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintPure, meta = (DisplayName = "GetStringTablesCheckerSettings"))
	static UStringTablesCheckerSettings* Get();

	UFUNCTION(BlueprintPure)
	FString GetConfluenceURL() const;

	TArray<FSoftClassPath> GetSupportedAssetsClasses() const;
	TArray<FString> GetAssetsPaths() const;
	TArray<FString> GetStringTablesPaths() const;
	TArray<FString> GetStringTablesExclusions() const;

private:
	UPROPERTY(EditAnywhere, Config, Category = General, meta = (DisplayName = "Supported assets classes for text search", AllowAbstract = "true"))
	TArray<FSoftClassPath> SupportedAssetsClasses {};

	UPROPERTY(EditAnywhere, Config, Category = General, meta = (DisplayName = "Assets paths for text search"))
	TArray<FString> AssetsPaths {};

	UPROPERTY(EditAnywhere, Config, Category = General, meta = (DisplayName = "String tables paths for check"))
	TArray<FString> StringTablesPaths {};

	UPROPERTY(EditAnywhere, Config, Category = General, meta = (DisplayName = "String tables names exclusions"))
	TArray<FString> StringTablesExclusions {};

	// Confluence page URL (for widget info block)
	UPROPERTY(EditAnywhere, Config, Category = Widget)
	FString ConfluenceURL = "None";

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
};
