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

#include "Generators/TGHydraulicErosion.h"

#include "Async/ParallelFor.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/FeedbackContext.h"
#include "TerrainGeneratorHelper.h"


UTGHydraulicErosion::UTGHydraulicErosion()
{
	GeneratorName = TEXT("HydraulicErosion");
}

UTextureRenderTarget2D* UTGHydraulicErosion::Generate(const FTGTerrainInfo& TerrainInfo)
{
	TArray<float> HeightData = TerrainInfo.DecodedHeight;

	Erode(HeightData, TerrainInfo.Size.GetMax(), NumIterations, bResetSeed);

	for (int32 Iter = 0; Iter < HeightData.Num(); Iter++)
	{
		switch (ErosionType)
		{
		case Erosion:
			HeightData[Iter] = FMath::Clamp(TerrainInfo.DecodedHeight[Iter] - HeightData[Iter], 0.f, MAX_flt);
			break;
		case Sediment:
			HeightData[Iter] = FMath::Clamp(HeightData[Iter] - TerrainInfo.DecodedHeight[Iter], 0.f, MAX_flt);
			break;
		}
	}

	RemapHeight(HeightData);

	MinHeight = FMath::Min(MinHeight, MaxHeight);
	MaxHeight = FMath::Max(MinHeight, MaxHeight);
	const float HMax = FMath::Max(HeightData);

	for (float& Iter : HeightData)
	{
		Iter = ApplyAdjustments(Iter);
		Iter = FMath::Clamp(Iter, MinHeight / 100.f * HMax, MaxHeight / 100.f * HMax);
	}

	const TArray<uint8> WeightData = RemapToWeight(HeightData);

	RenderTarget = DrawWeightmap(WeightData, GeneratorName, TerrainInfo.Size, TerrainInfo.bFullRange);

	return GetRenderTarget();
}

