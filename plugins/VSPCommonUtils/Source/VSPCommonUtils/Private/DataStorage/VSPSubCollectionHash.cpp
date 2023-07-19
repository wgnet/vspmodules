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


#include "DataStorage/VSPSubCollectionHash.h"

UVSPEntity* UVSPSubCollectionHash::FindEntity(const FString& Key)
{
	const auto Result = Map.Find(Key);
	return Result ? *Result : nullptr;
}

void UVSPSubCollectionHash::Add(UVSPEntity* Entity)
{
	Super::Add(Entity);
	Map.Add(EntityHashFunction(Entity), Entity);
}

void UVSPSubCollectionHash::Remove(UVSPEntity* Entity)
{
	Map.Remove(EntityHashFunction(Entity));
	Super::Remove(Entity);
}
