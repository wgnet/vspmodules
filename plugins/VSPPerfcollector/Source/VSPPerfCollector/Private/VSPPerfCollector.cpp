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
#include "VSPPerfCollector.h"

#include "Budgets/VSPBudgetBase.h"
#include "Budgets/VSPTargetTypeBudget.h"
#include "Features/IModularFeatures.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Trace/Analyzer.h"
#include "Trace/Detail/EventNode.h"
#include "Trace/Detail/LogScope.h"
#include "Trace/Trace.inl"
#include "TraceServices/ModuleService.h"
#include "VSPEventInfo.h"
#include "VSPPerfCollectorSettings.h"
#include "VSPPerfTrace.h"
#include "VSPSyntheticEvent.h"

#define LOCTEXT_NAMESPACE "FVSPPerfCollectorModule"

DEFINE_LOG_CATEGORY(LogVSPPerfCollector);

UE_TRACE_EVENT_BEGIN(VSPPerfProvider, PerfMetadata)
UE_TRACE_EVENT_FIELD(Trace::AnsiString, PerfConfig)
UE_TRACE_EVENT_END()

FVSPPerfCollectorModule::FVSPPerfCollectorModule():
	VSPAnalysisFeatureModule(this),
	VSPHeatmapCollectorModule(VSPAnalysisFeatureModule)
{
}

void FVSPPerfCollectorModule::PreUnloadCallback()
{
	Disable();
	VSPAnalysisFeatureModule.Disable();
}

void FVSPPerfCollectorModule::StartupModule()
{
#if UE_TRACE_ENABLED
	{
		FString JsonFileContents;
		if (FFileHelper::LoadFileToString(JsonFileContents, *UVSPPerfCollectorSettings::FullConfigPath()))
		{
			const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonFileContents);
			const TOptional<FVSPPerfEventConfig> Config = ParseConfigFrom(JsonReader);
			if (!Config.IsSet())
			{
				UE_LOG(LogVSPPerfCollector,
					Error,
					TEXT("Coudln't parse perf config from %s"),
					*UVSPPerfCollectorSettings::FullConfigPath());
			}
			else if (UE_TRACE_CHANNELEXPR_IS_ENABLED(VSPUEChannel))
			{
				UE_TRACE_LOG(VSPPerfProvider, PerfMetadata, VSPUEChannel)
					<< PerfMetadata.PerfConfig(*JsonFileContents);
			}
		}
		else
		{
			UE_LOG(LogVSPPerfCollector,
				Error,
				TEXT("Couldn't read perf config from %s"),
				*UVSPPerfCollectorSettings::FullConfigPath());
		}
	}
#endif
#if IS_PROGRAM // For Insights
	{
		constexpr int32 ReturnCode = 1;
		FString JsonFileContents;
		if (!FFileHelper::LoadFileToString(JsonFileContents, *UVSPPerfCollectorSettings::FullConfigPath()))
		{
			UE_LOG(LogVSPPerfCollector,
				Error,
				TEXT("Default config not found at %s"),
				*UVSPPerfCollectorSettings::FullConfigPath());
			FPlatformMisc::RequestExitWithStatus(true, ReturnCode);
			return;
		}

		if (!UpdateConfig(TJsonReaderFactory<>::Create(JsonFileContents)))
		{
			UE_LOG(LogVSPPerfCollector,
				Error,
				TEXT("Couldn't parse default config from %s"),
				*UVSPPerfCollectorSettings::FullConfigPath());
			FPlatformMisc::RequestExitWithStatus(true, ReturnCode);
			return;
		}
		
		Enable();
		UE_LOG(LogVSPPerfCollector,
			Log,
			TEXT("Config successfully loaded from %s"),
			*UVSPPerfCollectorSettings::FullConfigPath());
	}