void UTGHydraulicErosion::Erode(TArray<float>& Map, int32 MapSize, int32 Iterations, bool ResetSeed)
{

	GWarn->BeginSlowTask(FText::FromString(TEXT("Generate: Hydraulic Erosion")), true, false);

	Initialize(MapSize, ResetSeed);

	const uint32 ThreadId = FPlatformTLS::GetCurrentThreadId();

	ParallelFor(
		Iterations,
		[&](int32 Iter)
		{
			if (FPlatformTLS::GetCurrentThreadId() == ThreadId)
			{
				GWarn->UpdateProgress(Iter, Iterations);
			}

			// Create water droplet at random point on map
			float PosX = FMath::RandRange(0.f, MapSize - 1.f);
			float PosY = FMath::RandRange(0.f, MapSize - 1.f);
			float DirX = 0;
			float DirY = 0;
			float Speed = InitialSpeed;
			float Water = InitialWaterVolume;
			float Sediment = 0;

			for (int32 Lifetime = 0; Lifetime < MaxDropletLifetime; Lifetime++)
			{
				const int32 NodeX = FMath::TruncToInt(PosX);
				const int32 NodeY = FMath::TruncToInt(PosY);
				const int32 DropletIndex = NodeY * MapSize + NodeX;
				// Calculate droplet's offset inside the cell (0,0) = at NW node, (1,1) = at SE node
				const float CellOffsetX = PosX - NodeX;
				const float CellOffsetY = PosY - NodeY;

				// Calculate droplet's height and direction of flow with bilinear interpolation of surrounding heights
				const FHeightAndGradient HeightAndGradient = CalculateHeightAndGradient(Map, MapSize, PosX, PosY);
				;

				// Update the droplet's direction and position (move position 1 unit regardless of speed)
				DirX = (DirX * Inertia - HeightAndGradient.GradientX * (1 - Inertia));
				DirY = (DirY * Inertia - HeightAndGradient.GradientY * (1 - Inertia));

				// Normalize direction
				const float Length = FMath::Sqrt(DirX * DirX + DirY * DirY);
				if (Length != 0)
				{
					DirX /= Length;
					DirY /= Length;
				}
				PosX += DirX;
				PosY += DirY;

				// Stop simulating droplet if it's not moving or has flowed over edge of map
				if ((DirX == 0 && DirY == 0) || PosX < 0 || PosX >= MapSize - 1 || PosY < 0 || PosY >= MapSize - 1)
				{
					break;
				}

				// Find the droplet's new height and calculate the DeltaHeight
				const float NewHeight = CalculateHeightAndGradient(Map, MapSize, PosX, PosY).Height;
				const float DeltaHeight = NewHeight - HeightAndGradient.Height;

				// Calculate the droplet's sediment capacity (higher when moving fast down a slope and contains lots of water)
				const float SedimentCapacity =
					FMath::Max(-DeltaHeight * Speed * Water * SedimentCapacityFactor, MinSedimentCapacity);

				// If carrying more sediment than capacity, or if flowing uphill:
				if (Sediment > SedimentCapacity || DeltaHeight > 0)
				{
					// If moving uphill (deltaHeight > 0) try fill up to the current height, otherwise deposit a fraction of the excess sediment
					const float AmountToDeposit = (DeltaHeight > 0) ? FMath::Min(DeltaHeight, Sediment)
																	: (Sediment - SedimentCapacity) * DepositSpeed;
					Sediment -= AmountToDeposit;

					// Add the sediment to the four nodes of the current cell using bilinear interpolation
					// Deposition is not distributed over a radius (like erosion) so that it can fill small pits
					Map[DropletIndex] += AmountToDeposit * (1 - CellOffsetX) * (1 - CellOffsetY);
					Map[DropletIndex + 1] += AmountToDeposit * CellOffsetX * (1 - CellOffsetY);
					Map[FMath::Min(DropletIndex + MapSize, Map.Num() - 1)] +=
						AmountToDeposit * (1 - CellOffsetX) * CellOffsetY;
					Map[FMath::Min(DropletIndex + MapSize + 1, Map.Num() - 1)] +=
						AmountToDeposit * CellOffsetX * CellOffsetY;
				}
				else
				{
					// Erode a fraction of the droplet's current carry capacity.
					// Clamp the erosion to the change in height so that it doesn't dig a hole in the terrain behind the droplet
					const float AmountToErode = FMath::Min((SedimentCapacity - Sediment) * ErodeSpeed, -DeltaHeight);

					// Use erosion brush to erode from all nodes inside the droplet's erosion radius
					for (int32 BrushPointIndex = 0; BrushPointIndex < ErosionBrushIndices[DropletIndex].Num();
						 BrushPointIndex++)
					{
						const int32 NodeIndex = ErosionBrushIndices[DropletIndex][BrushPointIndex];
						const float WeighedErodeAmount =
							AmountToErode * ErosionBrushWeights[DropletIndex][BrushPointIndex];
						const float DeltaSediment =
							(Map[NodeIndex] < WeighedErodeAmount) ? Map[NodeIndex] : WeighedErodeAmount;
						Map[NodeIndex] -= DeltaSediment;
						Sediment += DeltaSediment;
					}
				}

				// Update droplet's speed and water content
				Speed = FMath::Sqrt(Speed * Speed + DeltaHeight * Gravity);
				Water *= (1 - EvaporateSpeed);
			}
		},
		!FApp::ShouldUseThreadingForPerformance());
	GWarn->EndSlowTask();
}

void UTGHydraulicErosion::Initialize(int32 MapSize, bool bReset)
{
	ErosionBrushWeights.Empty();
	ErosionBrushIndices.Empty();

	if (bReset || CurrentSeed != Seed)
	{
		RandomStream.Initialize(Seed);
		CurrentSeed = Seed;
	}

	if (ErosionBrushIndices.Num() == 0 || CurrentErosionRadius != ErosionRadius || CurrentMapSize != MapSize)
	{
		InitializeBrushIndices(MapSize, ErosionRadius);
		CurrentErosionRadius = ErosionRadius;
		CurrentMapSize = MapSize;
	}
}

