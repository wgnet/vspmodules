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
#include "DataStorage/VSPDataStorage.h"

#include "DataStorage/VSPCollection.h"
#include "DataStorage/VSPEntity.h"

namespace FVSPDataStorageLocal
{
	uint64 GetNextID()
	{
		static uint64 NextID = 0;
		++NextID;
		return NextID;
	}
}


UVSPEntity* UVSPDataStorage::CreateEntity()
{
	const auto EntityID = FVSPDataStorageLocal::GetNextID();
	UVSPEntity* Entity = NewObject<UVSPEntity>();

	Entity->ID = EntityID;
	Entity->DataStorage = this;

	Entities.Emplace(EntityID, Entity);
	return Entity;
}

void UVSPDataStorage::RemoveEntity(UVSPEntity* Entity)
{
	VSPCheckReturn(Entity->IsValid());

	const auto EntityID = Entity->Reset();
	Entities.Remove(EntityID);
}

UVSPEntity* UVSPDataStorage::GetEntity(const int64 EntityId)
{
	const auto Entity = Entities.Find(EntityId);
	return Entity ? *Entity : nullptr;
}

void UVSPDataStorage::GetCollection(const TSubclassOf<UObject> ComponentType, UVSPCollection*& OutInterface)
{
	OutInterface = GetCollectionLocal(ComponentType.Get());
}

UVSPCollection* UVSPDataStorage::GetCollectionLocal(const UClass* Key)
{
	if (const auto CollectionRef = Collections.Find(Key))
		return *CollectionRef;

	return Collections.Emplace(Key, NewObject<UVSPCollection>());
}

void UVSPDataStorage::AddComponent(const UClass* Key, UVSPEntity* Entity)
{
	GetCollectionLocal(Key)->Add(Entity);
}

void UVSPDataStorage::RemoveComponent(const UClass* Key, UVSPEntity* Entity)
{
	GetCollectionLocal(Key)->Remove(Entity);
}
