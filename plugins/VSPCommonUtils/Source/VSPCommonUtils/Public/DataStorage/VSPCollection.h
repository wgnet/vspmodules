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

#include "GameplayTagContainer.h"

#include "CoreMinimal.h"

#include "VSPCollection.generated.h"

class UVSPEntity;

UCLASS(BlueprintType)
class VSPCOMMONUTILS_API UVSPCollection : public UObject
{
	GENERATED_BODY()

public:
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnAdded, UVSPEntity*, Entity);

	UPROPERTY(BlueprintAssignable)
	FOnAdded OnAdded;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRemoved, UVSPEntity*, Entity);

	UPROPERTY(BlueprintAssignable)
	FOnRemoved OnRemoved;

	DECLARE_DYNAMIC_MULTICAST_DELEGATE(FCustomUpdate);

	UPROPERTY(BlueprintAssignable)
	FCustomUpdate CustomUpdate;

	UFUNCTION(BlueprintCallable)
	TArray<UVSPEntity*> GetEntities();

	UFUNCTION(BlueprintCallable)
	int32 Num() const;

	void AddSubCollection(const FGameplayTag& Tag, UVSPCollection* SubCollection);
	void RemoveSubCollection(const FGameplayTag& Tag);

	UFUNCTION(BlueprintCallable)
	UVSPCollection* GetSubCollection(const FGameplayTag& Tag);

	virtual void Update();

protected:
	virtual void Add(UVSPEntity* Entity);
	virtual void Remove(UVSPEntity* Entity);

	UPROPERTY()
	TArray<UVSPEntity*> Entities;

	UPROPERTY()
	TMap<FGameplayTag, UVSPCollection*> SubCollections;

	friend class UVSPDataStorage;
};
