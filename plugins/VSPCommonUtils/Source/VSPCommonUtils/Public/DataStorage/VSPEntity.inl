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
template<typename T>
UVSPEntity* UVSPEntity::AddComponent(T* Object)
{
	VSPCheckReturn(Object, this);
	const UClass* Key = T::StaticClass();
	UObject* Value = Object;
	AddComponentLocal(Key, Value);
	return this;
}

template<typename T>
UVSPEntity* UVSPEntity::AddComponent(const T* Object)
{
	return AddComponent(const_cast<T*>(Object));
}

template<typename T>
void UVSPEntity::RemoveComponent(T* Object)
{
	RemoveComponent<T>();
}

template<typename T>
void UVSPEntity::RemoveComponent()
{
	const UClass* Key = T::StaticClass();
	RemoveComponentLocal(Key);
}

template<typename T>
T* UVSPEntity::GetComponent() const
{
	const UClass* Key = T::StaticClass();
	const auto Result = Components.Find(Key);
	return Result ? static_cast<T*>(*Result) : nullptr;
}
