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


#include "Generators/TGDistanceField2D.h"

#include "Algo/Transform.h"
#include "Engine/TextureRenderTarget2D.h"
#include "SignedDistanceField2D.h"


UTGDistanceField::UTGDistanceField()
{
	GeneratorName = TEXT("DistanceField");
}

UTextureRenderTarget2D* UTGDistanceField::Generate(const FTGTerrainInfo& TerrainInfo)
{
	if (TerrainInfo.ObjectsMask.Num() > 0)
	{
		TArray<uint8> Data = TerrainInfo.ObjectsMask;
		FSignedDistanceField2D TGSignedDistanceField2D { Scale, TerrainInfo.Size.X, TerrainInfo.Size.Y };
		Data = TGSignedDistanceField2D.Generate(Data);

		for (uint8& Iter : Data)
		{
			Iter = ApplyAdjustments(Iter);
		}

		RenderTarget = DrawWeightmap(Data, GeneratorName, TerrainInfo.Size, TerrainInfo.bFullRange);
	}

	return GetRenderTarget();
}
