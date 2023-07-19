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
#include "VSPHeatmapCollectSettings.generated.h"

UCLASS(Config = VSPPerfCollector, meta = (DisplayName = "VSPHeatmapCollecting"))
class UVSPHeatmapCollectSettings: public UObject
{
	GENERATED_BODY()
public:
	static const UVSPHeatmapCollectSettings* Get() { return GetDefault<UVSPHeatmapCollectSettings>(); }
	static UVSPHeatmapCollectSettings* GetMutable() { return GetMutableDefault<UVSPHeatmapCollectSettings>(); }

	UFUNCTION(BlueprintCallable, Category = Heatmap)
	static const UVSPHeatmapCollectSettings* GetHeatmapCollectorSettings() { return Get(); }

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	float StartupTimeDelay = 1.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	int32 CamerasCountX = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	int32 CamerasCountY = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	int32 CameraRotations = 8;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	int32 CollectionFramesDelay = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	int32 FramesToCollect = 30;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	int32 CollectedTopPercentile = 2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	int32 CollectedBottomPercentile = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	bool bCaptureAllMap = true;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	float StartPosX = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	float StartPosY = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	float EndPosX = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	float EndPosY = 0.f;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	FString PostingUrl;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	FString SettingsUrl;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Config, Category = Heatmap)
	FString DumpFile = "VSPPerfHeatmap.json";
};
