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

namespace VSPUtils
{
	template<typename T>
	T* ConstCast(const T* Ptr)
	{
		return const_cast<T*>(Ptr);
	}

	template<typename T>
	const T* ConstCast(T* Ptr)
	{
		return const_cast<const T*>(Ptr);
	}

	template<typename T>
	T& ConstCast(const T& Ptr)
	{
		return const_cast<T&>(Ptr);
	}

	template<typename T>
	const T& ConstCast(T& Ptr)
	{
		return const_cast<const T&>(Ptr);
	}

	template<typename T, typename InAllocator, typename OutAllocator = FDefaultAllocator>
	void ConstCastArray(const TArray<const T*, InAllocator>& InArray, TArray<T*, OutAllocator>& OutArray)
	{
		OutArray.Reserve(InArray.Num());
		for (const T* Element : InArray)
			OutArray.Add(VSPUtils::ConstCast(Element));
	}

	template<typename T, typename InAllocator, typename OutAllocator = FDefaultAllocator>
	void ConstCastArray(const TArray<T*, InAllocator>& InArray, TArray<const T*, OutAllocator>& OutArray)
	{
		OutArray.Reserve(InArray.Num());
		for (T* Element : InArray)
			OutArray.Add(VSPUtils::ConstCast(Element));
	}

	template<typename T, typename InAllocator, typename OutAllocator = FDefaultAllocator>
	TArray<const T*, OutAllocator> ConstCastArray(const TArray<T*, InAllocator>& InArray)
	{
		TArray<const T*, OutAllocator> Result;
		ConstCastArray(InArray, Result);
		return Result;
	}

	template<typename T, typename InAllocator, typename OutAllocator = FDefaultAllocator>
	TArray<T*, OutAllocator> ConstCastArray(const TArray<const T*, InAllocator>& InArray)
	{
		TArray<T*, OutAllocator> Result;
		ConstCastArray(InArray, Result);
		return Result;
	}
}
