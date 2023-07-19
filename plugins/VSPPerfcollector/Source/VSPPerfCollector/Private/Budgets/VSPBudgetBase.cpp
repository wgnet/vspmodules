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
#include "Budgets/VSPBudgetBase.h"
#include "Templates/SubclassOf.h"

#include "VSPPerfEventConfig.h"

const double UVSPBudgetBase::InvalidBudget = -1;

UVSPBudgetBase::UVSPBudgetBase():
	Budget(InvalidBudget)
{

}

void UVSPBudgetBase::PostCDOContruct()
{
	if (!ConfigTag.IsEmpty())
		FVSPPerfEventConfig::AddBudgetType(ConfigTag, TSubclassOf<UVSPBudgetBase>(GetClass()));

	Super::PostCDOContruct();
}

bool UVSPBudgetBase::Init(const TSharedPtr<FJsonValue>& Json)
{
	if (Json->Type != EJson::Number)
		return false;

	Budget = Json->AsNumber();
	return true;
}

void UVSPBudgetBase::RequestUpdate()
{
	Budget = UpdateBudget();
}

double UVSPBudgetBase::UpdateBudget()
{
	return Budget;
}

void UVSPBudgetBase::SetConfigTag(const FString& NewTag)
{
	if (ConfigTag.IsEmpty())
		ConfigTag = NewTag;
}
