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
#include "VSPTraceAnalyzer.h"

#include "VSPPerfCollector.h"

FVSPDataAnalyzer::FVSPDataAnalyzer(Trace::IAnalysisSession& InSession, FVSPPerfCollectorModule& PerfCollector):
	Session(InSession),
	VSPPerfCollector(PerfCollector) { }

void FVSPDataAnalyzer::OnAnalysisBegin(const FOnAnalysisContext& Context)
{
	auto& Builder = Context.InterfaceBuilder;
	Builder.RouteEvent(RouteId_PerfMetadata, "VSPPerfProvider", "PerfMetadata");
}

bool FVSPDataAnalyzer::OnEvent(uint16 RouteId, EStyle Style, const FOnEventContext& Context)
{
	Trace::FAnalysisSessionEditScope AnalysisSessionEditScope(Session);
	const FEventData& EventData = Context.EventData;
	switch (RouteId)
	{
	case RouteId_PerfMetadata:
	{
		FString PerfConfig;
		if (!EventData.GetString("PerfConfig", PerfConfig))
		{
			UE_LOG(LogVSPPerfCollector, Log, TEXT("Couldn't read 'PerfConfig' field from trace"));
			break;
		}

		const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(PerfConfig);
		if (!VSPPerfCollector.UpdateConfig(JsonReader))
		{
			UE_LOG(LogVSPPerfCollector, Error, TEXT("Couldn't parse perf config"));
			UE_LOG(LogVSPPerfCollector, Verbose, TEXT("%s"), *PerfConfig);
			break;
		}

		VSPPerfCollector.Enable();
		UE_LOG(LogVSPPerfCollector, Log, TEXT("Perf config successfully loaded"));
		UE_LOG(LogVSPPerfCollector, Verbose, TEXT("%s"), *PerfConfig);
		break;
	}
	default:
		break;
	}

	return true;
}
