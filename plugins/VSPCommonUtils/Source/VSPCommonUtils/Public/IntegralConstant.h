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

template<class T, T _Value>
struct IntegralConstant
{
	static constexpr T Value = _Value;

	using ValueType = T;
	using Type = IntegralConstant;

	constexpr operator ValueType() const
	{
		return Value;
	}

	constexpr ValueType operator()() const
	{
		return Value;
	}
};

template<bool Value>
using BoolConstant = IntegralConstant<bool, Value>;

// equivalent of std::true_type and std::false_type
using TrueType = BoolConstant<true>;
using FalseType = BoolConstant<false>;
