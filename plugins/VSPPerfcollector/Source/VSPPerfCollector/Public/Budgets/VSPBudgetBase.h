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

#include "VSPBudgetBase.generated.h"

/// Create rule for budget calculating when read from PerfConfig.json
/// Be careful, VSPPerfCollector didn't exists at Shipping
/// So all children of UVSPBudgetBase must be excluded from shipping build too
UCLASS()
class VSPPERFCOLLECTOR_API UVSPBudgetBase: public UObject
{
	GENERATED_BODY()
public:
	UVSPBudgetBase();

	static const double InvalidBudget;

	void PostCDOContruct() override;

	virtual bool Init(const TSharedPtr<class FJsonValue>& Json);
	void RequestUpdate();

	FORCEINLINE double GetBudget() const { return Budget; }

	/// Setup json field name for parsing
	void SetConfigTag(const FString& NewTag);


protected:
	virtual double UpdateBudget();


private:
	double Budget;
	FString ConfigTag;
};
