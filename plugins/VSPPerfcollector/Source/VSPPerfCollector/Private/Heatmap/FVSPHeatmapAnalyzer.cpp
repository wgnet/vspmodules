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
#include "FVSPHeatmapAnalyzer.h"

#include "Heatmap/VSPHeatmapCollectorModule.h"
#include "Heatmap/VSPHeatmapCollectSettings.h"
#include "TraceServices/Model/AnalysisSession.h"

namespace HeatmapAnalyzerLocal
{
	constexpr const ANSICHAR* TimeSec = "TimeSec";
}

FVSPHeatmapAnalyzer::FVSPHeatmapAnalyzer(Trace::IAnalysisSession &InSession, FVSPHeatmapCollectorModule& InHeatmapCollector):
	Session(InSession),
	HeatmapCollector(InHeatmapCollector)
{
}

void FVSPHeatmapAnalyzer::OnAnalysisBegin(const FOnAnalysisContext &Context)
{
	constexpr const ANSICHAR* AnalyzerName = "VSPHeatmap";
	auto& Builder = Context.InterfaceBuilder;
	Builder.RouteEvent(RouteId_HeatmapSettings, AnalyzerName, "HeatmapSettings");
	Builder.RouteEvent(RouteId_BeginSnapshot, AnalyzerName, "BeginSnapshot");
	Builder.RouteEvent(RouteId_StopSnapshot, AnalyzerName, "StopSnapshot");
	Builder.RouteEvent(RouteId_MinimapInfo, AnalyzerName, "MinimapInfo");
}

bool FVSPHeatmapAnalyzer::OnEvent(uint16 RouteId, EStyle Style, const FOnEventContext &Context)
{

	Trace::FAnalysisSessionEditScope AnalysisSessionEditScope(Session);
	switch (RouteId)
	{
		case RouteId_HeatmapSettings:
		{
			HandleHeatmapSettings(Context);
			break;
		}
		case RouteId_BeginSnapshot:
		{
			HandleBeginSnapshot(Context);
			break;
		}
		case RouteId_StopSnapshot:
		{
			HandleStopSnapshot(Context);
			break;
		}
		case RouteId_MinimapInfo:
		{
			HandleMinimapInfo(Context);
			break;
		}
		default:
			break;
	}

	return true;
}

void FVSPHeatmapAnalyzer::HandleHeatmapSettings(const FOnEventContext &Context) const
{
	UVSPHeatmapCollectSettings* Settings = UVSPHeatmapCollectSettings::GetMutable();
	Settings->StartupTimeDelay = Context.EventData.GetValue<float>("StartupTimeDelay");
	Settings->CamerasCountX = Context.EventData.GetValue<int32>("CamerasCountX");
	Settings->CamerasCountY = Context.EventData.GetValue<int32>("CamerasCountY");
	Settings->CollectionFramesDelay = Context.EventData.GetValue<int32>("CollectionFramesDelay");
	Settings->FramesToCollect = Context.EventData.GetValue<int32>("FramesToCollect");
	Settings->CollectedTopPercentile = Context.EventData.GetValue<int32>("CollectedTopPercentile");
	Settings->CollectedBottomPercentile = Context.EventData.GetValue<int32>("CollectedBottomPercentile");
	Settings->StartPosX = Context.EventData.GetValue<float>("StartPosX");
	Settings->StartPosY = Context.EventData.GetValue<float>("StartPosY");
	Settings->EndPosX = Context.EventData.GetValue<float>("EndPosX");
	Settings->EndPosY = Context.EventData.GetValue<float>("EndPosY");
}

void FVSPHeatmapAnalyzer::HandleBeginSnapshot(const FOnEventContext &Context) const
{
	using namespace HeatmapAnalyzerLocal;
	const FVector Location{
		Context.EventData.GetValue<float>("LocationX"),
		Context.EventData.GetValue<float>("LocationY"),
		Context.EventData.GetValue<float>("LocationZ")
	};

	const FVector Rotation{
		Context.EventData.GetValue<float>("RotationX"),
		Context.EventData.GetValue<float>("RotationY"),
		Context.EventData.GetValue<float>("RotationZ")
	};

	HeatmapCollector.SnapshotBegin(
		Context.EventTime.AsSeconds(Context.EventData.GetValue<uint64>(TimeSec)),
		Location,
		Rotation);
}

void FVSPHeatmapAnalyzer::HandleStopSnapshot(const FOnEventContext &Context) const
{
	using namespace HeatmapAnalyzerLocal;
	HeatmapCollector.SnapshotEnd(Context.EventTime.AsSeconds(Context.EventData.GetValue<uint64>(TimeSec)));
}

void FVSPHeatmapAnalyzer::HandleMinimapInfo(const FOnEventContext &Context) const
{
	FString MapName;
	Context.EventData.GetString("Name", MapName);
	HeatmapCollector.SetMiniMapInfo(
		MapName,
		Context.EventData.GetValue<float>("StartX"),
		Context.EventData.GetValue<float>("StartY"),
		Context.EventData.GetValue<float>("EndX"),
		Context.EventData.GetValue<float>("EndY"));
}
