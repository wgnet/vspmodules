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

#include "EventBus/VSPBaseEventBusHandler.h"

#include "Templates/Function.h"

template<typename EventType>
class FVSPEventBusHandler : public FVSPBaseEventBusHandler
{
public:
	using THandlerFunction = TFunction<void(const EventType&)>;

	FVSPEventBusHandler(THandlerFunction InFunction, UObject* InWeakObject, const FString& InName, uint32 InHash);

private:
	virtual void Broadcast(const FVSPEventBusEvent& Event) const override;

private:
	const THandlerFunction Function;
};

template<typename EventType>
FVSPEventBusHandler<EventType>::FVSPEventBusHandler(
	THandlerFunction InFunction,
	UObject* InWeakObject,
	const FString& InName,
	uint32 InHash)
	: FVSPBaseEventBusHandler(InWeakObject, InName, InHash)
	, Function(MoveTemp(InFunction))
{
}

template<typename EventType>
void FVSPEventBusHandler<EventType>::Broadcast(const FVSPEventBusEvent& Event) const
{
	// Function is guaranteed to be valid by UVSPEventBus::Add
	// Event is guaranteed to be of type EventType by UVSPEventBus::Subscriptions
	if (GetName().Len() == 0 || GetName() == Event.Name)
		Function(static_cast<const EventType&>(Event));
}
