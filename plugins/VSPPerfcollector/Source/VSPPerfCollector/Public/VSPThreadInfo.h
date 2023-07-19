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

#include "VSPSyntheticEvent.h"
#include "ProfilingDebugging/MiscTrace.h"
#include "Templates/SharedPointer.h"

// Info about thread inside PerfCollectorModule, contains evnet range(EventRootStart/End) for owning collecting events
struct VSPPERFCOLLECTOR_API FVSPThreadInfo
{
	static constexpr int32 EventStackInitialSize = 1024;
	static constexpr int32 FrameChildrenStorageInitialSize = 65536;
	
	FVSPThreadInfo(const FString& Name,
	              const FString& LastEventName,
	              ETraceFrameType Type):
		Name(Name),
		LastEventName(LastEventName),
		Type(Type)
	{
		EventStack.Reserve(EventStackInitialSize);
		FrameChildrenStorage.Reserve(FrameChildrenStorageInitialSize);
	}

	FString Name;

	// metric's name that means end of frame("BeginFrame" for RenderThread, "FEngineLoog" for GameThread, etc)
	FString LastEventName;
	ETraceFrameType Type;
	int32 EventRootStart = 0;
	int32 EventRootEnd = 0;
	uint32 LocalThreadId = MAX_uint32;
	float CleanupTimer = 0.f;

	TArray<TSharedPtr<FVSPSyntheticEvent>> SyntheticEvents;
	// Stack of processing metrics, fill up in runtime
	TArray<FVSPTimerInfo> EventStack;
	// Cached storage for Event's child metrics
	TArray<FVSPTimerInfo> FrameChildrenStorage;
	
	TArray<TWeakPtr<FVSPEventInfo>> FlatTotalEvents;
};
