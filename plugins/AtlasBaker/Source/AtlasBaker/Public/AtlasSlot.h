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
#include "UObject/NoExportTypes.h"
#include "AtlasSlot.generated.h"

USTRUCT()
struct FAtlasSlot
{
	GENERATED_BODY()

	UPROPERTY()
	FBox2D Region = ForceInitToZero;

	UPROPERTY(VisibleAnywhere)
	UTexture2D* Texture = nullptr;

	UPROPERTY(VisibleAnywhere)
	FVector2D UVMin = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere)
	FVector2D UVMax = FVector2D::ZeroVector;

	UPROPERTY(VisibleAnywhere)
	FVector2D UVScale = FVector2D::ZeroVector;

	FAtlasSlot() = default;
	FAtlasSlot(UTexture2D* InTexture, const FVector2D& InAtlasSize, const FBox2D& InRegion);
};
