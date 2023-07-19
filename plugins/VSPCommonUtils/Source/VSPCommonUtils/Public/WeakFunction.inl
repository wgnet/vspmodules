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

template<class UObjectType, class CallableType, class ReturnType>
auto MakeWeakFunction(
	UObjectType* Object,
	CallableType InFunction,
	ReturnType&& InFailureReturn,
	bool InDeadWeakIsError,
	TEnableIfUObject<UObjectType>)
{
	VSPCheck(!InDeadWeakIsError || IsValid(Object));

	return [WeakObject = MakeWeakObjectPtr(Object),
			Function = MoveTemp(InFunction),
			FailureReturn = Forward<ReturnType>(InFailureReturn),
			bDeadWeakIsError = InDeadWeakIsError](auto&&... args) mutable
	{
		if (!WeakObject.IsValid())
		{
			VSPCheck(!bDeadWeakIsError);
			return MoveTemp(FailureReturn);
		}

		return Function(Forward<decltype(args)>(args)...);
	};
}

template<class UObjectType, class CallableType>
auto MakeWeakFunction(
	UObjectType* Object,
	CallableType InFunction,
	bool InDeadWeakIsError,
	TEnableIfUObject<UObjectType>)
{
	VSPCheck(!InDeadWeakIsError || IsValid(Object));

	return [WeakObject = MakeWeakObjectPtr(Object),
			Function = MoveTemp(InFunction),
			bDeadWeakIsError = InDeadWeakIsError](auto&&... args)
	{
		if (!WeakObject.IsValid())
		{
			VSPCheck(!bDeadWeakIsError);
			return;
		}

		return Function(Forward<decltype(args)>(args)...);
	};
}

template<class SharedFromThisObjectType, class CallableType, class ReturnType>
auto MakeWeakFunction(
	SharedFromThisObjectType* Object,
	CallableType InFunction,
	ReturnType&& InFailureReturn,
	bool InDeadWeakIsError,
	TEnableIfSharedFromThis<SharedFromThisObjectType>)
{
	VSPCheck(!InDeadWeakIsError || Object);

	return [WeakObject = TWeakPtr<SharedFromThisObjectType>(
				Object ? TSharedPtr<SharedFromThisObjectType>(Object->AsShared()) : nullptr),
			Function = MoveTemp(InFunction),
			FailureReturn = Forward<ReturnType>(InFailureReturn),
			bDeadWeakIsError = InDeadWeakIsError](auto&&... args) mutable
	{
		if (!WeakObject.IsValid())
		{
			VSPCheck(!bDeadWeakIsError);
			return MoveTemp(FailureReturn);
		}

		return Function(Forward<decltype(args)>(args)...);
	};
}

template<class SharedFromThisObjectType, class CallableType>
auto MakeWeakFunction(
	SharedFromThisObjectType* Object,
	CallableType InFunction,
	bool InDeadWeakIsError,
	TEnableIfSharedFromThis<SharedFromThisObjectType>)
{
	VSPCheck(!InDeadWeakIsError || Object);

	return [WeakObject = TWeakPtr<SharedFromThisObjectType>(
				Object ? TSharedPtr<SharedFromThisObjectType>(Object->AsShared()) : nullptr),
			Function = MoveTemp(InFunction),
			bDeadWeakIsError = InDeadWeakIsError](auto&&... args)
	{
		if (!WeakObject.IsValid())
		{
			VSPCheck(!bDeadWeakIsError);
			return;
		}

		return Function(Forward<decltype(args)>(args)...);
	};
}
