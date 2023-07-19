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

#include "Engine/TextureRenderTarget2D.h"

class ALandscapeProxy;
class ALandscape;

struct FFluidSurfaceUtils
{
	static UTextureRenderTarget2D* GetOrCreateTransientRenderTarget2D(
		UTextureRenderTarget2D* InRenderTarget,
		FName InRenderTargetName,
		const FIntPoint& InSize,
		ETextureRenderTargetFormat InFormat,
		const FLinearColor& InClearColor = FLinearColor::Black,
		bool bInAutoGenerateMipMaps = false);

	static FCollisionQueryParams ConfigureCollisionParams(
		FName TraceTag,
		bool bTraceComplex,
		const TArray<AActor*>& ActorsToIgnore,
		bool bIgnoreSelf,
		const UObject* WorldContextObject);

	static FCollisionObjectQueryParams ConfigureCollisionObjectParams(
		const TArray<TEnumAsByte<EObjectTypeQuery>>& ObjectTypes);

	static void GetLandscapeCenterAndExtent(
		const ALandscape* InLandscape,
		FVector& OutCenter,
		FVector& OutExtent,
		int32& OutSubsections,
		int32& OutQuadsSize);

	static float GetLandscapeHeightAtLocation(const ALandscape* InLandscape, const FVector& InLocation);

	static ALandscapeProxy* FindLandscape(const AActor* ContextActor);
};
