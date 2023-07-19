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
#include "VSPPerfEventConfig.h"
#include "VSPPerfCollector.h"
#include "VSPPerfUtils.h"
#include "VSPSyntheticEvent.h"
#include "Budgets/VSPBudgetBase.h"

TMap<FString, TSubclassOf<UVSPBudgetBase>> FVSPPerfEventConfig::BudgetTypes{};

namespace VSPPerfEventConfig_Local
{
	struct
	{
		FString Name = GET_MEMBER_NAME_STRING_CHECKED(FVSPEventInfo, Name);
		FString Value = GET_MEMBER_NAME_STRING_CHECKED(FVSPEventInfo, Value);
		FString Alias = GET_MEMBER_NAME_STRING_CHECKED(FVSPEventInfo, Alias);
		FString Budget = TEXT("Budget");
		FString Children = GET_MEMBER_NAME_STRING_CHECKED(FVSPEventInfo, Children);
		FString ConfluenceUrl = GET_MEMBER_NAME_STRING_CHECKED(FVSPEventInfo, ConfluenceUrl);
	} EventInfoNames;
}


bool FVSPPerfEventConfig::InitFromJson(TSharedPtr<FJsonObject> JsonData)
{
	if (!JsonData.IsValid())
	{
		UE_LOG(LogVSPPerfCollector, Error, TEXT("Try to init VSPPerfEventConfig from empty json object"));
		return false;
	}
	
	AllEventRoots.Empty();
	const bool bState = ParseJsonData(JsonData, RenderThreadInfo, AllEventRoots) &&
		ParseJsonData(JsonData, GameThreadInfo, AllEventRoots) &&
		ParseJsonData(JsonData, GpuThreadInfo, AllEventRoots);
	
	if (bState)
		JsonConfig = MakeShared<FJsonObject>(*JsonData.Get());
	
	return bState;
}

bool FVSPPerfEventConfig::AddBudgetType(const FString& ConfigTag, TSubclassOf<UVSPBudgetBase> Type)
{
	if (BudgetTypes.Contains(ConfigTag))
		return false;

	BudgetTypes.Add(ConfigTag, Type);
	return true;
}

bool FVSPPerfEventConfig::ParseJsonData(const TSharedPtr<FJsonObject>& RootJsonObject,
                                       FVSPThreadInfo& ThreadInfo,
                                       TArray<TSharedPtr<FVSPEventInfo>>& OutBudget)
{
	ThreadInfo.SyntheticEvents.Empty();
	ThreadInfo.CleanupTimer = 0.f;
	ThreadInfo.EventRootStart = OutBudget.Num();

	const TSharedPtr<FJsonObject> JsonObject = RootJsonObject->GetObjectField(ThreadInfo.Name);
	for (TTuple<FString, TSharedPtr<FJsonValue>>& EventObject : JsonObject->Values)
	{
		if (!ParseChildren(EventObject.Key,
			EventObject.Value,
			ThreadInfo.SyntheticEvents,
			OutBudget,
			nullptr))
		{
			ThreadInfo.EventRootEnd = OutBudget.Num();
			UE_LOG(LogVSPPerfCollector, Error, TEXT("Json object %s parsing error"), *EventObject.Key);
			return false;
		}
	}
	ThreadInfo.EventRootEnd = OutBudget.Num();

	const TFunction<VSPPerfUtils::EWalkStatus(TSharedPtr<FVSPEventInfo>&)> FillFlatData =
		[ &ThreadInfo ](TSharedPtr<FVSPEventInfo>& Child)
		{
			ThreadInfo.FlatTotalEvents.Push(Child);
			return VSPPerfUtils::EWalkStatus::Continue;
		};
	for (int32 Id = ThreadInfo.EventRootStart; Id < ThreadInfo.EventRootEnd; ++Id)
		RecursiveBfsWalk(OutBudget[Id], &FVSPEventInfo::Children, FillFlatData);

	return true;
}

