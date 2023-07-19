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


#include "Generators/TGFillingDepressions.h"

#include "Kismet/KismetRenderingLibrary.h"
#include <queue>


UTGFillingDepressions::UTGFillingDepressions()
{
	GeneratorName = TEXT("FillingDepressions");
}


UTextureRenderTarget2D* UTGFillingDepressions::Generate(const FTGTerrainInfo& TerrainInfo)
{
	TArray<float> Elevation = FillDepressions(TerrainInfo);

	for (int32 Iter = 0; Iter < Elevation.Num(); Iter++)
	{
		Elevation[Iter] -= TerrainInfo.DecodedHeight[Iter];
	}

	MinHeight = FMath::Min(MinHeight, MaxHeight);
	MaxHeight = FMath::Max(MinHeight, MaxHeight);
	const float HMax = FMath::Max(Elevation);

	for (float& Iter : Elevation)
	{
		Iter = ApplyAdjustments(Iter);
		Iter = FMath::Clamp(Iter, MinHeight / 100.f * HMax, MaxHeight / 100.f * HMax);
	}

	RenderTarget = DrawWeightmap(RemapToWeight(Elevation), GeneratorName, TerrainInfo.Size, TerrainInfo.bFullRange);

	return GetRenderTarget();
}


TArray<float> UTGFillingDepressions::FillDepressions(const FTGTerrainInfo& TerrainInfo) const
{
	// Stage 1: Initialization of the surface to m

	const int32 Height = TerrainInfo.Size.X;
	const int32 Width = TerrainInfo.Size.Y;

	FElevationModel ElevationModel { Width, Height };
	ElevationModel.Elevation = TerrainInfo.DecodedHeight;
	const float Volume = FMath::Max(TerrainInfo.DecodedHeight);
	// A transient surface that converging to the depression-filled DEM
	FElevationModel Water { Width, Height };
	Water.SetNoData();

	std::queue<FIntPoint> KnownCells;

	for (int32 Row = 0; Row < Height; ++Row)
	{
		for (int32 Col = 0; Col < Width; ++Col)
		{
			if (ElevationModel.IsNoData(Row, Col))
			{
				Water.SetValue(Row, Col, ElevationModel.GetValue(Row, Col));
			}
			else
			{
				bool IsBorder = false;
				for (int32 i = 0; i < 8; ++i)
				{
					const int32 IRow = GetRowTo(i, Row);
					const int32 ICol = GetColTo(i, Col);

					if (!ElevationModel.IsInGrid(IRow, ICol) || ElevationModel.IsNoData(IRow, ICol))
					{
						IsBorder = true;
						break;
					}
				}

				if (IsBorder)
				{
					Water.SetValue(Row, Col, ElevationModel.GetValue(Row, Col));
					FIntPoint Node(Row, Col);
					KnownCells.push(Node);
				}
				else
				{
					Water.SetValue(Row, Col, Volume);
				}
			}
		}
	}

	// filling depressions...

	// Stage 2:Removal of excess water

	std::queue<FIntPoint> UnknownCells;

	while (!KnownCells.empty() || !UnknownCells.empty())
	{
		FIntPoint Node = FIntPoint { -1, -1 };

		if (!KnownCells.empty())
		{
			Node = KnownCells.front();
			KnownCells.pop();
		}
		else if (!UnknownCells.empty())
		{
			Node = UnknownCells.front();
			UnknownCells.pop();
			if (ElevationModel.GetValue(Node.X, Node.Y) >= Water.GetValue(Node.X, Node.Y))
				continue;
		}

		const float Spill = Water.GetValue(Node.X, Node.Y);

		for (int i = 0; i < 8; ++i)
		{
			const int32 IRow = GetRowTo(i, Node.X);
			const int32 ICol = GetColTo(i, Node.Y);

			if (!ElevationModel.IsInGrid(IRow, ICol)
				|| ElevationModel.GetValue(IRow, ICol) >= Water.GetValue(IRow, ICol))
				continue;

			if (ElevationModel.GetValue(IRow, ICol) >= Spill + Epsilon)
			{
				// Dried Cell
				Water.SetValue(IRow, ICol, ElevationModel.GetValue(IRow, ICol));
				FIntPoint Temp(IRow, ICol);
				KnownCells.push(Temp);
			}
			else if (Water.GetValue(IRow, ICol) > Spill + Epsilon)
			{
				// Remove Excess Water
				Water.SetValue(IRow, ICol, Spill + Epsilon);
				FIntPoint Temp(IRow, ICol);
				UnknownCells.push(Temp);
			}
		}
	}

	return Water.Elevation;
}
