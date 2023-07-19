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
#include "Engine/AssetUserData.h"
#include "GameplayTagContainer.h"
#include "AssetRegisterData.generated.h"

UENUM(BlueprintType)
enum class EAssetType : uint8
{
	Undefined,
	StaticMesh,
	Material,
	MaterialInstance,
	Texture
};

UCLASS(BlueprintType)
class ASSETREGISTERRUNTIME_API UAssetRegisterMark : public UAssetUserData
{
	GENERATED_BODY()

public:
	static UAssetRegisterMark* GetAssetRegisterMark(UObject* InObject);

	UFUNCTION(BlueprintPure)
	static FName GetAssetRegisterMarkTag(UObject* InObject);

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	FGameplayTag PrimaryTag;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	TArray<FGameplayTag> AdditionalTags;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
	bool bLegacyAsset;

	UFUNCTION(BlueprintCallable)
	UObject* GetOwner() const;

	UFUNCTION(BlueprintCallable)
	EAssetType GetType() const;
};
