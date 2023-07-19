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

template<class ComponentType, class PredicateType>
TArray<ComponentType*> VSPDataStorageUtils::GetComponentsByPredicate(
	UVSPDataStorage* DataStorage,
	const PredicateType& InPredicate)
{
	TArray<ComponentType*> Components;

	const auto Collection = DataStorage->GetCollection<ComponentType>();
	VSPCheckReturn(Collection, Components);

	for (const auto& Entity : Collection->GetEntities())
	{
		const auto Component = Entity->template GetComponent<ComponentType>();
		VSPCheckContinue(Component);
		if (InPredicate(Component))
			Components.Add(Component);
	}

	return Components;
}

template<class ComponentType, class PredicateType>
int32 VSPDataStorageUtils::RemoveEntitiesByPredicate(UVSPDataStorage* DataStorage, const PredicateType& Predicate)
{
	int32 NumRemoved = 0;

	const auto Collection = DataStorage->GetCollection<ComponentType>();
	VSPCheckReturn(Collection, NumRemoved);

	for (UVSPEntity* Entity : Collection->GetEntities())
	{
		VSPCheckContinue(Entity);

		auto Component = Entity->GetComponent<ComponentType>();
		VSPCheckContinue(Component);

		if (Predicate(Entity, Component))
		{
			DataStorage->RemoveEntity(Entity);
			++NumRemoved;
		}
	}

	return NumRemoved;
}
