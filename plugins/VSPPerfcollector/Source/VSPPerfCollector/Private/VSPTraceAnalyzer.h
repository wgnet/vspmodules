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
#include "TraceServices/Model/AnalysisSession.h"
#include "Trace/Analyzer.h"

// Custom analyzer for receiving metadata through .utrace
class FVSPDataAnalyzer : public Trace::IAnalyzer
{
	enum : uint16
	{
		RouteId_PerfMetadata,
	};

public:
	FVSPDataAnalyzer(Trace::IAnalysisSession& InSession, class FVSPPerfCollectorModule& PerfCollector);

	virtual void OnAnalysisBegin(const FOnAnalysisContext& Context) override;
	virtual bool OnEvent(uint16 RouteId, EStyle Style, const FOnEventContext& Context) override;

private:
	Trace::IAnalysisSession& Session;
	FVSPPerfCollectorModule& VSPPerfCollector;
};
