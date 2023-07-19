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

#include "Templates/SharedPointer.h"
#include "Templates/SubclassOf.h"

class UVSPBudgetBase;

struct VSPPERFCOLLECTOR_API FVSPEventInfo
{
	FVSPEventInfo(const FString& Name);
	FVSPEventInfo(const FVSPEventInfo& Oth);
	~FVSPEventInfo();

	FString Name;

	FString Alias;

	TSharedPtr<FVSPEventInfo> Parent;

	FString ConfluenceUrl;

	double Value;

	bool bSyntheticEvent;

	uint32 TimerId;

	TArray<TSharedPtr<FVSPEventInfo>> Children;

	bool operator==(const FVSPEventInfo& Rhs) const;
	void DeepCopy(const FVSPEventInfo& From, bool bReplaceChildren = false);
	const FString& GetDisplayName() const;

	double GetBudget() const;
	bool CreateBudgetObject(TSubclassOf<UVSPBudgetBase> BudgetClass, const TSharedPtr<class FJsonValue>& JsonData);
	void UpdateBudget() const;

private:
	// Need to use raw UObject pointer with attachment to Root, because TStrongObjectPtr works only on GameThread
	// But this object creates on TraceAnalysis thread
	UVSPBudgetBase* Budget;
};
