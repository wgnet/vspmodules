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

#include "TGBaseLayer.h"
#include "TGHydraulicErosion.generated.h"

struct FHeightAndGradient
{
	float Height;
	float GradientX;
	float GradientY;
};

UENUM()
enum ETGChannels
{
	Erosion,
	Sediment,
};

UCLASS(EditInlineNew)
class UTGHydraulicErosion : public UTGBaseLayer
{
	GENERATED_BODY()

public:
	UTGHydraulicErosion();

	virtual UTextureRenderTarget2D* Generate(const FTGTerrainInfo& TerrainInfo) override;

	void Erode(TArray<float>& Map, int32 MapSize, int32 Iterations = 1, bool ResetSeed = false);

	void Initialize(int32 MapSize, bool ResetSeed);

	void InitializeBrushIndices(int32 MapSize, int32 Radius);

	FHeightAndGradient CalculateHeightAndGradient(TArray<float>& Nodes, int32& MapSize, float PosX, float PosY) const;

	void RemapHeight(TArray<float>& Data) const;

	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	TEnumAsByte<ETGChannels> ErosionType;

	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	bool bResetSeed = false;

	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	int32 NumIterations = 70000;

	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	int32 Seed = 12345;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", ClampMax = "8"), Category = "HydraulicErosion")
	int32 ErosionRadius = 3;

	// At zero, water will instantly change direction to flow downhill. At 1, water will never change direction.
	UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", ClampMax = "1"), Category = "HydraulicErosion")
	float Inertia = 0.05f;

	// Multiplier for how much sediment a droplet can carry
	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	float SedimentCapacityFactor = 4.f;

	// Used to prevent carry capacity getting too close to zero on flatter terrain
	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	float MinSedimentCapacity = 0.01f;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", ClampMax = "1"), Category = "HydraulicErosion")
	float ErodeSpeed = 0.3f;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", ClampMax = "1"), Category = "HydraulicErosion")
	float DepositSpeed = 0.3f;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = "0", ClampMax = "1"), Category = "HydraulicErosion")
	float EvaporateSpeed = 0.01f;

	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	float Gravity = 4.f;

	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	int32 MaxDropletLifetime = 30;

	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	float InitialWaterVolume = 1.f;

	UPROPERTY(EditInstanceOnly, Category = "HydraulicErosion")
	float InitialSpeed = 1.f;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = 0, ClampMax = 100), Category = "Adjustments")
	int32 MinHeight = 0;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = 0, ClampMax = 100), Category = "Adjustments")
	int32 MaxHeight = 100;

private:
	// Indices and weights of erosion brush precomputed for every node
	TArray<TArray<int32> > ErosionBrushIndices;
	TArray<TArray<float> > ErosionBrushWeights;
	FRandomStream RandomStream;
	int32 CurrentSeed;
	int32 CurrentErosionRadius;
	int32 CurrentMapSize;
};
