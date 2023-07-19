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
#include "VSPAnalysisFeatureModule.h"

#include "HttpModule.h"
#include "VSPPerfCollector.h"
#include "VSPPerfCollectorSettings.h"
#include "VSPPerfUtils.h"
#include "VSPThreadInfo.h"
#include "VSPTraceAnalyzer.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"
#include "TraceServices/Model/AnalysisSession.h"
#include "TraceServices/Model/Bookmarks.h"
#include "TraceServices/Model/Threads.h"
#include "TraceServices/Model/TimingProfiler.h"

FVSPAnalysisFeatureModule::FVSPAnalysisFeatureModule(FVSPPerfCollectorModule* InVSPPerfCollector):
	VSPPerfCollector(InVSPPerfCollector)
{
}

void FVSPAnalysisFeatureModule::Disable()
{
	if (!Session)
		return;

	Trace::FAnalysisSessionReadScope SessionReadScope(*Session);
	TimingProvider->OnTimelineAdded.Remove(TimelineHandler);
	TimingProvider->EnumerateTimelines([](const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline)
	{
		Timeline.OnEventStarted.Unbind();
		Timeline.OnEventEnded.Unbind();
	});
	Session = nullptr;
}

void FVSPAnalysisFeatureModule::GetModuleInfo(Trace::FModuleInfo& OutModuleInfo)
{
	OutModuleInfo.Name = "VSPPerfCollector";
	OutModuleInfo.DisplayName = TEXT("VSPPerfCollector");
}

void FVSPAnalysisFeatureModule::OnAnalysisBegin(Trace::IAnalysisSession& InSession)
{
	InSession.AddAnalyzer(new FVSPDataAnalyzer(InSession, *VSPPerfCollector));
}

void FVSPAnalysisFeatureModule::PostAnalysisBegin(Trace::IAnalysisSession& InSession)
{
	Session = &InSession;
	TimingProvider = ReadTimingProfilerProvider(*Session);

	if (!TimingProvider)
		return;

	TimingProvider->ReadTimers([ this ](const Trace::ITimingProfilerTimerReader& Out) { TimerReader = &Out; });
	
	GpuTimelineBinding();
	TimelineHandler = TimingProvider->OnTimelineAdded.AddRaw(this, &FVSPAnalysisFeatureModule::OnTimelineAddBinding);
	VSPPerfCollector->OnStatCollectEnd.AddRaw(this, &FVSPAnalysisFeatureModule::OnFrameEnded);
}

void FVSPAnalysisFeatureModule::GetLoggers(TArray<const TCHAR*>& OutLoggers)
{
}

void FVSPAnalysisFeatureModule::GenerateReports(const Trace::IAnalysisSession& InSession,
                                               const TCHAR* CmdLine,
                                               const TCHAR* OutputDirectory)
{
	const FString DumpFilepath = FString(OutputDirectory) / TEXT("VSPPerfResults.json");
	const TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*DumpFilepath));
	if (!Ar)
		return;

	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(Ar.Get());
	FJsonDomBuilder::FObject Root;
	Root.Set("Header", HeaderJson());
	Root.Set("PerfConfig", GetConfigJson());

	FJsonDomBuilder::FArray Bookmarks;
	BookmarksToJson([ &Bookmarks ](const FJsonDomBuilder::FObject& Object)
	{
		Bookmarks.Add(Object);
	});
	Root.Set("Bookmarks", Bookmarks);

	FJsonDomBuilder::FArray Metrics;
	MetricsToJson(PerfData, [ &Metrics ](const FVSPPerfFrame&, const FJsonDomBuilder::FObject& Object)
	{
		Metrics.Add(Object);
	});
	Root.Set("Metrics", Metrics);
	
	FJsonSerializer::Serialize(Root.AsJsonObject(), Writer);
}

const TCHAR* FVSPAnalysisFeatureModule::GetCommandLineArgument()
{
	return TEXT("vspperfcollector");
}

