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
#include "Heatmap/VSPHeatmapCollectorModule.h"

#include "FVSPHeatmapAnalyzer.h"
#include "HttpModule.h"
#include "VSPPerfCollector.h"
#include "VSPPerfTrace.h"
#include "Algo/Accumulate.h"
#include "Algo/MaxElement.h"
#include "Heatmap/VSPHeatmapCollectSettings.h"
#include "Trace/Trace.inl"
#include "TraceServices/Model/AnalysisSession.h"
#include "Trace/Detail/LogScope.h"
#include "HAL/FileManager.h"
#include "Modules/ModuleManager.h"

UE_TRACE_EVENT_BEGIN(VSPHeatmap, HeatmapSettings)
	UE_TRACE_EVENT_FIELD(float, StartupTimeDelay)
	UE_TRACE_EVENT_FIELD(int32, CamerasCountX)
	UE_TRACE_EVENT_FIELD(int32, CamerasCountY)
	UE_TRACE_EVENT_FIELD(int32, CameraRotations)
	UE_TRACE_EVENT_FIELD(int32, CollectionFramesDelay)
	UE_TRACE_EVENT_FIELD(int32, FramesToCollect)
	UE_TRACE_EVENT_FIELD(int32, CollectedTopPercentile)
	UE_TRACE_EVENT_FIELD(int32, CollectedBottomPercentile)
	UE_TRACE_EVENT_FIELD(float, StartPosX)
	UE_TRACE_EVENT_FIELD(float, StartPosY)
	UE_TRACE_EVENT_FIELD(float, EndPosX)
	UE_TRACE_EVENT_FIELD(float, EndPosY)
UE_TRACE_EVENT_END()


UE_TRACE_EVENT_BEGIN(VSPHeatmap, BeginSnapshot)
	UE_TRACE_EVENT_FIELD(float, TimeSec)
	UE_TRACE_EVENT_FIELD(float, LocationX)
	UE_TRACE_EVENT_FIELD(float, LocationY)
	UE_TRACE_EVENT_FIELD(float, LocationZ)
	UE_TRACE_EVENT_FIELD(float, RotationX)
	UE_TRACE_EVENT_FIELD(float, RotationY)
	UE_TRACE_EVENT_FIELD(float, RotationZ)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(VSPHeatmap, StopSnapshot)
	UE_TRACE_EVENT_FIELD(float, TimeSec)
UE_TRACE_EVENT_END()

UE_TRACE_EVENT_BEGIN(VSPHeatmap, MinimapInfo)
	UE_TRACE_EVENT_FIELD(Trace::WideString, Name)
	UE_TRACE_EVENT_FIELD(float, StartX)
	UE_TRACE_EVENT_FIELD(float, StartY)
	UE_TRACE_EVENT_FIELD(float, EndX)
	UE_TRACE_EVENT_FIELD(float, EndY)
UE_TRACE_EVENT_END()

void FVSPHeatmapCollectorModule::SendHeatmapSettings()
{
	const UVSPHeatmapCollectSettings* Settings = UVSPHeatmapCollectSettings::Get();
	if (UE_TRACE_CHANNELEXPR_IS_ENABLED(VSPUEChannel))
	{
		UE_TRACE_LOG(VSPHeatmap, HeatmapSettings, VSPUEChannel)
			<< HeatmapSettings.StartupTimeDelay(Settings->StartupTimeDelay)
			<< HeatmapSettings.CamerasCountX(Settings->CamerasCountX)
			<< HeatmapSettings.CamerasCountY(Settings->CamerasCountY)
			<< HeatmapSettings.CameraRotations(Settings->CameraRotations)
			<< HeatmapSettings.CollectionFramesDelay(Settings->CollectionFramesDelay)
			<< HeatmapSettings.FramesToCollect(Settings->FramesToCollect)
			<< HeatmapSettings.CollectedTopPercentile(Settings->CollectedTopPercentile)
			<< HeatmapSettings.CollectedBottomPercentile(Settings->CollectedBottomPercentile)
			<< HeatmapSettings.StartPosX(Settings->StartPosX)
			<< HeatmapSettings.StartPosY(Settings->StartPosY)
			<< HeatmapSettings.EndPosX(Settings->EndPosX)
			<< HeatmapSettings.EndPosY(Settings->EndPosY);
	}
}

