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

#include "VSPCheck.h"

#include "CoreMinimal.h"

#include "VSPEntity.generated.h"

class UVSPDataStorage;

UCLASS(BlueprintType)
class VSPCOMMONUTILS_API UVSPEntity final : public UObject
{
	GENERATED_BODY()
public:
	template<typename T>
	UVSPEntity* AddComponent(T* Object);

	template<typename T>
	UVSPEntity* AddComponent(const T* Object);

	template<typename T>
	void RemoveComponent(T* Object);

	template<typename T>
	void RemoveComponent();

	template<typename T>
	T* GetComponent() const;

	UFUNCTION(BlueprintCallable, meta = (DeterminesOutputType = "ComponentType", DynamicOutputParam = "OutInterface"))
	void GetComponent(TSubclassOf<UObject> ComponentType, UObject*& OutInterface);

	UFUNCTION(BlueprintCallable)
	bool IsValid() const;

	UFUNCTION(BlueprintPure)
	int64 GetID() const;

	constexpr static int64 InvalidID = 0;

private:
	int64 ID = InvalidID;

	void AddComponentLocal(const UClass* Key, UObject* Value);
	void RemoveComponentLocal(const UClass* Key);
	void Register(const UClass* Key);
	void Unregister(const UClass* Key);
	int64 Reset();

	UPROPERTY()
	TMap<const UClass*, UObject*> Components;

	UPROPERTY()
	UVSPDataStorage* DataStorage;

	friend class UVSPDataStorage;
};

#if CPP
	#include "VSPEntity.inl"
#endif