bool FVSPPerfEventConfig::ParseChildren(const FString& Name,
                                       const TSharedPtr<FJsonValue>& Value,
                                       TArray<TSharedPtr<FVSPSyntheticEvent>>& SyntheticEvents,
                                       TArray<TSharedPtr<FVSPEventInfo>>& OutEvents,
                                       const TSharedPtr<FVSPEventInfo>& Parent)
{
	using namespace VSPPerfEventConfig_Local;
	if (!Value)
	{
		UE_LOG(LogVSPPerfCollector, Error, TEXT("%s node is empty"), *Name);
		return false;
	}

	const TSharedPtr<FJsonObject> EventObject = Value->AsObject();
	if (!EventObject)
	{
		UE_LOG(LogVSPPerfCollector, Error, TEXT("%s node is not a JsonObject"), *Name);
		return false;
	}
	TSharedPtr<FVSPEventInfo> Event;
	const TFunction<VSPPerfUtils::EWalkStatus(TSharedPtr<FVSPEventInfo>&)> FindEventPred =
		[ &Name, &Event ](TSharedPtr<FVSPEventInfo>& Child)
		{
			if (Child.IsValid() && Child->Name == Name)
			{
				Event = Child;
				return VSPPerfUtils::EWalkStatus::Stop;
			}

			return VSPPerfUtils::EWalkStatus::Continue;
		};
	for (TSharedPtr<FVSPEventInfo>& RootEvent : OutEvents)
	{
		RecursiveBfsWalk(RootEvent, &FVSPEventInfo::Children, FindEventPred);
		if (Event)
			break;
	}

	if (!Event)
	{
		OutEvents.Push(MakeShared<FVSPEventInfo>(Name));
		Event = OutEvents.Last();
	}
	Event->Parent = Parent;

	if (EventObject->HasTypedField<EJson::String>(EventInfoNames.ConfluenceUrl))
		Event->ConfluenceUrl = EventObject->GetStringField(EventInfoNames.ConfluenceUrl);

	if (EventObject->HasTypedField<EJson::String>(EventInfoNames.Alias))
		Event->Alias = EventObject->GetStringField(EventInfoNames.Alias);

	if (EventObject->HasTypedField<EJson::Object>(EventInfoNames.Budget))
	{
		const TSharedPtr<FJsonObject> BudgetObject = EventObject->GetObjectField(EventInfoNames.Budget);
		if (BudgetObject->Values.Num() > 0)
		{
			const auto Iter = BudgetObject->Values.CreateIterator();
			const TSubclassOf<UVSPBudgetBase>* BudgetSubclass = BudgetTypes.Find(Iter.Key());
			if (!BudgetSubclass || !Event->CreateBudgetObject(*BudgetSubclass, Iter.Value()))
				UE_LOG(LogVSPPerfCollector, Warning, TEXT("Error in \"%s\" budget of %s metric"), *Iter.Key(), *Event->Name);
		}
	}
	else if (EventObject->HasTypedField<EJson::Number>(EventInfoNames.Budget))
	{
		if (!Event->CreateBudgetObject(UVSPBudgetBase::StaticClass(), EventObject->Values[EventInfoNames.Budget]))
			UE_LOG(LogVSPPerfCollector, Warning, TEXT("Error in \"%s\" budget of %s metric"), *EventInfoNames.Budget, *Event->Name);
	}

	TSharedPtr<IVSPSyntheticCalculator> SyntheticCalculator;
	for (TTuple<FString, TSharedPtr<FJsonValue>>& Field : EventObject->Values)
	{
		if (!Field.Value || Field.Value->Type != EJson::Object)
			continue;

		SyntheticCalculator = IVSPSyntheticCalculator::TryCreateFromName(Field.Key, Event);
		if (SyntheticCalculator)
		{
			if (SyntheticCalculator->SetupFromJson(Field.Value->AsObject()))
			{
				break;
			}

			SyntheticCalculator.Reset();
			UE_LOG(LogVSPPerfCollector, Warning, TEXT("Synthetic event `%s` has wrong data structure"), *Field.Key);
		}
	}

	if (SyntheticCalculator)
	{
		Event->bSyntheticEvent = true;
		SyntheticEvents.Push(MakeShared<FVSPSyntheticEvent>(Event));
		SyntheticEvents.Last()->SetCalculationStrategy(SyntheticCalculator);
	}

	if (EventObject->HasTypedField<EJson::Object>(EventInfoNames.Children))
	{
		const TSharedPtr<FJsonObject> ChildrenObject = EventObject->GetObjectField(EventInfoNames.Children);
		for (TTuple<FString, TSharedPtr<FJsonValue>>& Child : ChildrenObject->Values)
		{
			if (!ParseChildren(Child.Key, Child.Value, SyntheticEvents, Event->Children, Event))
			{
				UE_LOG(LogVSPPerfCollector, Warning, TEXT("Json object %s parsing error"), *Child.Key);
				return false;
			}
		}
	}

	return true;
}
