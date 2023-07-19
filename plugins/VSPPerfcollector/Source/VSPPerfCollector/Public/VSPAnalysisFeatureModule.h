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

#include "JsonDomBuilder.h"
#include "VSPEventInfo.h"
#include "TraceServices/ModuleService.h"
#include "TraceServices/Model/TimingProfiler.h"

struct FVSPThreadInfo;
struct FVSPEventInfo;

namespace Trace
{
	class ITimingProfilerTimerReader;
}

struct FVSPPerfInfo
{
	FString Name;
	double Duration;
	FString ParentName;
};

struct FVSPPerfFrame
{
	double StartTime;
	double EndTime;
	FString ThreadName;
	TArray<FVSPPerfInfo> Metrics;
};

struct FVSPTreeItem
{
	const FVSPPerfInfo* Self;
	TArray<FString> ChildrenNames;
};

class VSPPERFCOLLECTOR_API FVSPAnalysisFeatureModule : public Trace::IModule
{
public:
	using MetricJsonConstructor = TFunction<void(const FString& ThreadName, FJsonDomBuilder::FObject& Object, const FVSPTreeItem& Item)>;
	FVSPAnalysisFeatureModule(class FVSPPerfCollectorModule* VSPPerfCollector);
	virtual ~FVSPAnalysisFeatureModule() = default;

	void Disable();

	virtual void GetModuleInfo(Trace::FModuleInfo& OutModuleInfo) override;
	virtual void OnAnalysisBegin(Trace::IAnalysisSession& InSession) override;
	virtual void PostAnalysisBegin(Trace::IAnalysisSession& InSession) override;
	virtual void GetLoggers(TArray<const TCHAR*>& OutLoggers) override;
	virtual void GenerateReports(const Trace::IAnalysisSession& InSession, const TCHAR* CmdLine,
	                             const TCHAR* OutputDirectory) override;
	virtual const TCHAR* GetCommandLineArgument() override;

	void OnFrameEnded(TArray<TSharedPtr<FVSPEventInfo>>& AllEvents,
	                  FVSPThreadInfo& CurrentThread,
	                  double FrameStart,
	                  double FrameEnd);

	FJsonDomBuilder::FObject GetConfigJson() const;
	FJsonDomBuilder::FObject HeaderJson() const;
	static void MetricsToJson(const TArray<FVSPPerfFrame>& PerfData,
	                          const TFunction<void(const FVSPPerfFrame&, FJsonDomBuilder::FObject& Object)>& Callback,
	                          const MetricJsonConstructor& MetricConstructor = DefaultMetricJsonConstructor);
	void BookmarksToJson(const TFunction<void(const FJsonDomBuilder::FObject& Object)>& Callback) const;
	uint32 BulkWriteSize = 1000;

	Trace::IAnalysisSession* GetSession() const { return Session; }
	Trace::ITimingProfilerTimerReader const* GetTimerReader() const { return TimerReader; }

	void BindOnStartEvent(const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline, FVSPThreadInfo& Thread) const;
	void BindOnEndEvent(const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline, FVSPThreadInfo& Thread) const;

	/// Enumerate through FramePerformanceData until  Callback returns true
	void EnumerateFrameData(TFunction<bool(const FVSPPerfFrame& Frame)>&& Callback);

protected:
	void GpuTimelineBinding() const;
	void OnTimelineAddBinding(uint32 ThreadId, uint32 NewTimelineId) const;

	static FJsonDomBuilder::FObject BuildMetricTreeItem(const FString& ThreadName,
	                                                    const FVSPTreeItem& Item,
	                                                    const TMap<FString, FVSPTreeItem>& Cache,
	                                                    const MetricJsonConstructor& MetricConstructor);

	static void DefaultMetricJsonConstructor(const FString& ThreadName,
	                                         FJsonDomBuilder::FObject& Object,
	                                         const FVSPTreeItem& Item);

private:
	FVSPPerfCollectorModule* VSPPerfCollector{};

	Trace::IAnalysisSession* Session{};
	const Trace::ITimingProfilerProvider* TimingProvider{};
	Trace::ITimingProfilerTimerReader const* TimerReader{};
	
	FDelegateHandle TimelineHandler;

	uint64 TotalMetricFramesCounter{};
	TArray<FVSPPerfFrame> PerfData;
};