void FVSPAnalysisFeatureModule::OnFrameEnded(TArray<TSharedPtr<FVSPEventInfo>>& AllEvents,
                                            FVSPThreadInfo& CurrentThread,
                                            double FrameStart,
                                            double FrameEnd)
{
	PerfData.Push({FrameStart * 1000.f, FrameEnd * 1000.f, CurrentThread.Name}); // From seconds to milliseconds
	++TotalMetricFramesCounter;

	for (TWeakPtr<FVSPEventInfo>& WeakChild : CurrentThread.FlatTotalEvents)
	{
		if (const TSharedPtr<FVSPEventInfo> Child = WeakChild.Pin())
		{
			PerfData.Last().Metrics.Push(
				{
					Child->GetDisplayName(),
					Child->Value,
					Child->Parent ? Child->Parent->GetDisplayName() : ""
				});
		}
	}
}

FJsonDomBuilder::FObject ProcessConfigEvents(const FVSPEventInfo& Event)
{
	FJsonDomBuilder::FObject EventJson;

	const TCHAR* BudgetFieldName = GET_MEMBER_NAME_STRING_CHECKED(FVSPEventInfo, Value);
	const TCHAR* ConfluenceUrl = GET_MEMBER_NAME_STRING_CHECKED(FVSPEventInfo, ConfluenceUrl);
	// TODO: Update code after preset will works
	if (Event.Parent && Event.Parent->bSyntheticEvent)
	{
		EventJson.Set(BudgetFieldName, Event.Parent->GetBudget());
		EventJson.Set(ConfluenceUrl, Event.Parent->ConfluenceUrl);
	}
	else
	{
		EventJson.Set(BudgetFieldName, Event.GetBudget());
		EventJson.Set(ConfluenceUrl, Event.ConfluenceUrl);
	}

	if (Event.Children.Num())
	{
		FJsonDomBuilder::FObject ChildrenJson;
		for (const TSharedPtr<FVSPEventInfo>& Child : Event.Children)
		{
			if (Child)
				ChildrenJson.Set(Child->GetDisplayName(), ProcessConfigEvents(*Child));
		}
		EventJson.Set(GET_MEMBER_NAME_STRING_CHECKED(FVSPEventInfo, Children), ChildrenJson);
	}

	return EventJson;
}

FJsonDomBuilder::FObject FVSPAnalysisFeatureModule::GetConfigJson() const
{
	if (!ensure(VSPPerfCollector && VSPPerfCollector->GetAllEvents()))
		return {};

	const FVSPPerfEventConfig& Config = VSPPerfCollector->GetConfig();
	FJsonDomBuilder::FObject ConfigJson;

	TArray<const FVSPThreadInfo*> Threads{
		&Config.GameThreadInfo,
		&Config.RenderThreadInfo,
		&Config.GpuThreadInfo
	};
	for (const FVSPThreadInfo* Thread : Threads)
	{
		FJsonDomBuilder::FObject Events;
		const TArray<TSharedPtr<FVSPEventInfo>>& RootEvents = *VSPPerfCollector->GetAllEvents();
		for (int32 It = Thread->EventRootStart; It < Thread->EventRootEnd; ++It)
		{
			if (RootEvents[It])
				Events.Set(RootEvents[It]->GetDisplayName(), ProcessConfigEvents(*RootEvents[It]));
		}
		ConfigJson.Set(Thread->Name, Events);
	}

	return ConfigJson;
}

FJsonDomBuilder::FObject FVSPAnalysisFeatureModule::HeaderJson() const
{
	FJsonDomBuilder::FObject Header;

	Header.Set("MetricFramesCount", TotalMetricFramesCounter);

	return Header;
}

