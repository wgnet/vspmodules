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
	/**
		@brief   Convert type to its string representation (aliases will be fully expanded)
		@warning Result is compiler dependent! Do not use it for comparison and same things
		@tparam  T - type to convert
		@retval    - resulting string
	**/
	template<typename T>
	FString GetTypeName()
	{
		FString TypeName;

#if defined(_MSC_VER)

		TypeName = __FUNCSIG__;

		// remove start
		constexpr char TypeNameStartMarker[] = "GetTypeName<";
		int32 TypeNameStart = TypeName.Find(TypeNameStartMarker);
		if (TypeNameStart != INDEX_NONE)
			TypeName.RemoveAt(0, TypeNameStart + sizeof(TypeNameStartMarker) - 1);

		// remove long lambda names
		while (true)
		{
			constexpr char LambdaNameStartMarker[] = "<lambda_";
			int32 LambdaNameStart = TypeName.Find(LambdaNameStartMarker);
			if (LambdaNameStart == INDEX_NONE)
				break;

			TypeName.RemoveAt(LambdaNameStart + sizeof(LambdaNameStartMarker) - 2, 33);
		}

		// remove end
		constexpr char TypeNameEndMarker[] = ">(void)";
		int32 TypeNameEnd = TypeName.Find(TypeNameEndMarker);
		if (TypeNameEnd != INDEX_NONE)
			TypeName.RemoveAt(TypeNameEnd, sizeof(TypeNameEndMarker) - 1);

#elif defined(__clang__)

		TypeName = __PRETTY_FUNCTION__;

		// remove start
		constexpr char TypeNameStartMarker[] = "T = ";
		int32 TypeNameStart = TypeName.Find(TypeNameStartMarker);
		if (TypeNameStart != INDEX_NONE)
			TypeName.RemoveAt(0, TypeNameStart + sizeof(TypeNameStartMarker) - 1);

		// remove end
		constexpr char TypeNameEndMarker[] = "]";
		int32 TypeNameEnd = TypeName.Find(TypeNameEndMarker);
		if (TypeNameEnd != INDEX_NONE)
			TypeName.RemoveAt(TypeNameEnd, sizeof(TypeNameEndMarker) - 1);

#endif

		return TypeName;
	}
}