void UTGHydraulicErosion::InitializeBrushIndices(const int32 MapSize, const int32 Radius)
{
	ErosionBrushIndices.SetNumZeroed(MapSize * MapSize);
	ErosionBrushWeights.SetNumZeroed(MapSize * MapSize);

	TArray<int32> OffsetsX;
	TArray<int32> OffsetsY;
	TArray<float> Weights;

	OffsetsX.SetNum(Radius * Radius * 4);
	OffsetsY.SetNum(Radius * Radius * 4);
	Weights.SetNum(Radius * Radius * 4);
	float WeightSum = 0;
	int32 AddIndex = 0;

	for (int32 i = 0; i < ErosionBrushIndices.Num(); i++)
	{
		const int32 CentreX = i % MapSize;
		const int32 CentreY = i / MapSize;

		if (CentreY <= Radius || CentreY >= MapSize - Radius || CentreX <= Radius + 1 || CentreX >= MapSize - Radius)
		{
			WeightSum = 0;
			AddIndex = 0;

			for (int32 y = -Radius; y <= Radius; y++)
			{
				for (int32 x = -Radius; x <= Radius; x++)
				{
					const float SqrDst = x * x + y * y;

					if (SqrDst < Radius * Radius)
					{
						const int32 CoordX = CentreX + x;
						const int32 CoordY = CentreY + y;

						if (CoordX >= 0 && CoordX < MapSize && CoordY >= 0 && CoordY < MapSize)
						{
							const float Weight = 1 - sqrt(SqrDst) / Radius;
							WeightSum += Weight;
							Weights[AddIndex] = Weight;
							OffsetsX[AddIndex] = x;
							OffsetsY[AddIndex] = y;
							AddIndex++;
						}
					}
				}
			}
		}

		const int32 NumEntries = AddIndex;
		ErosionBrushIndices[i].SetNum(NumEntries);
		ErosionBrushWeights[i].SetNum(NumEntries);

		for (int32 j = 0; j < NumEntries; j++)
		{
			ErosionBrushIndices[i][j] = (OffsetsY[j] + CentreY) * MapSize + OffsetsX[j] + CentreX;
			ErosionBrushWeights[i][j] = Weights[j] / WeightSum;
		}
	}
}

FHeightAndGradient UTGHydraulicErosion::CalculateHeightAndGradient(
	TArray<float>& Nodes,
	int32& MapSize,
	float PosX,
	float PosY) const
{
	const int32 CoordX = FMath::TruncToInt(PosX);
	const int32 CoordY = FMath::TruncToInt(PosY);

	// Calculate droplet's offset inside the cell (0,0) = at NW node, (1,1) = at SE node
	const float X = PosX - CoordX;
	const float Y = PosY - CoordY;

	// Calculate heights of the four nodes of the droplet's cell
	const int32 NodeIndexNW = CoordY * MapSize + CoordX;
	const float HeightNW = Nodes[FMath::Min(NodeIndexNW, Nodes.Num() - 1)];
	const float HeightNE = Nodes[FMath::Min(NodeIndexNW + 1, Nodes.Num() - 1)];
	const float HeightSW = Nodes[FMath::Min(NodeIndexNW + MapSize, Nodes.Num() - 1)];
	const float HeightSE = Nodes[FMath::Min(NodeIndexNW + MapSize + 1, Nodes.Num() - 1)];

	// Calculate droplet's direction of flow with bilinear interpolation of height difference along the edges
	const float GradientX = (HeightNE - HeightNW) * (1 - Y) + (HeightSE - HeightSW) * Y;
	const float GradientY = (HeightSW - HeightNW) * (1 - X) + (HeightSE - HeightNE) * X;

	// Calculate height with bilinear interpolation of the heights of the nodes of the cell
	const float Height =
		HeightNW * (1 - X) * (1 - Y) + HeightNE * X * (1 - Y) + HeightSW * (1 - X) * Y + HeightSE * X * Y;

	const FHeightAndGradient OutResult { Height, GradientX, GradientY };

	return OutResult;
}

void UTGHydraulicErosion::RemapHeight(TArray<float>& Data) const
{
	const float MaxH = FMath::Max(Data);
	const float MinH = FMath::Min(Data);
	const float Dist = MaxH - MinH;

	for (int32 Iter = 0; Iter < Data.Num(); Iter++)
	{
		const float Rem = (((Data[Iter] - MinH) / (MaxH - MinH)) * (MAX_uint16 - MIN_uint16) + MIN_uint16);
		Data[Iter] = FMath::Clamp(Rem / Dist, 0.f, 1.f) * MAX_uint16;
		if (bInvert)
		{
			Data[Iter] = MAX_uint16 - Data[Iter];
		}
	}
}