void FVSPHeatmapCollectorModule::StartHeatmapSnapshot(const FVector& Location, const FVector& Rotation)
{
	if (UE_TRACE_CHANNELEXPR_IS_ENABLED(VSPUEChannel))
	{
		UE_TRACE_LOG(VSPHeatmap, BeginSnapshot, VSPUEChannel)
			<< BeginSnapshot.TimeSec(FPlatformTime::Cycles64())
			<< BeginSnapshot.LocationX(Location.X)
			<< BeginSnapshot.LocationY(Location.Y)
			<< BeginSnapshot.LocationZ(Location.Z)
			<< BeginSnapshot.RotationX(Rotation.X)
			<< BeginSnapshot.RotationY(Rotation.Y)
			<< BeginSnapshot.RotationZ(Rotation.Z);
	}
}

void FVSPHeatmapCollectorModule::StopHeatmapSnapshot()
{
	if (UE_TRACE_CHANNELEXPR_IS_ENABLED(VSPUEChannel))
	{
		UE_TRACE_LOG(VSPHeatmap, StopSnapshot, VSPUEChannel)
			<< StopSnapshot.TimeSec(FPlatformTime::Cycles64());
	}
}

void FVSPHeatmapCollectorModule::SetHeatmapMinimapInfo(const FString& MapName, float StartX, float StartY, float EndX, float EndY)
	{
		if (UE_TRACE_CHANNELEXPR_IS_ENABLED(VSPUEChannel))
		{
			UE_TRACE_LOG(VSPHeatmap, MinimapInfo, VSPUEChannel)
				<< MinimapInfo.Name(*MapName)
				<< MinimapInfo.StartX(StartX)
				<< MinimapInfo.StartY(StartY)
				<< MinimapInfo.EndX(EndX)
				<< MinimapInfo.EndY(EndY);
		}
}

FVSPHeatmapCollectorModule::FVSPHeatmapCollectorModule(FVSPAnalysisFeatureModule& InPerfCollector):
	PerfAnalysis(InPerfCollector)
{

}

void FVSPHeatmapCollectorModule::GetModuleInfo(Trace::FModuleInfo& OutModuleInfo)
{
	OutModuleInfo.DisplayName = TEXT("FVSPHeatmapCollectorModule");
	OutModuleInfo.Name = OutModuleInfo.DisplayName;
}

void FVSPHeatmapCollectorModule::OnAnalysisBegin(Trace::IAnalysisSession& InSession)
{
	InSession.AddAnalyzer(new FVSPHeatmapAnalyzer(InSession, *this));
}

void FVSPHeatmapCollectorModule::PostAnalysisBegin(Trace::IAnalysisSession& InSession)
{

}

void FVSPHeatmapCollectorModule::GetLoggers(TArray<const TCHAR*>& OutLoggers)
{
}

void FVSPHeatmapCollectorModule::GenerateReports(const Trace::IAnalysisSession& InSession,
	const TCHAR* CmdLine,
	const TCHAR* OutputDirectory)
{
	if (!UVSPHeatmapCollectSettings::Get()->DumpFile.IsEmpty())
		DumpToFile(FString(OutputDirectory) / UVSPHeatmapCollectSettings::Get()->DumpFile);
}

const TCHAR* FVSPHeatmapCollectorModule::GetCommandLineArgument()
{
	return TEXT("VSPHeatmapCollecting");
}

void FVSPHeatmapCollectorModule::DumpToFile(const FString& Filepath) const
{
	if (Snapshots.Num() == 0)
		return;

	const TUniquePtr<FArchive> Ar(IFileManager::Get().CreateFileWriter(*Filepath));
	if (!Ar)
		return;

	const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(Ar.Get());
	FJsonDomBuilder::FObject Obj;
	Obj.Set("Settings", GetSettings());
	Obj.Set("Heatmaps", GetHeatmapJson());
	FJsonSerializer::Serialize(Obj.AsJsonObject(), Writer);
}

