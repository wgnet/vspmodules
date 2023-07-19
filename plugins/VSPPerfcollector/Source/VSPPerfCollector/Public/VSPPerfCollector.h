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

#include "CoreMinimal.h"
#include "VSPAnalysisFeatureModule.h"
#include "Heatmap/VSPHeatmapCollectorModule.h"
#include "Modules/ModuleInterface.h"
#include "VSPPerfEventConfig.h"
#include "VSPThreadInfo.h"
#include "TraceServices/Model/TimingProfiler.h"

DECLARE_LOG_CATEGORY_EXTERN(LogVSPPerfCollector, Log, All);

class VSPPERFCOLLECTOR_API FVSPPerfCollectorModule : public IModuleInterface
{
public:
	FVSPPerfCollectorModule();
	virtual void PreUnloadCallback() override;
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	DECLARE_MULTICAST_DELEGATE_FourParams(FOnStatCollectEnd,
		TArray<TSharedPtr<FVSPEventInfo>>&,
		FVSPThreadInfo&,
		double,
		double)

	FOnStatCollectEnd OnStatCollectEnd;

	void OnEventStarted(FVSPThreadInfo& ThreadInfo,
	                    double TimeSeconds,
	                    Trace::FTimingProfilerEvent Event,
	                    Trace::ITimingProfilerTimerReader const* TimerReader) const;
	void OnEventEnded(FVSPThreadInfo& ThreadInfo,
	                  double TimeSeconds,
	                  const FString& FinalEventName,
	                  Trace::ITimingProfilerTimerReader const* TimerReader);

	FORCEINLINE TArray<TSharedPtr<FVSPEventInfo>>* GetAllEvents() { return &EventsConfig.AllEventRoots; }
	FORCEINLINE FVSPThreadInfo* GetGameThreadInfo() { return &EventsConfig.GameThreadInfo; }
	FORCEINLINE FVSPThreadInfo* GetRenderThreadInfo() { return &EventsConfig.RenderThreadInfo; }
	FORCEINLINE FVSPThreadInfo* GetGpuThreadInfo() { return &EventsConfig.GpuThreadInfo; }

	FORCEINLINE const FVSPPerfEventConfig& GetConfig() const { return EventsConfig; }
	
	void Enable();
	void Disable();
	void UpdateDefaultConfig();
	bool UpdateConfig(const TSharedRef<TJsonReader<>> JsonReader);
	TOptional<FVSPPerfEventConfig> ParseConfigFrom(const TSharedRef<TJsonReader<>> JsonReader) const;

	bool IsEnabled() const { return bWorks; }
	bool IsReadyForWork() const { return bReadyForWork; }

	FVSPAnalysisFeatureModule& GetAnalysisModule() { return VSPAnalysisFeatureModule; }
	FVSPHeatmapCollectorModule& GetHeatmapCollector() { return VSPHeatmapCollectorModule; }

protected:
	void OnDataUpdated(TArray<TSharedPtr<FVSPEventInfo>>& AllEvents,
	                   FVSPThreadInfo& Info,
	                   double FrameStartSeconds,
	                   double FrameEndSeconds) const;


private:
	FVSPAnalysisFeatureModule VSPAnalysisFeatureModule;
	FVSPHeatmapCollectorModule VSPHeatmapCollectorModule;

	bool bWorks = false;
	FVSPPerfEventConfig EventsConfig;
	bool bReadyForWork = false;
};
