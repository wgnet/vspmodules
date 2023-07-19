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
#include "Budgets/VSPTargetTypeBudget.h"

UVSPTargetTypeBudget::UVSPTargetTypeBudget():
	TargetBudget(InvalidBudget)
{
	SetConfigTag("Target");
}

bool UVSPTargetTypeBudget::Init(const TSharedPtr<FJsonValue>& Json)
{
	if (!Json && Json->Type != EJson::Object)
		return false;

	TSharedPtr<FJsonObject> JsonObject = Json->AsObject();

#if UE_SERVER
	if (JsonObject->TryGetNumberField("Server", TargetBudget))
#else
	if (JsonObject->TryGetNumberField("Client", TargetBudget))
#endif
		return true;

	return false;
}

double UVSPTargetTypeBudget::UpdateBudget()
{
	return TargetBudget;
}
