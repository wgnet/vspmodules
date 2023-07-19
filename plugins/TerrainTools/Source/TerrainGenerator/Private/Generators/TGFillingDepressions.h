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

// Planchon and Darboux Algorithm for Filling Depressions

#pragma once

#include "TGBaseLayer.h"
#include "TGFillingDepressions.generated.h"

struct FElevationModel
{
	TArray<float> Elevation;
	int32 Width = 0;
	int32 Height = 0;
	const float NoDataValue = MIN_flt;

	FElevationModel(const int32 InWidth, const int32 InHeight)
	{
		Width = InWidth;
		Height = InHeight;
		Elevation.SetNum(InWidth * InHeight);
	}

	bool IsNoData(const int32 Row, const int32 Col)
	{
		if (fabs(Elevation[Row * Width + Col] - NoDataValue) < DELTA)
			return true;
		return false;
	}

	void SetValue(const int32 Row, const int32 Col, const float Z)
	{
		Elevation[Row * Width + Col] = Z;
	}

	float GetValue(const int32 Row, const int32 Col)
	{
		return Elevation[Row * Width + Col];
	}

	void SetNoData()
	{
		for (int32 i = 0; i < Elevation.Num(); i++)
		{
			Elevation[i] = NoDataValue;
		}
	}

	bool IsInGrid(const int32 Row, const int32 Col) const
	{
		if ((Row >= 0 && Row < Height) && (Col >= 0 && Col < Width))
			return true;
		return false;
	}

	/*
	// TODO Flow Length
	static float GetFlowLength( const int32 Dir )
	{
		if ( ( Dir & 0x1 ) == 1 )
		{
			return UE_SQRT_2;
		}
		return 1.0f;
	}
*/
	/*
	// TODO Compute Flow Directoin 
	int32 GetFlowDirction( int32 Row, int32 Col, float Spill )
	{
		int32 IRow = 0;
		int32 ICol = 0;
		float ISpill = 0.f;
		float MaxValue = 0.f;
		float Gradient = 0.f;
		int32 SteepestSpill = 255;

		unsigned char lastIndexINGridNoData = 0;
		for ( int32 i = 0; i < 8; i++ )
		{
			IRow = GetRowTo( i, Row );
			ICol = GetColTo( i, Col );

			if ( IsInGrid( IRow, ICol ) && !IsNoData( IRow, ICol ) && ( ISpill = GetValue( IRow, ICol ) ) < Spill )
			{
				Gradient = ( Spill - ISpill ) / GetFlowLength( i );
				if ( MaxValue < Gradient )
				{
					MaxValue = Gradient;
					SteepestSpill = i;
				}
			}
			if ( !IsInGrid( IRow, ICol ) || IsNoData( IRow, ICol ) )
			{
				lastIndexINGridNoData = i;
			}
		}
		return SteepestSpill != 255 ? FlowDirection[ SteepestSpill ] : FlowDirection[ lastIndexINGridNoData ];
	}
*/
};

UCLASS(EditInlineNew)
class UTGFillingDepressions : public UTGBaseLayer
{
	GENERATED_BODY()

public:
	UTGFillingDepressions();

	virtual UTextureRenderTarget2D* Generate(const FTGTerrainInfo& TerrainInfo) override;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = 0, ClampMax = 100), Category = "FillingDepressions")
	int32 MinHeight = 0;

	UPROPERTY(EditInstanceOnly, meta = (ClampMin = 0, ClampMax = 100), Category = "FillingDepressions")
	int32 MaxHeight = 100;

	UPROPERTY(VisibleInstanceOnly, Category = "FillingDepressions")
	float Epsilon = 0.f;

	TArray<float> FillDepressions(const FTGTerrainInfo& TerrainInfo) const;

private:
	/*
	*	Neighbor Index
	*	5  6  7
	*	4     0
	*	3  2  1
	*/

	const int32 NeighborsX[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
	const int32 NeighborsY[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };

	/*
	*	Reverse of Flow Directions
	*	2	4	8
	*	1	0	16
	*	128	64	32
	*/

	const int32 ReverseFlowDirection[8] = { 16, 32, 64, 128, 1, 2, 4, 8 };

	/*
	*	Flow Direction		
	*	32	64	128		
	*	16	0	1		
	*	8	4	2		
	*/

	const int32 FlowDirection[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };


	int32 GetRowTo(const int32 Dir, const int32 Row) const
	{
		return (Row + NeighborsX[Dir]);
	}

	int32 GetColTo(const int32 Dir, const int32 Col) const
	{
		return (Col + NeighborsY[Dir]);
	}
};
