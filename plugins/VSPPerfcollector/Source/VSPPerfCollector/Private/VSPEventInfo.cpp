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
#include "VSPEventInfo.h"

#include "Budgets/VSPBudgetBase.h"

FVSPEventInfo::FVSPEventInfo(const FString& Name):
	Name(Name),
	Value(0),
	bSyntheticEvent(false),
	TimerId(MAX_uint32),
	Budget(nullptr)
{
}

FVSPEventInfo::FVSPEventInfo(const FVSPEventInfo& Oth)
{
	DeepCopy(Oth, true);
}

FVSPEventInfo::~FVSPEventInfo()
{
	if (Budget)
		Budget->RemoveFromRoot();
}

bool FVSPEventInfo::operator==(const FVSPEventInfo& Rhs) const
{
	return Name == Rhs.Name;
}

double FVSPEventInfo::GetBudget() const
{
	if (Budget)
		return Budget->GetBudget();
	return -1;
}

void FVSPEventInfo::DeepCopy(const FVSPEventInfo& From, bool bReplaceChildren)
{
	Name = From.Name;
	Value = From.Value;
	bSyntheticEvent = From.bSyntheticEvent;
	Alias = From.Alias;
	Budget = From.Budget;
	TimerId = From.TimerId;

	if (bReplaceChildren)
	{
		Children.Empty(From.Children.Num());
		for (const TSharedPtr<FVSPEventInfo>& Event : From.Children)
			Children.Push(MakeShared<FVSPEventInfo>(*Event.Get()));
	}
	else
	{
		TArray<int32> ChildCounter;
		ChildCounter.SetNum(From.Children.Num());
		for (const TSharedPtr<FVSPEventInfo>& Event : From.Children)
		{
			if (!Event)
				continue;

			auto SearchPred = [Event](const TSharedPtr<FVSPEventInfo>& OthChild)
			{
				return *Event.Get() == *OthChild.Get();
			};

			if (const TSharedPtr<FVSPEventInfo>* Founded = Children.FindByPredicate(SearchPred))
			{
				ChildCounter.Push(Founded - &Children[0]);
				(*Founded)->DeepCopy(*Event.Get());
			}
			else
			{
				ChildCounter.Push(Children.Num());
				Children.Push(MakeShared<FVSPEventInfo>(*Event.Get()));
			}
		}
	}
}

const FString& FVSPEventInfo::GetDisplayName() const
{
	return Alias.IsEmpty() ? Name : Alias;
}

bool FVSPEventInfo::CreateBudgetObject(
	TSubclassOf<UVSPBudgetBase> BudgetClass,
	const TSharedPtr<FJsonValue>& JsonData)
{
	if (Budget)
		return false;

	Budget = NewObject<UVSPBudgetBase>((UObject*)GetTransientPackage(), BudgetClass);
	if (!Budget->Init(JsonData))
		return false;

	Budget->AddToRoot();
	UpdateBudget();

	return true;
}

void FVSPEventInfo::UpdateBudget() const
{
	if (Budget)
		Budget->RequestUpdate();
}
