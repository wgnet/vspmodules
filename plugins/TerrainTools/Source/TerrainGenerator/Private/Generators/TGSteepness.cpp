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

#include "Generators/TGSteepness.h"

#include "Kismet/KismetRenderingLibrary.h"
#include "Misc/FeedbackContext.h"
#include "Misc/ScopedSlowTask.h"


UTGSteepness::UTGSteepness()
{
	GeneratorName = TEXT("Steepness");
}

UTextureRenderTarget2D* UTGSteepness::Generate(const FTGTerrainInfo& TerrainInfo)
{
	TArray<float> HeightData = GenerateSteepness(TerrainInfo.DecodedHeight, TerrainInfo.Size);

	MinHeight = FMath::Min(MinHeight, MaxHeight);
	MaxHeight = FMath::Max(MinHeight, MaxHeight);
	const float HMax = FMath::Max(HeightData);

	for (float& Iter : HeightData)
	{
		Iter = ApplyAdjustments(Iter);
		Iter = FMath::Clamp(Iter, MinHeight / 100.f * HMax, MaxHeight / 100.f * HMax);
	}

	const TArray<uint8> OutResult = RemapToWeight(HeightData);
	RenderTarget = DrawWeightmap(OutResult, GeneratorName, TerrainInfo.Size, TerrainInfo.bFullRange);
	return GetRenderTarget();
}

TArray<float> UTGSteepness::GenerateSteepness(const TArray<float>& InHeightmap, FIntPoint Size)
{
	GWarn->BeginSlowTask(FText::FromString(TEXT("Generate: Steepness")), true, false);
	int32 Progress = 0;
	int32 TotalProgress = Size.X;

	TArray<float> OutResult;
	OutResult.SetNum(InHeightmap.Num());

	for (int32 x = 0; x < Size.X; x++)
	{
		for (int32 y = 0; y < Size.Y; y++)
		{
			OutResult[x * Size.Y + y] = GetSteepnessHQ(InHeightmap, Size, x, y);
		}
		GWarn->UpdateProgress(Progress++, TotalProgress);
	}

	GWarn->EndSlowTask();

	return OutResult;
}

float UTGSteepness::GetSteepnessLQ(const TArray<float>& InHeightmap, FIntPoint Size, const int32 X, const int32 Y)
{
	const float Height = GetPixel(InHeightmap, X, Y, Size);
	const float DX = GetHeight(GetPixel(InHeightmap, FMath::Clamp(X + 1, 0, Size.X), Y, Size)) - Height;
	const float DY = GetHeight(GetPixel(InHeightmap, X, FMath::Clamp(Y + 1, 0, Size.Y), Size)) - Height;
	return FMath::Abs(DX) + FMath::Abs(DY);
}

float UTGSteepness::GetSteepnessMQ(const TArray<float>& InHeightmap, FIntPoint Size, const int32 X, const int32 Y)
{
	const float Height = GetPixel(InHeightmap, X, Y, Size);
	const float DX = GetHeight(GetPixel(InHeightmap, FMath::Clamp(X + 1, 0, Size.X), Y, Size)) - Height;
	const float DY = GetHeight(GetPixel(InHeightmap, X, FMath::Clamp(Y + 1, 0, Size.Y), Size)) - Height;
	return FMath::Sqrt(DX * DX + DY * DY);
}

float UTGSteepness::GetSteepnessHQ(const TArray<float>& InHeightmap, FIntPoint Size, const int32 X, const int32 Y)
{
	const float Height = GetHeight(GetPixel(InHeightmap, X, Y, Size));

	const float North = GetHeight(GetPixel(InHeightmap, FMath::Clamp(X + 1, 0, Size.X - 1), Y, Size)) - Height;
	const float South = GetHeight(GetPixel(InHeightmap, X, FMath::Clamp(Y + 1, 0, Size.Y - 1), Size)) - Height;
	const float East = GetHeight(GetPixel(InHeightmap, FMath::Clamp(X - 1, 0, Size.X - 1), Y, Size)) - Height;
	const float West = GetHeight(GetPixel(InHeightmap, X, FMath::Clamp(Y - 1, 0, Size.Y - 1), Size)) - Height;

	return FMath::Sqrt(North * North + South * South + East * East + West * West);
}

float UTGSteepness::GetPixel(const TArray<float>& InHeightmap, const int32 X, const int32 Y, FIntPoint& Size)
{
	return InHeightmap[X * Size.X + Y];
}

float UTGSteepness::GetHeight(const float InHeight)
{
	return InHeight;
}
