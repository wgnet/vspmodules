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
#include "AtlasLayoutBuilder.h"
#include "UObject/NoExportTypes.h"
#include "UniformAtlasLayoutBuilder.generated.h"

class UTexture2D;

UENUM()
enum ETextureAtlasResolution
{
	ETR_32	 UMETA(DisplayName = "32"),
	ETR_64	 UMETA(DisplayName = "64"),
	ETR_128	 UMETA(DisplayName = "128"),
	ETR_256	 UMETA(DisplayName = "256"),
	ETR_512	 UMETA(DisplayName = "512"),
	ETR_1024 UMETA(DisplayName = "1024"),
	ETR_2048 UMETA(DisplayName = "2048"),
	ETR_4096 UMETA(DisplayName = "4096"),
};

UCLASS(EditInlineNew)
class UUniformAtlasLayoutBuilder : public UAtlasLayoutBuilder
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ETextureAtlasResolution> ResolutionX = ETR_1024;

	UPROPERTY(EditDefaultsOnly)
	TEnumAsByte<ETextureAtlasResolution> ResolutionY = ETR_1024;

	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "1"))
	int32 Dimension = 4;

	FAtlasLayout Build(const TArray<UTexture2D*>& TexturesToPack) override;

private:
	static int32 GetTextureResolution(ETextureAtlasResolution Select);
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
};
