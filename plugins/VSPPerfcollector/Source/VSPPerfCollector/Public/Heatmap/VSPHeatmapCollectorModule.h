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
#include "VSPAnalysisFeatureModule.h"
#include "TraceServices/ModuleService.h"

struct FVSPTreeItem;
class FVSPAnalysisFeatureModule;

class VSPPERFCOLLECTOR_API FVSPHeatmapCollectorModule: public Trace::IModule
{
	struct FMiniMapInfo
	{
		FString Name;
		float StartX, StartY;
		float EndX, EndY;
	};

	struct FHeatmapSnapshot
	{
		FVector Location;
		FVector Rotation;
		float BeginTimeMs, EndTimeMs;
	};
	struct FHeatmapThreadData
	{
		TMap<FString, TArray<double>> Metrics;
		FVSPPerfFrame LastFrame;
	};

public:
	static void SendHeatmapSettings();
	static void StartHeatmapSnapshot(const FVector& Location, const FVector& Rotation);
	static void StopHeatmapSnapshot();
	static void SetHeatmapMinimapInfo(const FString& MapName, float StartX, float StartY, float EndX, float EndY);

	FVSPHeatmapCollectorModule(FVSPAnalysisFeatureModule& InPerfCollector);
	virtual ~FVSPHeatmapCollectorModule() = default;

	void GetModuleInfo(Trace::FModuleInfo& OutModuleInfo) override;
	void OnAnalysisBegin(Trace::IAnalysisSession& InSession) override;
	void PostAnalysisBegin(Trace::IAnalysisSession& InSession) override;
	void GetLoggers(TArray<const TCHAR*>& OutLoggers) override;
	void GenerateReports(const Trace::IAnalysisSession& InSession,
						 const TCHAR* CmdLine,
						 const TCHAR* OutputDirectory) override;
	const TCHAR* GetCommandLineArgument() override;

	void DumpToFile(const FString& Filepath) const;
	void SendToUrl() const;

	void SetMiniMapInfo(const FString MapName, float StartX, float StartY, float EndX, float EndY);
	void SnapshotBegin(float TimeSec, const FVector& Location, const FVector& Rotation);
	void SnapshotEnd(float TimeSec);

	FJsonDomBuilder::FArray GetHeatmapJson() const;
	FJsonDomBuilder::FObject GetSettings() const;


protected:
	TArray<TArray<FVSPPerfFrame>> GetPerfSnapshots() const;
	TMap<FString, FHeatmapThreadData> GetThreadDataFromFrames(const TArray<FVSPPerfFrame>& Frames) const;

private:
	FVSPAnalysisFeatureModule& PerfAnalysis;

	FMiniMapInfo MinimapInfo;
	TArray<FHeatmapSnapshot> Snapshots;
};
