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

#include "DataStorage/VSPCollection.h"
#include "DataStorage/VSPDataStorage.h"
#include "DataStorage/VSPEntity.h"

namespace VSPDataStorageUtils
{
	/**
		@brief Get array of components, where all elements satisfies provided predicate
		@tparam ComponentType - component type to get.
		@tparam PredicateType - predicate type, which takes const ComponentType*.
		@param InPredicate - predicate object.
	**/
	template<class ComponentType, class PredicateType>
	TArray<ComponentType*> GetComponentsByPredicate(UVSPDataStorage* DataStorage, const PredicateType& InPredicate);

	/**
		@brief Remove an entities with specific component by predicate from DataStorage and Collections,
			   also removes it's components with data.
		@tparam ComponentType - component type for entities lookup.
		@tparam PredicateType - predicate type.
		@param Predicate - predicate object receiving an entity and a component pointers.
		@return number of entities removed
	**/
	template<class ComponentType, class PredicateType>
	int32 RemoveEntitiesByPredicate(UVSPDataStorage* DataStorage, const PredicateType& Predicate);
}

#include "VSPDataStorageUtils.inl"
