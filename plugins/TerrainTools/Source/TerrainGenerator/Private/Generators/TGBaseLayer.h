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
#include "Algo/MaxElement.h"
#include "Algo/MinElement.h"
#include "GameFramework/Actor.h"
#include "TGBaseLayer.generated.h"

struct FTGTerrainInfo
{
	TArray<uint8> ObjectsMask;
	TArray<float> DecodedHeight;
	FIntPoint Size;
	bool bFullRange = false;
};

class FLinearColors_ChannelRange
{
public:
	class FIterator
	{
	public:
		FIterator(
			const TArray<FLinearColor>::RangedForConstIteratorType& InParentIterator,
			float FLinearColor::*InColorFieldPointer)
			: ParentIterator(InParentIterator)
			, ColorFieldPointer(InColorFieldPointer)
		{
		}

		const float& operator*() const
		{
			return (*ParentIterator).*ColorFieldPointer;
		}

		void operator++()
		{
			++ParentIterator;
		}

		bool operator!=(const FIterator& Other) const
		{
			return (ParentIterator != Other.ParentIterator);
		}

	private:
		TArray<FLinearColor>::RangedForConstIteratorType ParentIterator;
		float FLinearColor::*ColorFieldPointer;
	};

	FLinearColors_ChannelRange(const TArray<FLinearColor>& InParentRange, float FLinearColor::*InColorFieldPointer)
		: ParentRange(InParentRange)
		, ColorFieldPointer(InColorFieldPointer)
	{
	}

	FIterator begin() const
	{
		return FIterator { ParentRange.begin(), ColorFieldPointer };
	}

	FIterator end() const
	{
		return FIterator { ParentRange.end(), ColorFieldPointer };
	}

	int32 Num() const
	{
		return ParentRange.Num();
	}

private:
	const TArray<FLinearColor>& ParentRange;
	float FLinearColor::*ColorFieldPointer;
};


UCLASS(hidecategories = (LOD, Rendering, Replication, Collision, Input, Actor, Cooking))
class UTGBaseLayer : public UObject
{
	GENERATED_BODY()

public:
	UTGBaseLayer();

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	virtual UTextureRenderTarget2D* Generate(const FTGTerrainInfo& TerrainInfo);

	UPROPERTY(VisibleInstanceOnly, Category = "Adjustments")
	FName AffectedLayer;

	UPROPERTY(EditInstanceOnly, Category = "Adjustments")
	bool bInvert = false;

	UPROPERTY(EditInstanceOnly, Category = "Adjustments")
	float Falloff = 1.f;

	UPROPERTY(EditInstanceOnly, Category = "Adjustments")
	float Intensity = 1.f;

	UTextureRenderTarget2D* GetRenderTarget() const;
	;

protected:
	UTextureRenderTarget2D* DrawWeightmapR8(const TArray<uint8>& InData, FName Name, FIntPoint Resolution);
	UTextureRenderTarget2D* DrawWeightmapRGBA8(const TArray<uint8>& InData, FName Name, FIntPoint Resolution);
	UTextureRenderTarget2D* DrawWeightmap(
		const TArray<uint8>& InData,
		FName Name,
		FIntPoint Resolution,
		bool bR8 = true);

	void ApplyAdjustments(UMaterialInstanceDynamic* MID) const;
	float ApplyAdjustments(float Value) const;
	uint8 ApplyAdjustments(uint8 Value) const;
	static uint8 ToWeight(float Value);
	template<typename Range>
	static TArray<uint8> RemapToWeight(const Range& InData)
	{
		const float MinValue = *Algo::MinElement(InData);
		const float MaxValue = *Algo::MaxElement(InData);

		TArray<uint8> OutData;
		OutData.Reserve(InData.Num());
		for (float DataElement : InData)
		{
			OutData.Emplace_GetRef() = static_cast<uint8>(
				((DataElement - MinValue) / (MaxValue - MinValue)) * (MAX_uint8 - MIN_uint8) + MIN_uint8);
		}

		return OutData;
	};

	FName GeneratorName;

	UPROPERTY(VisibleInstanceOnly, Category = "Adjustments")
	UTextureRenderTarget2D* RenderTarget;

private:
	FName CachedAffectedLayer;
};
