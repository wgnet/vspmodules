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
#include "SlateAtlasTileThumbnailRenderer.h"
#include "Engine/Public/CanvasItem.h"
#include "SlateAtlasTile.h"
#include "ThumbnailRendering/ThumbnailManager.h"

void USlateAtlasTileThumbnailRenderer::Draw(
	UObject* Object,
	int32 X,
	int32 Y,
	uint32 Width,
	uint32 Height,
	FRenderTarget* RenderTarget,
	FCanvas* Canvas,
	bool bAdditionalViewFamily)
{
	USlateAtlasTile* SlateAtlasTile = Cast<USlateAtlasTile>(Object);
	if (IsValid(SlateAtlasTile) && IsValid(SlateAtlasTile->Texture))
	{
		const int32 CheckerDensity = 8;
		UTexture2D* Checker = UThumbnailManager::Get().CheckerboardTexture;
		Canvas->DrawTile(
			0.0f,
			0.0f,
			Width,
			Height,
			0.0f,
			0.0f,
			CheckerDensity,
			CheckerDensity,
			FLinearColor::White,
			Checker->Resource);

		FCanvasTileItem CanvasTile(
			FVector2D(X, Y),
			SlateAtlasTile->Texture->Resource,
			FVector2D(Width, Height),
			SlateAtlasTile->Region.Min,
			SlateAtlasTile->Region.Max,
			FLinearColor::White);
		CanvasTile.BlendMode = SE_BLEND_Translucent;
		CanvasTile.Draw(Canvas);
	}
}