#endif

	// Sending to Exportana goes in reverse order
	IModularFeatures::Get().RegisterModularFeature(Trace::ModuleFeatureName, &VSPHeatmapCollectorModule);
	UE_LOG(LogVSPPerfCollector, Log, TEXT("HeatmapCollector enabled"));

	IModularFeatures::Get().RegisterModularFeature(Trace::ModuleFeatureName, &VSPAnalysisFeatureModule);
	UE_LOG(LogVSPPerfCollector, Log, TEXT("VSPPerfCollector started"));
}

void FVSPPerfCollectorModule::Enable()
{
	if (!bReadyForWork)
		return;
	
	bWorks = true;
	UE_LOG(LogVSPPerfCollector, Log, TEXT("VSPPerfCollector enabled"));
}

void FVSPPerfCollectorModule::Disable()
{
	bWorks = false;
	UE_LOG(LogVSPPerfCollector, Log, TEXT("VSPPerfCollector disabled"));
}

bool FVSPPerfCollectorModule::UpdateConfig(const TSharedRef<TJsonReader<>> JsonReader)
{
	TOptional<FVSPPerfEventConfig> NewConfig = ParseConfigFrom(JsonReader);
	if (!NewConfig.IsSet())
		return false;

	const bool bDidWork = bWorks;
	bReadyForWork = false;

	Disable();
	EventsConfig = NewConfig.GetValue();

	bReadyForWork = true;

	if (bDidWork)
		Enable();

	return true;
}


void FVSPPerfCollectorModule::UpdateDefaultConfig()
{
	FString JsonFileContents;
	if (!FFileHelper::LoadFileToString(JsonFileContents, *UVSPPerfCollectorSettings::FullConfigPath()))
	{
		UE_LOG(
			LogVSPPerfCollector,
			Error,
			TEXT("Couldn't read perf config from %s"),
			*UVSPPerfCollectorSettings::FullConfigPath());
		return;
	}

	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonFileContents);
	const TOptional<FVSPPerfEventConfig> Config = ParseConfigFrom(JsonReader);
	if (!Config.IsSet())
	{
		UE_LOG(
			LogVSPPerfCollector,
			Error,
			TEXT("Coudln't parse perf config from %s"),
			*UVSPPerfCollectorSettings::FullConfigPath());
		return;
	}

	if (!UpdateConfig(TJsonReaderFactory<>::Create(JsonFileContents)))
	{
		UE_LOG(
			LogVSPPerfCollector,
			Error,
			TEXT("Coudln't update perf config from %s"),
			*UVSPPerfCollectorSettings::FullConfigPath());
	}
}

TOptional<FVSPPerfEventConfig> FVSPPerfCollectorModule::ParseConfigFrom(const TSharedRef<TJsonReader<>> JsonReader) const
{
	TSharedPtr<FJsonObject> JsonData;
	if (!FJsonSerializer::Deserialize(JsonReader, JsonData))
		return TOptional<FVSPPerfEventConfig>();

	FVSPPerfEventConfig NewConfig;
	if (!NewConfig.InitFromJson(JsonData))
		return TOptional<FVSPPerfEventConfig>();

	return TOptional<FVSPPerfEventConfig>(NewConfig);
}

void FVSPPerfCollectorModule::OnEventStarted(FVSPThreadInfo& ThreadInfo,
                                            double TimeSeconds,
                                            Trace::FTimingProfilerEvent Event,
                                            Trace::ITimingProfilerTimerReader const* TimerReader) const
{
	if (!bWorks || !TimerReader || VSPAnalysisFeatureModule.GetSession()->IsAnalysisComplete())
		return;
	
	constexpr double UnknownDuration = -1;

	
	ThreadInfo.EventStack.Push({Event.TimerIndex, TimeSeconds, UnknownDuration, FString()});
	if (const Trace::FTimingProfilerTimer* Timer = TimerReader->GetTimer(ThreadInfo.EventStack.Last().Id))
	{
		ThreadInfo.EventStack.Last().Name = Timer->Name;
		ThreadInfo.EventStack.Last().Metadata = TimerReader->GetMetadata(ThreadInfo.EventStack.Last().Id);
	}
}

