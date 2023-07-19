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

#include "TGBaseLayer.h"
#include "TGSteepness.generated.h"


UCLASS(EditInlineNew)
class TERRAINGENERATOR_API UTGSteepness : public UTGBaseLayer
{
	GENERATED_BODY()

public:
	UTGSteepness();

	virtual UTextureRenderTarget2D* Generate(const FTGTerrainInfo& TerrainInfo) override;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = 0, ClampMax = 100), Category = "Adjustments")
	int32 MinHeight = 0;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = 0, ClampMax = 100), Category = "Adjustments")
	int32 MaxHeight = 100;

private:
	TArray<float> GenerateSteepness(const TArray<float>& InHeightmap, FIntPoint Size);
	static float GetSteepnessLQ(const TArray<float>& InHeightmap, FIntPoint Size, int32 X, int32 Y);
	static float GetSteepnessMQ(const TArray<float>& InHeightmap, FIntPoint Size, int32 X, int32 Y);
	static float GetSteepnessHQ(const TArray<float>& InHeightmap, FIntPoint Size, int32 X, int32 Y);
	static float GetPixel(const TArray<float>& InHeightmap, int32 X, int32 Y, FIntPoint& Size);
	static float GetHeight(float InHeight);
};
