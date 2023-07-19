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


#include "Generators/TGCustomWeight.h"

#include "Algo/Transform.h"
#include "CanvasItem.h"
#include "Engine/TextureRenderTarget2D.h"
#include "K2Node_GetClassDefaults.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "TerrainGeneratorHelper.h"
#include "TerrainGeneratorUtils.h"

UTGCustomWeight::UTGCustomWeight()
{
	GeneratorName = TEXT("CustomWeight");
}

UTextureRenderTarget2D* UTGCustomWeight::Generate(const FTGTerrainInfo& TerrainInfo)
{
	if (Mask)
	{
		RenderTarget = FTerrainUtils::GetOrCreateTransientRenderTarget2D(
			GetRenderTarget(),
			GeneratorName,
			TerrainInfo.Size,
			RTF_RGBA8);
		UTerrainGeneratorHelper::CopyTextureToRenderTargetTexture(
			Mask.Get(),
			GetRenderTarget(),
			GEditor->GetEditorWorldContext().World()->FeatureLevel);

		FTextureRenderTargetResource* RenderTargetResource = RenderTarget->GameThread_GetRenderTargetResource();
		const FLinearColor Rect =
			FLinearColor(0, 0, RenderTargetResource->GetSizeX(), RenderTargetResource->GetSizeY());
		TArray<FLinearColor> TexPixels = UTerrainGeneratorHelper::SampleRTData(GetRenderTarget(), Rect);

		TArray<uint8> RenderData;
		RenderData.Reserve(TexPixels.Num());
		for (float Element : FLinearColors_ChannelRange { TexPixels, GetChannel() })
		{
			RenderData.Emplace_GetRef() = ToWeight(ApplyAdjustments(Element));
		}

		RenderTarget = DrawWeightmap(
			RenderData,
			GeneratorName,
			FIntPoint { RenderTarget->SizeX, RenderTarget->SizeY },
			TerrainInfo.bFullRange);

		return GetRenderTarget();
	}

	return nullptr;
}

float FLinearColor::*UTGCustomWeight::GetChannel() const
{
	switch (UseChannel)
	{
	case R:
		return &FLinearColor::R;
	case G:
		return &FLinearColor::G;
	case B:
		return &FLinearColor::B;
	case A:
		return &FLinearColor::A;
	}
	return 0;
}
