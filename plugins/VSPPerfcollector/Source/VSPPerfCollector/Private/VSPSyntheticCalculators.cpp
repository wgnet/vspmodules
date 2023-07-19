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
#include "VSPSyntheticCalculators.h"

#include "VSPEventInfo.h"
#include "VSPPerfCollector.h"
#include "VSPPerfEventConfig.h"
#include "VSPThreadInfo.h"
#include "Algo/Accumulate.h"
#include "Dom/JsonObject.h"


const FString FVSPAccumulationCalculator::JsonName = "SyntheticAccumulation";
const FString FVSPInstanceCalculator::JsonName = "InstanceCalculator";
const FString FVSPHighestTimedCalculator::JsonName = "HighestTimedCalculator";


IVSPSyntheticCalculator::IVSPSyntheticCalculator(TSharedPtr<FVSPEventInfo> Info):
	EventInfo(Info)
{
}

TSharedPtr<IVSPSyntheticCalculator> IVSPSyntheticCalculator::TryCreateFromName(const FString& Name, TSharedPtr<FVSPEventInfo> Info)
{
	if (Name == FVSPAccumulationCalculator::JsonName)
		return MakeShared<FVSPAccumulationCalculator>(Info);

	if (Name == FVSPInstanceCalculator::JsonName)
		return MakeShared<FVSPInstanceCalculator>(Info);

	if (Name == FVSPHighestTimedCalculator::JsonName)
		return MakeShared<FVSPHighestTimedCalculator>(Info);
	
	return nullptr;
}

FVSPAccumulationCalculator::FVSPAccumulationCalculator(TSharedPtr<FVSPEventInfo> Info):
	IVSPSyntheticCalculator(Info)
{
}

bool FVSPAccumulationCalculator::SetupFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	if (!JsonObject.IsValid() || !EventInfo.IsValid())
		return false;

	const TSharedPtr<FJsonValue> IncField = JsonObject->TryGetField("Increment");
	const TSharedPtr<FJsonValue> DecField = JsonObject->TryGetField("Decrement");

	if (IncField)
		IncFilter = IVSPSyntheticFilter::TryCreateFromJson(IncField, EventInfo.Pin());

	if (DecField)
		DecFilter = IVSPSyntheticFilter::TryCreateFromJson(DecField, EventInfo.Pin());

	if (!IncFilter && !DecFilter)
		return false;

	return true;
}

void FVSPAccumulationCalculator::Search(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	if (IncFilter)
	{
		if (const FVSPTimerInfo* Timer = IncFilter->ProcessStack(Stack, Thread))
		{
			FVSPTimerInfo* Founded = IncTimers.Find(Timer->Id);
			if (Founded)
				Founded->Duration += Timer->Duration;
			else
				Founded = &IncTimers.Emplace(Timer->Id, *Timer);
		}
	}

	if (DecFilter)
	{
		if (const FVSPTimerInfo* Timer = DecFilter->ProcessStack(Stack, Thread))
		{
			FVSPTimerInfo* Founded = DecTimers.Find(Timer->Id);
			if (Founded)
				Founded->Duration += Timer->Duration;
			else
				Founded = &DecTimers.Emplace(Timer->Id, *Timer);
		}
	}
}

double FVSPAccumulationCalculator::CalculateAndDumpTo(FVSPEventInfo& Info)
{
	double Value = 0;
	for (TTuple<uint32, FVSPTimerInfo>& Timer : IncTimers)
	{
		if (Timer.Value.Duration >= 0.0)
		{
			Value += Timer.Value.Duration;
			Timer.Value.Duration = 0;
		}
	}

	for (TTuple<uint32, FVSPTimerInfo>& Timer : DecTimers)
	{
		if (Timer.Value.Duration >= 0.0)
		{
			Value -= Timer.Value.Duration;
			Timer.Value.Duration = 0;
		}
	}

	IncTimers.Empty();
	DecTimers.Empty();
	return Value;
}

FVSPInstanceCalculator::FVSPInstanceCalculator(TSharedPtr<FVSPEventInfo> Info):
	IVSPSyntheticCalculator(Info)
{
}

bool FVSPInstanceCalculator::SetupFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	const TSharedPtr<FVSPEventInfo> Event = EventInfo.Pin();
	if (!JsonObject.IsValid() || !EventInfo.IsValid() || !Event)
		return false;

	const TSharedPtr<FJsonValue> InstanceIdExtractorValue = JsonObject->TryGetField("InstanceIdExtractor");
	if (!InstanceIdExtractorValue || InstanceIdExtractorValue->Type != EJson::String)
		return false;
	
	InstanceIdExtractor = MakeUnique<FRegexPattern>(InstanceIdExtractorValue->AsString());
	
	if (const TSharedPtr<FJsonValue> FilterValue = JsonObject->TryGetField("Filter"))
	{
		Filter = IVSPSyntheticFilter::TryCreateFromJson(FilterValue, Event);
		if (Filter)
			return true;
	}
	
	return false;
}

