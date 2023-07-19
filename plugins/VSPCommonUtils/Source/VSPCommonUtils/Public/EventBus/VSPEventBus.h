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

#include "EventBus/VSPEventBusHandler.h"
#include "VSPCheck.h"

#include "VSPEventBus.generated.h"

/**
	* @brief Event System 
	* @details Generally an EventBus is a mechanism that allows different systems to communicate with each other
	* without knowing about each other.
	* A system can send an Event to the EventBus without knowing who will pick it up or how many others will pick it up.
	* Systems can also listen to Events on an EventBus, without knowing who sent the Events.
	* That way, systems can communicate without depending on each other.
**/
UCLASS()
class VSPCOMMONUTILS_API UVSPEventBus : public UObject
{
	GENERATED_BODY()

	using TEventHandler = TSharedPtr<FVSPBaseEventBusHandler>;
	using TEventHandlers = TArray<TEventHandler>;

public:
	/**
        @brief  Subscribe via object + method pointers
        @tparam ReceiverType  - receiver type, must be inherited from UObject to track lifetime
        @tparam EventType     - event type, must be inherited from FVSPEventBusEvent
        @param  EventReceiver - object to call Method from
        @param  Method        - method pointer, must be invocable with EventType
        @param  EventName     - optional event name to observe specific events of EventType only
    **/
	template<typename ReceiverType, typename EventType>
	void Subscribe(
		ReceiverType* EventReceiver,
		void (ReceiverType::*Method)(const EventType&),
		const FString& EventName = "");

	/**
        @brief  Unsubscribe via object + method pointers
        @tparam ReceiverType  - receiver type, must be inherited from UObject to track lifetime
        @tparam EventType     - event type, must be inherited from FVSPEventBusEvent
        @param  EventReceiver - EventReceiver specified in Subscribe
        @param  Method        - Method specified in Subscribe
        @param  EventName     - EventName specified in Subscribe
    **/
	template<typename ReceiverType, typename EventType>
	void Unsubscribe(
		ReceiverType* EventReceiver,
		void (ReceiverType::*Method)(const EventType&),
		const FString& EventName = "");

	/**
        @brief  Subscribe via any callable type
        @tparam EventType    - event type, must be inherited from FVSPEventBusEvent
        @tparam CallableType - callable type, e.g. TFunction, lambda, ect. must be invocable with EventType
        @param  InWeakObj    - object pointer to track lifetime
        @param  InCallable   - callable object
        @param  EventName    - optional event name to observe specific events of EventType only
    **/
	template<typename EventType, typename CallableType>
	void Subscribe(UObject* InWeakObj, CallableType InCallable, const FString& EventName = "");

	/**
        @brief  Unsubscribe for subscriptions with callable
        @tparam EventType    - event type, must be inherited from FVSPEventBusEvent
        @tparam CallableType - callable type, e.g. TFunction, lambda, ect. must be invocable with EventType
        @param  InWeakObj    - InWeakObj specified in Subscribe
        @param  EventName    - EventName specified in Subscribe
    **/
	template<typename EventType, typename CallableType>
	void Unsubscribe(const UObject* InWeakObj, const FString& EventName = "");

	/**
		@brief  Unsubscribe all subscribers tracked via InWeakObj
		@tparam InWeakObj - track pointer to check
	**/
	void UnsubscribeAll(const UObject* InWeakObj);

	/**
        @brief  Broadcast event
        @tparam EventType - event type, must be inherited from FVSPEventBusEvent
        @param  Event     - event object. set Name to broadcast to specific subscribers only
    **/
	template<typename EventType>
	void Broadcast(const EventType& Event);

	/**
        @brief Clear all subscriptions
    **/
	void Clear();

private:
	template<typename EventType, typename CallableType>
	void Add(CallableType InCallable, UObject* InWeakObject, const FString& InName, uint32 Hash);

	template<typename EventType>
	void Remove(uint32 Hash);

private:
	FCriticalSection Mutex;
	TMap<UStruct*, TEventHandlers> Subscriptions;
};

#if CPP
	#include "EventBus/VSPEventBus.inl"
#endif
