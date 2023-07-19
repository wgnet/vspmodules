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
#include "NonuniformAtlasLayoutBuilder.h"
#include "Algo/Accumulate.h"
#include "Algo/Copy.h"
#include "Algo/Transform.h"

FAtlasLayout UNonuniformAtlasLayoutBuilder::Build(const TArray<UTexture2D*>& TexturesToPack)
{
	FAtlasLayout LayoutOutput;
	FVector2D Size = PredictTextureSize(TexturesToPack);
	bool bSuccess = false;
	while (!bSuccess)
	{
		bSuccess = TryPackInSize(TexturesToPack, Size, LayoutOutput);
		if (!bSuccess)
		{
			Size = ResizeUp(Size);
		}
	}

	return LayoutOutput;
}

UNonuniformAtlasLayoutBuilder::FNode::FNode(const FBox2D& Region)
	: Region(Region)
	, AChild(nullptr)
	, BChild(nullptr)
	, InsertedTexture(nullptr)
{
}

UNonuniformAtlasLayoutBuilder::FNode::~FNode()
{
	if (AChild)
	{
		delete AChild;
	}

	if (BChild)
	{
		delete BChild;
	}
}

bool UNonuniformAtlasLayoutBuilder::FNode::InsertTexture(UTexture2D* Texture, int32 Border, FBox2D& OutRegion)
{
	if (InsertedTexture)
		return false;

	const FVector2D NeededRegionSize = FVector2D(Texture->GetSizeX(), Texture->GetSizeY()) + 2 * Border;
	const FVector2D NodeSize = Region.GetSize();
	const FVector2D SizeDifference = NodeSize - NeededRegionSize;

	if (SizeDifference.X < 0 || SizeDifference.Y < 0)
		return false;

	if (IsLeaf())
	{
		if (SizeDifference.X <= 1 && SizeDifference.Y <= 1)
		{
			InsertedTexture = Texture;
			OutRegion = FBox2D(Region.Min + Border, Region.Max - Border);
			return true;
		}


		if (SizeDifference.X > SizeDifference.Y)
		{
			AChild = new FNode(FBox2D(Region.Min, FVector2D(Region.Min.X + NeededRegionSize.X, Region.Max.Y)));
			BChild = new FNode(FBox2D(FVector2D(Region.Min.X + NeededRegionSize.X, Region.Min.Y), Region.Max));
		}
		else
		{
			AChild = new FNode(FBox2D(Region.Min, FVector2D(Region.Max.X, Region.Min.Y + NeededRegionSize.Y)));
			BChild = new FNode(FBox2D(FVector2D(Region.Min.X, Region.Min.Y + NeededRegionSize.Y), Region.Max));
		}

		return AChild->InsertTexture(Texture, Border, OutRegion);
	}


	return AChild->InsertTexture(Texture, Border, OutRegion) || BChild->InsertTexture(Texture, Border, OutRegion);
}

bool UNonuniformAtlasLayoutBuilder::FNode::FindTextureRegion(UTexture2D* Texture, FBox2D& OutRegion) const
{
	if (InsertedTexture == Texture)
	{
		OutRegion = Region;
		return true;
	}

	return (AChild && AChild->FindTextureRegion(Texture, OutRegion))
		|| (BChild && BChild->FindTextureRegion(Texture, OutRegion));
}

bool UNonuniformAtlasLayoutBuilder::FNode::IsLeaf() const
{
	return !AChild && !BChild;
}

bool UNonuniformAtlasLayoutBuilder::TryPackInSize(
	const TArray<UTexture2D*>& TexturesToPack,
	const FVector2D& Size,
	FAtlasLayout& OutLayout) const
{
	struct FTextureContext
	{
		UTexture2D* Texture;
		int32 Idx;
		FBox2D Region;
	};

	int32 i = 0;
	TArray<FTextureContext> Contexts;
	for (UTexture2D* Texture : TexturesToPack)
	{
		Contexts.Add({ Texture, i++, FBox2D() });
	}
	Contexts.Sort(
		[](const FTextureContext& A, const FTextureContext& B)
		{
			return A.Texture->GetSizeY() > B.Texture->GetSizeY();
		});

	bool bSuccess = true;
	FNode Root = FNode(FBox2D(FVector2D::ZeroVector, FVector2D(Size.X, Size.Y)));
	for (FTextureContext& TextureContext : Contexts)
	{
		bSuccess &= Root.InsertTexture(TextureContext.Texture, BorderSize, TextureContext.Region);
		if (!bSuccess)
			break;
	}

	if (bSuccess)
	{
		OutLayout.SizeX = Size.X;
		OutLayout.SizeY = Size.Y;

		Contexts.Sort(
			[](const FTextureContext& A, const FTextureContext& B)
			{
				return A.Idx < B.Idx;
			});
		Algo::Transform(
			Contexts,
			OutLayout.Slots,
			[&Size](FTextureContext Context)
			{
				return FAtlasSlot(Context.Texture, Size, Context.Region);
			});
	}

	return bSuccess;
}

FVector2D UNonuniformAtlasLayoutBuilder::PredictTextureSize(const TArray<UTexture2D*>& TexturesToPack) const
{
	const int32 TotalWidth = Algo::Accumulate(
		TexturesToPack,
		0,
		[](int32 AccumulatedWidth, UTexture2D* Texture)
		{
			return AccumulatedWidth + Texture->GetSizeX();
		});

	const int32 MaxHeight = Algo::Accumulate(
		TexturesToPack,
		0,
		[](int32 MaxHeight, UTexture2D* Texture)
		{
			return FMath::Max(MaxHeight, Texture->GetSizeY());
		});

	float OutWidth = TotalWidth;
	float OutHeight = MaxHeight;
	while (static_cast<float>(OutWidth) / OutHeight > 2)
	{
		OutHeight += MaxHeight;
		OutWidth /= 2;
	}

	return FVector2D(FMath::RoundUpToPowerOfTwo(OutWidth), FMath::RoundUpToPowerOfTwo(OutHeight));
}

FVector2D UNonuniformAtlasLayoutBuilder::ResizeUp(const FVector2D& OldSize) const
{
	float XPowerOfTwo = FMath::Log2(OldSize.X);
	float YPowerOfTwo = FMath::Log2(OldSize.Y);

	if (XPowerOfTwo <= YPowerOfTwo)
	{
		XPowerOfTwo += 1;
	}
	else
	{
		YPowerOfTwo += 1;
	}

	return FVector2D(FMath::Pow(2, XPowerOfTwo), FMath::Pow(2, YPowerOfTwo));
}