void FVSPPerfCollectorModule::OnEventEnded(FVSPThreadInfo& ThreadInfo,
                                          double TimeSeconds,
                                          const FString& FinalEventName,
                                          Trace::ITimingProfilerTimerReader const* TimerReader)
{
	if (ThreadInfo.EventStack.Num() == 0 || !TimerReader)
		return;

	if (!bWorks)
	{
		ThreadInfo.EventStack.SetNum(0);
		return;
	}

	FVSPTimerInfo& TimerInfo = ThreadInfo.EventStack.Last();
	TimerInfo.Duration = (TimeSeconds - TimerInfo.StartTimeSeconds) * 1000.0;
	// Convert insights default seconds timing into milliseconds

	if (const Trace::FTimingProfilerTimer* Timer = TimerReader->GetTimer(TimerInfo.Id))
	{
		for (TWeakPtr<FVSPEventInfo>& WeakChild : ThreadInfo.FlatTotalEvents)
		{
			if (const TSharedPtr<FVSPEventInfo> Child = WeakChild.Pin())
			{
				if (Child->TimerId == MAX_uint32 && Child->Name == Timer->Name)
				{
					Child->TimerId = Timer->Id;
					Child->Value += TimerInfo.Duration;
				}
				else if (Child->TimerId == Timer->Id)
				{
					Child->Value += TimerInfo.Duration;
				}
			}
		}
		for (const TSharedPtr<FVSPSyntheticEvent>& SyntheticEvent : ThreadInfo.SyntheticEvents)
			SyntheticEvent->ProcessTimerStack(ThreadInfo.EventStack, ThreadInfo);

		if (ThreadInfo.EventStack.Num() > 1)
		{
			ThreadInfo.EventStack.Last(1).ChildIds.Push(ThreadInfo.FrameChildrenStorage.Num());
			ThreadInfo.FrameChildrenStorage.Push(TimerInfo);
		}

		if (ThreadInfo.EventStack.Num() > 0)
		{
			ThreadInfo.EventStack.RemoveAt(ThreadInfo.EventStack.Num() - 1, 1, false);
		}

		if (ThreadInfo.EventStack.Num() == 0 && Timer->Name == FinalEventName)
		{
			for (const TSharedPtr<FVSPSyntheticEvent>& SyntheticEvent : ThreadInfo.SyntheticEvents)
				SyntheticEvent->UpdateValue();
			OnDataUpdated(EventsConfig.AllEventRoots, ThreadInfo, TimerInfo.StartTimeSeconds, TimeSeconds);
#if !IS_PROGRAM
			ThreadInfo.CleanupTimer += TimerInfo.Duration;
			if (ThreadInfo.CleanupTimer >= UVSPPerfCollectorSettings::Get().CleanupEventsTimeMs)
			{
				for (const TSharedPtr<FVSPSyntheticEvent>& SyntheticEvent : ThreadInfo.SyntheticEvents)
					SyntheticEvent->PendingCleanup();

				ThreadInfo.CleanupTimer = 0;
			}
#endif
			for (TWeakPtr<FVSPEventInfo>& WeakChild : ThreadInfo.FlatTotalEvents)
			{
				if (const TSharedPtr<FVSPEventInfo> Child = WeakChild.Pin())
					Child->Value = 0;
			}
			ThreadInfo.FrameChildrenStorage.Empty(ThreadInfo.FrameChildrenStorage.Num());
		}
	}
}

void FVSPPerfCollectorModule::OnDataUpdated(TArray<TSharedPtr<FVSPEventInfo>>& AllEvents,
                                           FVSPThreadInfo& Info,
                                           double FrameStartSeconds,
                                           double FrameEndSeconds) const
{
	if (bWorks)
		OnStatCollectEnd.Broadcast(AllEvents, Info, FrameStartSeconds, FrameEndSeconds);
}

void FVSPPerfCollectorModule::ShutdownModule()
{
	OnStatCollectEnd.RemoveAll(this);
	UE_LOG(LogVSPPerfCollector, Log, TEXT("VSPPerfCollector shutdown"));
}
#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FVSPPerfCollectorModule, VSPPerfCollector)
