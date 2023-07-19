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
#include "GameplayTagContainer.h"
#include "AssetRegisterSettings.generated.h"

USTRUCT(BlueprintType)
struct FAssetRegisterMarkTagsSettings
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, Category = "AssetRegisterMark", meta = (DisplayName = "Primary Tag"))
	FGameplayTag PrimaryTag;

	UPROPERTY(EditDefaultsOnly, Category = "AssetRegisterMark", meta = (DisplayName = "Additional Tags (Optional)"))
	TArray<FGameplayTag> AdditionalTags;
};

UCLASS(Config = VSPEditor, DefaultConfig, meta = (DisplayName = "Asset Register"))
class ASSETREGISTEREDITOR_API UAssetRegisterSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	static UAssetRegisterSettings* Get();

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;

	UPROPERTY(EditDefaultsOnly, Config)
	UClass* PrefabAssetClass;

	UPROPERTY(EditDefaultsOnly, Config)
	UClass* PrefabActorClass;

	UPROPERTY(EditDefaultsOnly, Config)
	TSet<UClass*> AdditionalClassesForIgnore;

	UPROPERTY(EditDefaultsOnly, Config)
	FSoftObjectPath EditorWidgetUtility;

	UPROPERTY(EditDefaultsOnly, Config)
	FSoftObjectPath LODsDataAsset;
};
