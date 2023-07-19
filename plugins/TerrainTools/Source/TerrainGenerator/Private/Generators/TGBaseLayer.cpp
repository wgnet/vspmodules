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

#include "Generators/TGBaseLayer.h"

#include "Algo/MinElement.h"
#include "GeneratedCodeHelpers.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "TerrainGeneratorUtils.h"


UTGBaseLayer::UTGBaseLayer()
{
}

void UTGBaseLayer::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(UTGBaseLayer, AffectedLayer))
	{
		CachedAffectedLayer = AffectedLayer;
	}
}

UTextureRenderTarget2D* UTGBaseLayer::Generate(const FTGTerrainInfo& TerrainInfo)
{
	return GetRenderTarget();
}

UTextureRenderTarget2D* UTGBaseLayer::GetRenderTarget() const
{
	return RenderTarget;
}

UTextureRenderTarget2D* UTGBaseLayer::DrawWeightmapR8(const TArray<uint8>& InData, FName Name, FIntPoint Resolution)
{
	RenderTarget = FTerrainUtils::GetOrCreateTransientRenderTarget2D(GetRenderTarget(), Name, Resolution, RTF_R8);
	FTextureRenderTargetResource* RTResourceDest = RenderTarget->GameThread_GetRenderTargetResource();


	ENQUEUE_RENDER_COMMAND(UpdateTexture2D)
	(
		[&RTResourceDest, Data = InData](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.UpdateTexture2D(
				RTResourceDest->GetTexture2DRHI(),
				0,
				FUpdateTextureRegion2D(0, 0, 0, 0, RTResourceDest->GetSizeX(), RTResourceDest->GetSizeY()),
				RTResourceDest->GetSizeX() * sizeof(uint8),
				Data.GetData());
		});

	FlushRenderingCommands();
	return RenderTarget;
}

UTextureRenderTarget2D* UTGBaseLayer::DrawWeightmapRGBA8(const TArray<uint8>& InData, FName Name, FIntPoint Resolution)
{
	RenderTarget = FTerrainUtils::GetOrCreateTransientRenderTarget2D(GetRenderTarget(), Name, Resolution, RTF_RGBA8);
	FTextureRenderTargetResource* RTResourceDest = RenderTarget->GameThread_GetRenderTargetResource();

	TArray<uint8> NewData;
	NewData.SetNum(InData.Num() * 4);
	for (int32 i = 0; i < InData.Num(); i++)
	{
		NewData[i * 4 + 0] = MIN_uint8; // B
		NewData[i * 4 + 1] = MIN_uint8; // G
		NewData[i * 4 + 2] = InData[i]; // R
		NewData[i * 4 + 3] = MAX_uint8; // A
	}

	ENQUEUE_RENDER_COMMAND(UpdateTexture2D)
	(
		[&RTResourceDest, Data = NewData](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.UpdateTexture2D(
				RTResourceDest->GetTexture2DRHI(),
				0,
				FUpdateTextureRegion2D(0, 0, 0, 0, RTResourceDest->GetSizeX(), RTResourceDest->GetSizeY()),
				RTResourceDest->GetSizeX() * sizeof(FColor),
				Data.GetData());
		});

	FlushRenderingCommands();
	return GetRenderTarget();
}

UTextureRenderTarget2D* UTGBaseLayer::DrawWeightmap(
	const TArray<uint8>& InData,
	FName Name,
	FIntPoint Resolution,
	bool bR8)
{
	if (bR8)
		return DrawWeightmapR8(InData, Name, Resolution);
	else
		return DrawWeightmapRGBA8(InData, Name, Resolution);
}

void UTGBaseLayer::ApplyAdjustments(UMaterialInstanceDynamic* MID) const
{
	MID->SetScalarParameterValue(TEXT("Invert"), bInvert);
	MID->SetScalarParameterValue(TEXT("Falloff"), Falloff);
	MID->SetScalarParameterValue(TEXT("Intensity"), Intensity);
}

float UTGBaseLayer::ApplyAdjustments(float Value) const
{
	return FMath::Pow(Value * Intensity, Falloff);
}

uint8 UTGBaseLayer::ApplyAdjustments(uint8 Value) const
{
	if (bInvert)
		return MAX_uint8
			- (uint8)FMath::Clamp(FMath::Pow(Value * Intensity, Falloff), (float)MIN_uint8, (float)MAX_uint8);
	else
		return (uint8)FMath::Clamp(FMath::Pow(Value * Intensity, Falloff), (float)MIN_uint8, (float)MAX_uint8);
}

uint8 UTGBaseLayer::ToWeight(const float Value)
{
	float FloatR = FMath::Clamp(Value, 0.0f, 1.0f);
	return (uint8)FMath::FloorToInt(Value * 255.999f);
}