FJsonDomBuilder::FObject FVSPAnalysisFeatureModule::BuildMetricTreeItem(
	const FString& ThreadName,
	const FVSPTreeItem& Item,
	const TMap<FString, FVSPTreeItem>& Cache,
	const MetricJsonConstructor& MetricConstructor)
{
	FJsonDomBuilder::FObject MetricJson;

	MetricConstructor(ThreadName, MetricJson, Item);

	if (Item.ChildrenNames.Num())
	{
		FJsonDomBuilder::FObject ChildrenJson;
		for (const FString& ChildName : Item.ChildrenNames)
		{
			if (const FVSPTreeItem* Child = Cache.Find(ChildName))
				ChildrenJson.Set(Child->Self->Name, BuildMetricTreeItem(ThreadName, *Child, Cache, MetricConstructor));
			else
				UE_LOG(LogVSPPerfCollector, Error, TEXT("BuildMetricTreeItem %s"), *Item.Self->Name);
		}
		MetricJson.Set("Children", ChildrenJson);
	}

	return MetricJson;
}

void FVSPAnalysisFeatureModule::DefaultMetricJsonConstructor(const FString& ThreadName,
                                                            FJsonDomBuilder::FObject& MetricJson,
                                                            const FVSPTreeItem& Item)
{
	MetricJson.Set("Value", Item.Self->Duration);
}

void FVSPAnalysisFeatureModule::MetricsToJson(const TArray<FVSPPerfFrame>& PerfData,
                                             const TFunction<void(const FVSPPerfFrame&, FJsonDomBuilder::FObject& Object)>& Callback,
                                             const MetricJsonConstructor& MetricConstructor)
{
	for (int32 Id = 0; Id < PerfData.Num(); ++Id)
	{
		FJsonDomBuilder::FObject Record;
		Record.Set("FrameStart", PerfData[Id].StartTime);
		Record.Set("FrameEnd", PerfData[Id].EndTime);

		FJsonDomBuilder::FObject Data;
		TMap<FString, FVSPTreeItem> TreeView;
		TSet<const FVSPPerfInfo*> Roots;
		for (const FVSPPerfInfo& Item : PerfData[Id].Metrics)
		{
			if (Item.ParentName.IsEmpty())
			{
				TreeView.Add(Item.Name, { &Item });
				Roots.Add(&Item);
				continue;
			}

			FVSPTreeItem* TreeItem = TreeView.Find(Item.ParentName);
			// Metrics is array and all events unwrap from root of tree, so all parents will be added before children
			if (!ensure(TreeItem))
			{
				UE_LOG(LogVSPPerfCollector, Error, TEXT("%s : %s"), *Item.Name, *Item.ParentName);
				continue;
			}

			TreeView.Add(Item.Name, {&Item});
			TreeItem->ChildrenNames.Add(Item.Name);
		}

		for (const FVSPPerfInfo* Item : Roots)
		{
			const FVSPTreeItem* TreeNode = TreeView.Find(Item->Name);
			// All items from Roots added to TreeView couple lines above
			if (!ensure(TreeNode))
			{
				UE_LOG(LogVSPPerfCollector, Error, TEXT("Root %s not found"), *Item->Name);
				continue;
			}
			Data.Set(Item->Name, BuildMetricTreeItem(PerfData[Id].ThreadName,*TreeNode, TreeView, MetricConstructor));
		}

		Record.Set(PerfData[Id].ThreadName, Data.AsJsonValue());
		Callback(PerfData[Id], Record);
	}
}

void FVSPAnalysisFeatureModule::BookmarksToJson(
	const TFunction<void(const FJsonDomBuilder::FObject& Object)>& Callback) const
{
	Trace::FAnalysisSessionReadScope SessionReadScope(*Session);
	const Trace::IBookmarkProvider* BookmarkProvider = &ReadBookmarkProvider(*Session);
	BookmarkProvider->EnumerateBookmarks(0,
		DBL_MAX,
		[ &Callback ](const Trace::FBookmark& Bookmark)
		{
			FJsonDomBuilder::FObject Object;
			Object.Set(Bookmark.Text, Bookmark.Time * 1000.f); // Bookmarks come in seconds -> make ms
			Callback(Object);
		});
}

void FVSPAnalysisFeatureModule::BindOnStartEvent(const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline,
                                                FVSPThreadInfo& Thread) const
{
	Timeline.OnEventStarted.BindLambda([ this, &Thread ](double Time, Trace::FTimingProfilerEvent Event)
	{
		VSPPerfCollector->OnEventStarted(Thread, Time, Event, TimerReader);
	});
}

