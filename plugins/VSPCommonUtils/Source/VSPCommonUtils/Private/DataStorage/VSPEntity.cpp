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
#include "DataStorage/VSPEntity.h"

#include "DataStorage/VSPDataStorage.h"
#include "VSPCheck.h"


void UVSPEntity::GetComponent(TSubclassOf<UObject> ComponentType, UObject*& OutInterface)
{
	const auto Result = Components.Find(ComponentType.Get());
	OutInterface = Result ? *Result : nullptr;
}

int64 UVSPEntity::GetID() const
{
	return ID;
}

bool UVSPEntity::IsValid() const
{
	return ID != InvalidID;
}

void UVSPEntity::AddComponentLocal(const UClass* Key, UObject* Value)
{
	VSPCheckReturn(IsValid());
	VSPCheckReturn(!Components.Find(Key));

	Components.Emplace(Key, Value);
	Register(Key);
}

void UVSPEntity::RemoveComponentLocal(const UClass* Key)
{
	VSPCheckReturn(IsValid());

	const auto Result = Components.Remove(Key);
	VSPCheckReturn(Result == 1);

	Unregister(Key);
}

void UVSPEntity::Register(const UClass* Key)
{
	DataStorage->AddComponent(Key, this);
}

void UVSPEntity::Unregister(const UClass* Key)
{
	DataStorage->RemoveComponent(Key, this);
}

int64 UVSPEntity::Reset()
{
	const auto Result = ID;

	for (const auto Item : Components)
	{
		DataStorage->RemoveComponent(Item.Key, this);
	}
	Components.Reset();

	ID = InvalidID;
	DataStorage = nullptr;

	return Result;
}
