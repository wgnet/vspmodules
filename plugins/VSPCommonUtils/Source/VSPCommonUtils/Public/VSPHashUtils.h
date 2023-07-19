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

template<typename... Types>
uint32 GetCombinedHash(Types&&... Values)
{
	uint32 Result = 0;
	int32 _[] { (Result = ::HashCombine(Result, ::GetTypeHash(Forward<Types>(Values))), 0)... };
	return Result;
}

template<typename TArrayElement>
uint32 GetArrayTypeHash(const TArray<TArrayElement*>& Array)
{
	uint32 Hash = 0;
	for (const auto& Element : Array)
		Hash = HashCombine(Hash, GetTypeHash(Element));
	return Hash;
}

template<typename TArrayElement>
uint32 GetArrayTypeHash(const TArray<TArrayElement>& Array)
{
	uint32 Hash = 0;
	for (const auto& Element : Array)
		Hash = HashCombine(Hash, GetTypeHash(&Element));
	return Hash;
}