void FVSPAnalysisFeatureModule::BindOnEndEvent(const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline,
                                              FVSPThreadInfo& Thread) const
{
	Timeline.OnEventEnded.BindLambda([ this, &Thread ](double Time)
	{
		VSPPerfCollector->OnEventEnded(Thread,
			Time,
			Thread.LastEventName,
			TimerReader);
	});
}

void FVSPAnalysisFeatureModule::EnumerateFrameData(TFunction<bool(const FVSPPerfFrame& Frame)>&& Callback)
{
	for (FVSPPerfFrame& Frame : PerfData)
	{
		if (!Callback(Frame))
			break;
	}
}

void FVSPAnalysisFeatureModule::GpuTimelineBinding() const
{
	if (!TimingProvider)
		return;
	
	uint32 GpuTimelineIndex;
	TimingProvider->GetGpuTimelineIndex(GpuTimelineIndex);
	TimingProvider->ReadTimeline(GpuTimelineIndex,
		[ this ](const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline)
		{
			BindOnStartEvent(Timeline, *VSPPerfCollector->GetGpuThreadInfo());
			BindOnEndEvent(Timeline, *VSPPerfCollector->GetGpuThreadInfo());
		});
	TimingProvider->GetGpu2TimelineIndex(GpuTimelineIndex);
	TimingProvider->ReadTimeline(GpuTimelineIndex,
		[ this ](const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline)
		{
			BindOnStartEvent(Timeline, *VSPPerfCollector->GetGpuThreadInfo());
			BindOnEndEvent(Timeline, *VSPPerfCollector->GetGpuThreadInfo());
		});
}

void FVSPAnalysisFeatureModule::OnTimelineAddBinding(uint32 ThreadId, uint32 NewTimelineId) const
{
	if (Session->IsAnalysisComplete() || !TimingProvider)
		return;

	const Trace::IThreadProvider* ThreadProvider = &Trace::ReadThreadProvider(*Session);
	const TCHAR* ThreadName = ThreadProvider->GetThreadName(ThreadId);
	if (VSPPerfCollector->GetGameThreadInfo()->LocalThreadId == MAX_uint32 &&
		FCString::Strncmp(ThreadName, *VSPPerfCollector->GetGameThreadInfo()->Name,
			VSPPerfCollector->GetGameThreadInfo()->Name.Len()) == 0)
	{
		VSPPerfCollector->GetGameThreadInfo()->LocalThreadId = ThreadId;
		TimingProvider->ReadTimeline(NewTimelineId,
			[ this ](const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline)
			{
				BindOnStartEvent(Timeline, *VSPPerfCollector->GetGameThreadInfo());
				BindOnEndEvent(Timeline, *VSPPerfCollector->GetGameThreadInfo());
			});
	}
	else if (FCString::Strncmp(ThreadName,
			*VSPPerfCollector->GetRenderThreadInfo()->Name,
			VSPPerfCollector->GetRenderThreadInfo()->Name.Len()) == 0)
	{
		uint32 TimelineId;
		if (VSPPerfCollector->GetRenderThreadInfo()->LocalThreadId != MAX_uint32 &&
			TimingProvider->GetCpuThreadTimelineIndex(VSPPerfCollector->GetRenderThreadInfo()->LocalThreadId,
				TimelineId))
		{
			TimingProvider->ReadTimeline(TimelineId,
				[](const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline)
				{
					Timeline.OnEventStarted.Unbind();
					Timeline.OnEventEnded.Unbind();
				});
		}

		VSPPerfCollector->GetRenderThreadInfo()->LocalThreadId = ThreadId;
		TimingProvider->ReadTimeline(NewTimelineId,
			[ this ](const Trace::ITimeline<Trace::FTimingProfilerEvent>& Timeline)
			{
				BindOnStartEvent(Timeline, *VSPPerfCollector->GetRenderThreadInfo());
				BindOnEndEvent(Timeline, *VSPPerfCollector->GetRenderThreadInfo());
			});
	}
}
