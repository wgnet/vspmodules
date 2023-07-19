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
#include "VSPThreadInfo.h"
#include "Dom/JsonValue.h"
#include "Templates/SubclassOf.h"


class UVSPBudgetBase;

class VSPPERFCOLLECTOR_API FVSPPerfEventConfig
{
public:
	FVSPPerfEventConfig() = default;

	bool InitFromJson(TSharedPtr<FJsonObject> JsonData);

	TArray<TSharedPtr<FVSPEventInfo>> AllEventRoots;
	FVSPThreadInfo GameThreadInfo{"GameThread", "FEngineLoop", TraceFrameType_Game};
	FVSPThreadInfo RenderThreadInfo{"RenderThread", "BeginFrame", TraceFrameType_Rendering};
	FVSPThreadInfo GpuThreadInfo{"GpuThread", "Unaccounted", TraceFrameType_Count};

	const TSharedPtr<FJsonObject>& GetJsonConfig() const { return JsonConfig; }

	static bool AddBudgetType(const FString& ConfigTag, TSubclassOf<UVSPBudgetBase> Type);

protected:
	bool ParseJsonData(const TSharedPtr<FJsonObject>& RootJsonObject,
	                   FVSPThreadInfo& ThreadInfo,
	                   TArray<TSharedPtr<FVSPEventInfo>>& OutBudget);
	bool ParseChildren(const FString& Name,
	                   const TSharedPtr<FJsonValue>& Value,
	                   TArray<TSharedPtr<FVSPSyntheticEvent>>& SyntheticEvents,
	                   TArray<TSharedPtr<FVSPEventInfo>>& OutEvents,
	                   const TSharedPtr<FVSPEventInfo>& Parent);

	TSharedPtr<FJsonObject> JsonConfig;

	static TMap<FString, TSubclassOf<UVSPBudgetBase>> BudgetTypes;
};
