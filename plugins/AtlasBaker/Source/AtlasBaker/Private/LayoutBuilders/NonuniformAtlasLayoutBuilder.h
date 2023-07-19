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
#include "NonuniformAtlasLayoutBuilder.generated.h"

class UTexture2D;

UCLASS(EditInlineNew)
class UNonuniformAtlasLayoutBuilder : public UAtlasLayoutBuilder
{
	GENERATED_BODY()

public:
	FAtlasLayout Build(const TArray<UTexture2D*>& TexturesToPack) override;

private:
	struct FNode
	{
		FBox2D Region;
		FNode* AChild;
		FNode* BChild;
		UTexture2D* InsertedTexture;

		FNode(const FBox2D& Region);
		~FNode();

		bool InsertTexture(UTexture2D* Texture, int32 Border, FBox2D& OutRegion);
		bool FindTextureRegion(UTexture2D* Texture, FBox2D& OutRegion) const;
		bool IsLeaf() const;
	};

	bool TryPackInSize(const TArray<UTexture2D*>& TexturesToPack, const FVector2D& Size, FAtlasLayout& OutLayout) const;
	FVector2D PredictTextureSize(const TArray<UTexture2D*>& TexturesToPack) const;
	FVector2D ResizeUp(const FVector2D& OldSize) const;
};