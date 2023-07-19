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
#include "DataStorage/VSPCollection.h"

#include "VSPCheck.h"


TArray<UVSPEntity*> UVSPCollection::GetEntities()
{
	return Entities;
}

int32 UVSPCollection::Num() const
{
	return Entities.Num();
}

void UVSPCollection::AddSubCollection(const FGameplayTag& Tag, UVSPCollection* SubCollection)
{
	VSPCheckReturn(!SubCollections.Find(Tag));

	SubCollections.Emplace(Tag, SubCollection);
	for (const auto Entity : Entities)
	{
		SubCollection->Add(Entity);
	}
}

void UVSPCollection::RemoveSubCollection(const FGameplayTag& Tag)
{
	SubCollections.Remove(Tag);
}

UVSPCollection* UVSPCollection::GetSubCollection(const FGameplayTag& Tag)
{
	const auto Result = SubCollections.Find((Tag));
	return Result ? *Result : nullptr;
}

void UVSPCollection::Update()
{
	for (const auto& SubCollection : SubCollections)
	{
		SubCollection.Value->Update();
	}
}

void UVSPCollection::Add(UVSPEntity* Entity)
{
	Entities.Add(Entity);
	OnAdded.Broadcast(Entity);
	for (const auto& SubCollection : SubCollections)
	{
		SubCollection.Value->Add(Entity);
	}
}

void UVSPCollection::Remove(UVSPEntity* Entity)
{
	for (const auto& SubCollection : SubCollections)
	{
		SubCollection.Value->Remove(Entity);
	}
	Entities.Remove(Entity);
	OnRemoved.Broadcast(Entity);
}
