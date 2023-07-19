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
#include "Templates/TypeHash.h"

namespace VSPEventBusDetails
{
	template<typename ReceiverType>
	constexpr void CheckReceiverType()
	{
		static_assert(TIsDerivedFrom<ReceiverType, UObject>::Value, "ReceiverType must be inherited from UObject");
	}

	template<typename EventType>
	constexpr void CheckEventType()
	{
		static_assert(
			TIsDerivedFrom<EventType, FVSPEventBusEvent>::Value,
			"EventType must be inherited from FVSPEventBusEvent");
	}

	template<typename CallableType, typename EventType>
	constexpr void CheckCallable()
	{
		static_assert(
			TIsInvocable<CallableType, const EventType&>::Value,
			"CallableType must be invocable with EventType");
	}

	template<typename ReceiverType, typename EventType>
	uint32 GetMethodHash(void (ReceiverType::*Method)(const EventType&))
	{
		constexpr auto Size = sizeof(Method);
		char Buffer[Size];
		FMemory::Memcpy(&Buffer, &Method, Size);
		uint32 Hash = 0;
		for (int i = 0; i < Size; ++i)
		{
			Hash = HashCombine(Hash, GetTypeHash(Buffer[i]));
		}
		return Hash;
	}
}

template<typename ReceiverType, typename EventType>
void UVSPEventBus::Subscribe(
	ReceiverType* EventReceiver,
	void (ReceiverType::*Method)(const EventType&),
	const FString& EventName)
{
	VSPEventBusDetails::CheckReceiverType<ReceiverType>();
	VSPEventBusDetails::CheckEventType<EventType>();

	const auto MethodHash = VSPEventBusDetails::GetMethodHash(Method);
	Add<EventType>(
		[EventReceiver, Method](const EventType& Event)
		{
			(EventReceiver->*Method)(Event);
		},
		EventReceiver,
		EventName,
		HashCombine(GetTypeHash(EventReceiver), HashCombine(MethodHash, GetTypeHash(EventName))));
}

template<typename ReceiverType, typename EventType>
void UVSPEventBus::Unsubscribe(
	ReceiverType* EventReceiver,
	void (ReceiverType::*Method)(const EventType&),
	const FString& EventName)
{
	VSPEventBusDetails::CheckReceiverType<ReceiverType>();
	VSPEventBusDetails::CheckEventType<EventType>();

	const auto MethodHash = VSPEventBusDetails::GetMethodHash(Method);
	Remove<EventType>(HashCombine(GetTypeHash(EventReceiver), HashCombine(MethodHash, GetTypeHash(EventName))));
}

template<typename EventType, typename CallableType>
void UVSPEventBus::Subscribe(UObject* InWeakObj, CallableType InCallable, const FString& EventName)
{
	VSPEventBusDetails::CheckEventType<EventType>();
	VSPEventBusDetails::CheckCallable<CallableType, EventType>();

	Add<EventType>(
		MoveTemp(InCallable),
		InWeakObj,
		EventName,
		HashCombine(GetTypeHash(InWeakObj), GetTypeHash(EventName)));
}

template<typename EventType, typename CallableType>
void UVSPEventBus::Unsubscribe(const UObject* InWeakObj, const FString& EventName)
{
	VSPEventBusDetails::CheckEventType<EventType>();

	Remove<EventType>(HashCombine(GetTypeHash(InWeakObj), GetTypeHash(EventName)));
}

inline void UVSPEventBus::UnsubscribeAll(const UObject* InWeakObj)
{
	FScopeLock ScopeLock(&Mutex);

	for (TPair<UStruct*, TEventHandlers>& Subscription : Subscriptions)
	{
		auto& Handlers = Subscription.Value;
		int32 Index = 0;
		while (Index < Handlers.Num())
		{
			const auto& Handler = Handlers[Index];
			if (Handler->MarkAsRemoved(Handler->GetWeakObject() == InWeakObj))
			{
				Handlers.RemoveAtSwap(Index);
			}
			++Index;
		}
	}
}

template<typename EventType>
void UVSPEventBus::Broadcast(const EventType& Event)
{
	VSPEventBusDetails::CheckEventType<EventType>();

	FScopeLock ScopeLock(&Mutex);

	const TEventHandlers* Handlers = Subscriptions.Find(EventType::StaticStruct());
	if (!Handlers)
		return;

	TEventHandlers HandlersCopy = *Handlers;
	for (const auto& Handler : HandlersCopy)
		if (Handler->IsAlive())
			Handler->Broadcast(Event);
}

inline void UVSPEventBus::Clear()
{
	FScopeLock ScopeLock(&Mutex);

	Subscriptions.Reset();
}

template<typename EventType, typename CallableType>
void UVSPEventBus::Add(CallableType InCallable, UObject* InWeakObject, const FString& InName, uint32 Hash)
{
	VSPEventBusDetails::CheckEventType<EventType>();
	VSPEventBusDetails::CheckCallable<CallableType, EventType>();

	using THandler = FVSPEventBusHandler<EventType>;
	typename THandler::THandlerFunction Function = MoveTemp(InCallable);
	VSPCheckReturn(Function);

	FScopeLock ScopeLock(&Mutex);

	auto& Subscription = Subscriptions.FindOrAdd(EventType::StaticStruct(), TEventHandlers());
	Subscription.Emplace(MakeShared<FVSPEventBusHandler<EventType>>(MoveTemp(Function), InWeakObject, InName, Hash));
}

template<typename EventType>
void UVSPEventBus::Remove(uint32 Hash)
{
	VSPEventBusDetails::CheckEventType<EventType>();

	FScopeLock ScopeLock(&Mutex);

	TEventHandlers* Handlers = Subscriptions.Find(EventType::StaticStruct());
	if (!Handlers)
		return;

	const int32 Index = Handlers->IndexOfByPredicate(
		[Hash](const TEventHandler& Handler)
		{
			return Handler->MarkAsRemoved(Handler->GetHash() == Hash);
		});

	if (Index != INDEX_NONE)
	{
		Handlers->RemoveAtSwap(Index, 1, true);
	}
}