void FVSPInstanceCalculator::Search(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	const TSharedPtr<FVSPEventInfo> Event = EventInfo.Pin();
	const FVSPTimerInfo* Timer = Filter->ProcessStack(Stack, Thread);
	
	if (!Timer || !Event)
		return;

	FRegexMatcher Matcher(*InstanceIdExtractor.Get(), Timer->Name);
	if (!Matcher.FindNext())
		return;
	
	const FString Group = Matcher.GetCaptureGroup(1);
	if (Group.IsEmpty())
		return;
	
	const TSharedPtr<FVSPEventInfo>* Child = Event->Children.FindByPredicate(
		[&Group](const TSharedPtr<FVSPEventInfo>& Child)
		{
			return Child->Name == Group;
		});
	
	if (!Child || !Child->IsValid())
	{
		const FString ChildName = FString::Printf(TEXT("%s: %s"), *Event->Name, *Group);
		const TSharedPtr<FVSPEventInfo> NewChild = MakeShared<FVSPEventInfo>(Group);
		NewChild->Value = Timer->Duration;
		NewChild->Alias = ChildName;
		Event->Children.Push(NewChild);
		NewChild->Parent = Event;
		Thread.FlatTotalEvents.Push(NewChild);
	}
	else
	{
		Child->Get()->Value += Timer->Duration;
	}
}

double FVSPInstanceCalculator::CalculateAndDumpTo(FVSPEventInfo& Info)
{
	const TSharedPtr<FVSPEventInfo> Event = EventInfo.Pin();
	if (!Event)
		return 0;
	
	double Value = TNumericLimits<double>::Min();
	for (const TSharedPtr<FVSPEventInfo>& Child : Event->Children)
	{
		if (Child && Child->Value > Value)
			Value = Child->Value;
	}
	
	return Value;
}

FVSPHighestTimedCalculator::FVSPHighestTimedCalculator(TSharedPtr<FVSPEventInfo> Info):
	IVSPSyntheticCalculator(Info),
	FadeOutTime(0),
	LastTime(0),
	HighestValue(-1)
{
}

bool FVSPHighestTimedCalculator::SetupFromJson(const TSharedPtr<FJsonObject>& JsonObject)
{
	const TSharedPtr<FVSPEventInfo> Event = EventInfo.Pin();
	if (!JsonObject.IsValid() || !EventInfo.IsValid() || !Event)
		return false;

	constexpr const TCHAR* FadeOutTimeName = TEXT("FadeOutTime");
	for (TTuple<FString, TSharedPtr<FJsonValue>> Field: JsonObject->Values)
	{
		if (Field.Key == FadeOutTimeName || !Field.Value || Field.Value->Type != EJson::Object)
			continue;

		InnerCalculator = TryCreateFromName(Field.Key, Event);
		if (InnerCalculator)
		{
			if (InnerCalculator->SetupFromJson(Field.Value->AsObject()))
			{
				break;
			}

			InnerCalculator.Reset();
			UE_LOG(LogVSPPerfCollector, Warning, TEXT("%s: Inner synthetic event `%s` has wrong data structure"), *Event->GetDisplayName(), *Field.Key);
		}
	}

	if (!InnerCalculator)
		return false;

	const TSharedPtr<FJsonValue> FadeOutTimeJson = JsonObject->TryGetField(FadeOutTimeName);
	if (!FadeOutTimeJson || FadeOutTimeJson->Type != EJson::Number)
		return false;

	FadeOutTime = FadeOutTimeJson->AsNumber();

	return true;
}

void FVSPHighestTimedCalculator::Search(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	const TSharedPtr<FVSPEventInfo> Event = EventInfo.Pin();
	if (InnerCalculator && Event)
		InnerCalculator->Search(Stack, Thread);
}

double FVSPHighestTimedCalculator::CalculateAndDumpTo(FVSPEventInfo& Info)
{
	const double Value = InnerCalculator->CalculateAndDumpTo(Info);
	const double CurrentTime = FPlatformTime::Seconds();
	const double Duration = CurrentTime - LastTime;
	if (Value > HighestValue || Duration >= FadeOutTime)
	{
		HighestValue = Value;
		LastTime = CurrentTime;
	}

	return HighestValue;
}