void FVSPHeatmapCollectorModule::SendToUrl() const
{
	if (Snapshots.Num() == 0 || UVSPHeatmapCollectSettings::Get()->PostingUrl.IsEmpty())
	{
		UE_LOG(
			LogTemp,
			Display,
			TEXT("Heatmap sending error: %d records, url = %s"),
			Snapshots.Num(),
			*UVSPHeatmapCollectSettings::Get()->PostingUrl);
		return;
	}

	FHttpModule& HttpModule = FModuleManager::LoadModuleChecked<FHttpModule>("HTTP");
	auto PostRequest =
		[ &HttpModule ](const FString& Data, const FString& TargetUrl)
		{
			UE_LOG(LogTemp, Display, TEXT("Heatmap sending to %s"), *TargetUrl);
			const TSharedRef<IHttpRequest, ESPMode::ThreadSafe> Request = HttpModule.CreateRequest();
			Request->SetURL(TargetUrl);
			Request->SetVerb("POST");
			Request->SetHeader(TEXT("Content-Type"), TEXT("application/json"));
			Request->SetContentAsString(Data);
			Request->ProcessRequest();
			FHttpResponsePtr Response = Request->GetResponse();
		};

	PostRequest(GetSettings().ToString(), UVSPHeatmapCollectSettings::Get()->SettingsUrl);
	PostRequest(GetHeatmapJson().ToString(), UVSPHeatmapCollectSettings::Get()->PostingUrl);
}

void FVSPHeatmapCollectorModule::SetMiniMapInfo(const FString MapName, float StartX, float StartY, float EndX, float EndY)
{
	MinimapInfo.Name = MapName;
	MinimapInfo.StartX = StartX;
	MinimapInfo.StartY = StartY;
	MinimapInfo.EndX = EndX;
	MinimapInfo.EndY = EndY;
}

void FVSPHeatmapCollectorModule::SnapshotBegin(float TimeSec, const FVector& Location, const FVector& Rotation)
{
	FHeatmapSnapshot Snapshot;
	Snapshot.Location = Location;
	Snapshot.Rotation = Rotation;
	Snapshot.BeginTimeMs = TimeSec * 1000.f;
	Snapshots.Push(Snapshot);
}

void FVSPHeatmapCollectorModule::SnapshotEnd(float TimeSec)
{
	Snapshots.Last().EndTimeMs = TimeSec * 1000.f;
}

TArray<TArray<FVSPPerfFrame>> FVSPHeatmapCollectorModule::GetPerfSnapshots() const
{
	TArray<TArray<FVSPPerfFrame>> SnapshotPerf;
	SnapshotPerf.SetNum(Snapshots.Num());
	for (int32 Idx = 0; Idx < Snapshots.Num(); ++Idx)
	{
		const FHeatmapSnapshot& Snapshot = Snapshots[Idx];
		PerfAnalysis.EnumerateFrameData([Idx, &Snapshot, &SnapshotPerf](const FVSPPerfFrame& Frame)
		{
			if (Frame.EndTime > Snapshot.EndTimeMs)
				return false;

			if (Frame.StartTime >= Snapshot.BeginTimeMs && Frame.EndTime <= Snapshot.EndTimeMs)
				SnapshotPerf[Idx].Push(Frame);

			return true;
		});
	}
	return SnapshotPerf;
}

TMap<FString, FVSPHeatmapCollectorModule::FHeatmapThreadData> FVSPHeatmapCollectorModule::GetThreadDataFromFrames(const TArray<FVSPPerfFrame>& Frames) const
{
	TMap<FString, FHeatmapThreadData> NamedThreadData;
	for (const FVSPPerfFrame& Frame : Frames)
	{
		FHeatmapThreadData& ThreadData = NamedThreadData.FindOrAdd(Frame.ThreadName);
		// Frames contains frame for each thread at once
		ThreadData.LastFrame = Frame;
		for (const FVSPPerfInfo& Metric :Frame.Metrics)
			ThreadData.Metrics.FindOrAdd(Metric.Name).Push(Metric.Duration);
	}

    for (TTuple<FString, FHeatmapThreadData> &Thread : NamedThreadData)
    {
		for (TTuple<FString, TArray<double>> Metric : Thread.Value.Metrics)
			Algo::Sort(Metric.Value);
    }
    return NamedThreadData;
}

