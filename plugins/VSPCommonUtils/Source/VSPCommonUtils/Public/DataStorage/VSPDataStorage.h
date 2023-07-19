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
#include "UObject/Object.h"

#include "VSPDataStorage.generated.h"


class UVSPEntity;
class UVSPCollection;

/**
	* @brief ECS alike data structure 
	* @details DataStorage implements ECS data storage structure: abstract entities contains
	* structured data in components. Entities grouped into collections by components it's owns.
	* As a result we have data, grouped in to directions: different components in one entity and different entities
	* in same components collection.
	*
	* Let's write some pseudocode:
	*
	*	auto DS = NewObject<UVSPDataStorage>();
	*	auto Entity1 = DS->CreateEntity();
	*	auto Entity2 = DS->CreateEntity();
	*	auto Entity3 = DS->CreateEntity();
	*
	*	Entity1->AddComponent(ComponentA);
	*
	*	Entity2->AddComponent(ComponentA);
	*	Entity2->AddComponent(ComponentC);
	*	
	*	Entity3->AddComponent(ComponentA);
	*	Entity3->AddComponent(ComponentB);
	*	Entity3->AddComponent(ComponentC);
	*
	* After that our DS will be looks literally like this: 	
	*
	*				|	Entity1		|	Entity2		|	Entity3		|
	* --------------|---------------|---------------|---------------|
	* ComponentA	|		+		|		+		|		+		|  (Collection<ComponentA>)
	* --------------|---------------|---------------|---------------|
	* ComponentB	|				|				|		+		|  (Collection<ComponentB>)
	* --------------|---------------|---------------|---------------|
	* ComponentC	|				|		+		|		+		|  (Collection<ComponentC>)
	* --------------|---------------|---------------|---------------|
	*
	* Each entity looks like:
	* Entity1 == [ComponentA]
	* Entity2 == [ComponentA, ComponentC]
	* Entity3 == [ComponentA, ComponentB, ComponentC]
	*
	* Each collection: 
	* Collection<ComponentA> == [Entity1, Entity2, Entity3]
	* Collection<ComponentB> == [Entity3]
	* Collection<ComponentC> == [Entity2, Entity3]
	* 
**/

UCLASS(BlueprintType)
class VSPCOMMONUTILS_API UVSPDataStorage final : public UObject
{
	GENERATED_BODY()

public:
	/**
		@brief Creates new empty entity
		@return Created entity ptr
	**/
	UVSPEntity* CreateEntity();

	/**
		@brief Remove an entity from DataStorage and Collections, also removes it's components with data.
		@param Entity - Entity to remove.
	**/
	void RemoveEntity(UVSPEntity* Entity);

	/**
		@brief Get Entity by it's unique ID.
		@param EntityId - unique ID.
		@return Found entity ptr. Can be nullptr if there is no matching entity in DataStorage
	**/
	UFUNCTION(BlueprintCallable)
	UVSPEntity* GetEntity(int64 EntityId);

	/**
		@brief Get the collection.
		@details Get collection of entities, where all entities contains component of provided type
		@tparam ComponentType - component type for this collection.
	**/
	template<typename ComponentType>
	UVSPCollection* GetCollection();

	/**
		@brief  Remove all entities with ComponentType from DataStorage.
		@tparam ComponentType - component type for this collection.
	**/
	template<typename ComponentType>
	void ClearCollection();

	/**
		@brief Get the collection. Exposed for blueprints
		@details Get collection of entities, where all entities contains component of provided type
		@param ComponentType - component type for this collection.
		@param OutInterface - ref for return value.
	**/
	UFUNCTION(BlueprintCallable)
	void GetCollection(TSubclassOf<UObject> ComponentType, UVSPCollection*& OutInterface);

private:
	UVSPCollection* GetCollectionLocal(const UClass* Key);

	void AddComponent(const UClass* Key, UVSPEntity* Entity);
	void RemoveComponent(const UClass* Key, UVSPEntity* Entity);

	UPROPERTY()
	TMap<const UClass*, UVSPCollection*> Collections;

	UPROPERTY()
	TMap<int64, UVSPEntity*> Entities;

	friend class UVSPEntity;
};

#if CPP
	#include "VSPDataStorage.inl"
#endif
