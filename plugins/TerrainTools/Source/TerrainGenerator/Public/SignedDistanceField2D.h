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

typedef TArray<TArray<FIntPoint>> FTGGrid;

class FSignedDistanceField2D
{
public:
	FSignedDistanceField2D(int32 InScale, int32 InWidth, int32 InHeight)
		: Scale { InScale }
		, Width { InWidth }
		, Height { InHeight }
	{
	}

	TArray<uint8> Generate(TArray<uint8>& InData);

private:
	FIntPoint GetPoint(FTGGrid& SdfGrid, const int32 X, const int32 Y) const;
	void PutPoint(FTGGrid& SdfGrid, const int32 X, const int32 Y, const FIntPoint& SdfPoint);
	void Compare(
		FTGGrid& SdfGrid,
		FIntPoint& SdfPoint,
		const int32 X,
		const int32 Y,
		const int32 OffsetX,
		const int32 OffsetY) const;
	void GenerateSDF(FTGGrid& SdfGrid);

	int32 Scale;
	int32 Width;
	int32 Height;

	const FIntPoint Inside = FIntPoint::ZeroValue;
	const FIntPoint Empty = FIntPoint { 9999, 9999 };
};