FJsonDomBuilder::FArray FVSPHeatmapCollectorModule::GetHeatmapJson() const
{
	TArray<TArray<FVSPPerfFrame>> SnapshotPerf = GetPerfSnapshots();
	FJsonDomBuilder::FArray HeatmapsJson;

	for (int32 Idx = 0; Idx < SnapshotPerf.Num(); ++Idx)
	{
		const FHeatmapSnapshot& Snapshot = Snapshots[Idx];
		const TArray<FVSPPerfFrame>& Frames = SnapshotPerf[Idx];
		TMap<FString, FHeatmapThreadData> NamedThreadData = GetThreadDataFromFrames(Frames);
		auto MetricConstructor =
			[&NamedThreadData](const FString& ThreadName, FJsonDomBuilder::FObject& Object, const FVSPTreeItem& Item)
			{
				FHeatmapThreadData* Thread = NamedThreadData.Find(ThreadName);
				if (!Item.Self || !Thread)
					return;

				if (TArray<double>* Metrics = Thread->Metrics.Find(Item.Self->Name))
				{
					TArrayView<double> MetricsView(*Metrics);
					const int32 SliceStart = UVSPHeatmapCollectSettings::Get()->CollectedBottomPercentile;
					const int32 SliceCount = Metrics->Num() - SliceStart - UVSPHeatmapCollectSettings::Get()->CollectedTopPercentile;
					MetricsView = MetricsView.Slice(SliceStart, SliceCount);

					Object.Set("Avg", Algo::Accumulate(MetricsView, 0.0) / MetricsView.Num());
					if (double* Max = Algo::MaxElement(MetricsView))
						Object.Set("Max", *Max);
				}
			};

		auto ResultProcessor =
			[&HeatmapsJson, &Snapshot](const FVSPPerfFrame& Frame, FJsonDomBuilder::FObject& Object)
			{
				FJsonDomBuilder::FObject LocationJson;
				LocationJson.Set("X", Snapshot.Location.X);
				LocationJson.Set("Y", Snapshot.Location.Y);
				LocationJson.Set("Z", Snapshot.Location.Z);
				Object.Set("Location", LocationJson);
				
				FJsonDomBuilder::FObject RotationJson;
				RotationJson.Set("X", Snapshot.Rotation.X);
				RotationJson.Set("Y", Snapshot.Rotation.Y);
				RotationJson.Set("Z", Snapshot.Rotation.Z);
				Object.Set("Rotation", RotationJson);

				HeatmapsJson.Add(Object);
			};

		TArray<FVSPPerfFrame> LastFrames;
		for (auto& Thread: NamedThreadData)
		{
			LastFrames.Push(Thread.Value.LastFrame);
		}
		PerfAnalysis.MetricsToJson(LastFrames, ResultProcessor, MetricConstructor);
	}
	UE_LOG(LogTemp, Display, TEXT("Heatmap count = %d"), HeatmapsJson.Num());
	return HeatmapsJson;
}

FJsonDomBuilder::FObject FVSPHeatmapCollectorModule::GetSettings() const
{
	const UVSPHeatmapCollectSettings* Settings = UVSPHeatmapCollectSettings::Get();

	FJsonDomBuilder::FObject SettingsJson;

	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, StartupTimeDelay), Settings->StartupTimeDelay);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, CamerasCountX), Settings->CamerasCountX);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, CamerasCountY), Settings->CamerasCountY);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, CameraRotations), Settings->CameraRotations);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, CollectionFramesDelay), Settings->CollectionFramesDelay);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, FramesToCollect), Settings->FramesToCollect);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, CollectedTopPercentile), Settings->CollectedTopPercentile);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, CollectedBottomPercentile), Settings->CollectedBottomPercentile);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, StartPosX), Settings->StartPosX);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, StartPosY), Settings->StartPosY);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, EndPosX), Settings->EndPosX);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(UVSPHeatmapCollectSettings, EndPosY), Settings->EndPosY);

	FJsonDomBuilder::FObject MapJson;
	MapJson.Set(GET_MEMBER_NAME_STRING_CHECKED(FMiniMapInfo, Name), MinimapInfo.Name);
	MapJson.Set(GET_MEMBER_NAME_STRING_CHECKED(FMiniMapInfo, StartX), MinimapInfo.StartX);
	MapJson.Set(GET_MEMBER_NAME_STRING_CHECKED(FMiniMapInfo, StartY), MinimapInfo.StartY);
	MapJson.Set(GET_MEMBER_NAME_STRING_CHECKED(FMiniMapInfo, EndX), MinimapInfo.EndX);
	MapJson.Set(GET_MEMBER_NAME_STRING_CHECKED(FMiniMapInfo, EndY), MinimapInfo.EndY);
	SettingsJson.Set(GET_MEMBER_NAME_STRING_CHECKED(FVSPHeatmapCollectorModule, MinimapInfo), MapJson);

	return SettingsJson;
}
