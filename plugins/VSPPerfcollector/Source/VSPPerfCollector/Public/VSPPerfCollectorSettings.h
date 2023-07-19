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
#include "Misc/Paths.h"
#include "VSPPerfCollectorSettings.generated.h"


UCLASS(Config = VSPPerfCollector, meta = (DisplayName = "VSPPerfCollector"))
class VSPPERFCOLLECTOR_API UVSPPerfCollectorSettings : public UObject
{
	GENERATED_BODY()
public:
	static const UVSPPerfCollectorSettings& Get() { return *GetDefault<UVSPPerfCollectorSettings>(); }

	/// Relative to config folder path
	UPROPERTY(Config, EditAnywhere, Category = Config)
	FString RelativeConfigPath;

	/// Local set of favorite metrics
	UPROPERTY(EditDefaultsOnly, Config, Category = UI)
	TSet<FString> FavoriteMetrics;

	/// Default frames count for average value calculation in BudgetMonitorWidget
	UPROPERTY(EditDefaultsOnly, Config, Category = UI)
	uint32 TableViewAvgFramesCount = 60;

	/// Defines how fast BudgetMonitor table widget will be updated
	UPROPERTY(EditDefaultsOnly, Config, Category = UI)
	uint32 UIUpdateMissingFrames = 3;

	UPROPERTY(Config, EditAnywhere, Category = Posting)
	uint32 PostBulkSize = 1000;

	UPROPERTY(Config, EditAnywhere, Category = Posting)
	FString ReceiverUrl;

	UPROPERTY(Config, EditAnywhere, Category = Posting)
	FString HeaderRoute;

	UPROPERTY(Config, EditAnywhere, Category = Posting)
	FString BookmarkRoute;

	UPROPERTY(Config, EditAnywhere, Category = Posting)
	FString MetricsRoute;

	UPROPERTY(Config, EditAnywhere, Category = Posting)
	FString PerfConfigRoute;

	/// Interval for clean disapeared events 
	UPROPERTY(Config, EditAnywhere, Category = Default)
	float CleanupEventsTimeMs = 5000.f;

	static FString FullConfigPath() { return FPaths::ProjectConfigDir() / Get().RelativeConfigPath; }
	static FString FullBookmarkRoute() { return Get().ReceiverUrl / Get().BookmarkRoute; }
	static FString FullMetricsRoute() { return Get().ReceiverUrl / Get().MetricsRoute; }
	static FString FullHeaderRoute() { return Get().ReceiverUrl / Get().HeaderRoute; }
	static FString FullPerfConfigRoute() { return Get().ReceiverUrl / Get().PerfConfigRoute; }
};
