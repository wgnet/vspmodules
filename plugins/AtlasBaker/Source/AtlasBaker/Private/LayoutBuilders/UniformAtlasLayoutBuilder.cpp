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
#include "UniformAtlasLayoutBuilder.h"

FAtlasLayout UUniformAtlasLayoutBuilder::Build(const TArray<UTexture2D*>& TexturesToPack)
{
	FAtlasLayout Output;
	Output.SizeX = GetTextureResolution(ResolutionX);
	Output.SizeY = GetTextureResolution(ResolutionY);

	const FVector2D TexelSize = FVector2D {
		float(Output.SizeX) / float(Dimension),
		float(Output.SizeX) / float(Dimension)
	};
	for (int32 i = 0; i < TexturesToPack.Num(); ++i)
	{
		const int32 Column = i % Dimension;
		const int32 Row = i / Dimension;
		const FVector2D ScreenPosition = FVector2D(TexelSize.X * Column, TexelSize.Y * Row) + BorderSize;

		Output.Slots.Add(FAtlasSlot(
			TexturesToPack[i],
			FVector2D(Output.SizeX, Output.SizeY),
			FBox2D(ScreenPosition, ScreenPosition + TexelSize - 2 * BorderSize)));
	}

	return Output;
}

void UUniformAtlasLayoutBuilder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const int32 XSize = GetTextureResolution(ResolutionX);
	const int32 YSize = GetTextureResolution(ResolutionY);

	if (XSize != YSize)
	{
		if (Dimension % 2 != 0)
		{
			Dimension++;
		}

		if (XSize < YSize)
		{
			ResolutionY = ResolutionX;
		}

		if (XSize / YSize > 2)
		{
			ResolutionX = TEnumAsByte<ETextureAtlasResolution>(ResolutionY.GetValue() + 1);
		}
	}
}

int32 UUniformAtlasLayoutBuilder::GetTextureResolution(ETextureAtlasResolution Select)
{
	switch (Select)
	{
	case ETR_32:
		return 32;
	case ETR_64:
		return 64;
	case ETR_128:
		return 128;
	case ETR_256:
		return 256;
	case ETR_512:
		return 512;
	case ETR_1024:
		return 1024;
	case ETR_2048:
		return 2048;
	case ETR_4096:
		return 4096;
	default:
		return 32;
	}
}