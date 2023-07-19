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


#include "SignedDistanceField2D.h"

#include "Misc/ScopedSlowTask.h"


TArray<uint8> FSignedDistanceField2D::Generate(TArray<uint8>& InData)
{
	FTGGrid Grid1;
	Grid1.SetNum(Width);
	for (auto& Iter : Grid1)
	{
		Iter.SetNum(Height);
	}

	FTGGrid Grid2;
	Grid2.SetNum(Width);
	for (auto& Iter : Grid2)
	{
		Iter.SetNum(Height);
	}

	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			// Points inside get marked with a dx/dy of zero.
			// Points outside get marked with an infinitely large distance.
			if (InData[Y * Width + X] < 128)
			{
				PutPoint(Grid1, X, Y, Inside);
				PutPoint(Grid2, X, Y, Empty);
			}
			else
			{
				PutPoint(Grid2, X, Y, Inside);
				PutPoint(Grid1, X, Y, Empty);
			}
		}
	}

	// Generate the SDF.
	GenerateSDF(Grid1);
	GenerateSDF(Grid2);

	// Render out the results.
	TArray<uint8> OutData;
	OutData.SetNum(Height * Width);
	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			// Calculate the actual distance from the dx/dy
			const int32 Dist1 = (int32)(sqrt((double)GetPoint(Grid1, X, Y).SizeSquared()));
			const int32 Dist2 = (int32)(sqrt((double)GetPoint(Grid2, X, Y).SizeSquared()));
			const int32 Dist = Dist1 - Dist2;

			// Clamp and scale
			int32 C = Dist * Scale + 128;
			C = FMath::Clamp(C, 0, 255);

			OutData[Y * Width + X] = C;
		}
	}

	return OutData;
}

FIntPoint FSignedDistanceField2D::GetPoint(FTGGrid& SdfGrid, const int32 X, const int32 Y) const
{
	// have a 1-pixel gutter.
	if (X >= 0 && Y >= 0 && X < Width && Y < Height)
		return SdfGrid[Y][X];
	else
		return Empty;
}

void FSignedDistanceField2D::PutPoint(FTGGrid& SdfGrid, const int32 X, const int32 Y, const FIntPoint& SdfPoint)
{
	SdfGrid[Y][X] = SdfPoint;
}

void FSignedDistanceField2D::Compare(
	FTGGrid& SdfGrid,
	FIntPoint& SdfPoint,
	const int32 X,
	const int32 Y,
	const int32 OffsetX,
	const int32 OffsetY) const
{
	FIntPoint OtherPoint = GetPoint(SdfGrid, X + OffsetX, Y + OffsetY);
	OtherPoint.X += OffsetX;
	OtherPoint.Y += OffsetY;

	if (OtherPoint.SizeSquared() < SdfPoint.SizeSquared())
		SdfPoint = OtherPoint;
}

void FSignedDistanceField2D::GenerateSDF(FTGGrid& SdfGrid)
{
	// Pass 0
	for (int32 Y = 0; Y < Height; Y++)
	{
		for (int32 X = 0; X < Width; X++)
		{
			FIntPoint Point = GetPoint(SdfGrid, X, Y);
			Compare(SdfGrid, Point, X, Y, -1, 0);
			Compare(SdfGrid, Point, X, Y, 0, -1);
			Compare(SdfGrid, Point, X, Y, -1, -1);
			Compare(SdfGrid, Point, X, Y, 1, -1);
			PutPoint(SdfGrid, X, Y, Point);
		}

		for (int32 X = Width - 1; X >= 0; X--)
		{
			FIntPoint Point = GetPoint(SdfGrid, X, Y);
			Compare(SdfGrid, Point, X, Y, 1, 0);
			PutPoint(SdfGrid, X, Y, Point);
		}
	}

	// Pass 1
	for (int32 Y = Height - 1; Y >= 0; Y--)
	{
		for (int32 X = Width - 1; X >= 0; X--)
		{
			FIntPoint Point = GetPoint(SdfGrid, X, Y);
			Compare(SdfGrid, Point, X, Y, 1, 0);
			Compare(SdfGrid, Point, X, Y, 0, 1);
			Compare(SdfGrid, Point, X, Y, -1, 1);
			Compare(SdfGrid, Point, X, Y, 1, 1);
			PutPoint(SdfGrid, X, Y, Point);
		}

		for (int32 X = 0; X < Width; X++)
		{
			FIntPoint Point = GetPoint(SdfGrid, X, Y);
			Compare(SdfGrid, Point, X, Y, -1, 0);
			PutPoint(SdfGrid, X, Y, Point);
		}
	}
}
