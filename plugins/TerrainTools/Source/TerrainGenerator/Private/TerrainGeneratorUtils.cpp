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


#include "TerrainGeneratorUtils.h"

#include "Async/ParallelFor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Kismet/KismetMathLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"

UMaterialInstanceDynamic* FTerrainUtils::GetOrCreateTransientMID(
	UMaterialInstanceDynamic* InMID,
	FName InMIDName,
	UMaterialInterface* InMaterialInterface,
	EObjectFlags InAdditionalObjectFlags)
{
	if (!IsValid(InMaterialInterface))
	{
		return nullptr;
	}

	UMaterialInstanceDynamic* ResultMID = InMID;

	// If there's no MID yet or if the MID's parent material interface (could be a Material or a MIC) doesn't match the requested material interface (could be a Material, MIC or MID), create a new one :
	if (!IsValid(InMID) || (InMID->Parent != InMaterialInterface))
	{
		// If the requested material is already a UMaterialInstanceDynamic, we can use it as is :
		ResultMID = Cast<UMaterialInstanceDynamic>(InMaterialInterface);

		if (ResultMID != nullptr)
		{
			ensure(EnumHasAnyFlags(
				InMaterialInterface->GetFlags(),
				EObjectFlags::RF_Transient)); // The name of the function implies we're dealing with transient MIDs
		}
		else
		{
			// If it's not a UMaterialInstanceDynamic, it's a UMaterialInstanceConstant or a UMaterial, both of which can be used to create a MID :
			ResultMID = UMaterialInstanceDynamic::Create(
				InMaterialInterface,
				nullptr,
				MakeUniqueObjectName(GetTransientPackage(), UMaterialInstanceDynamic::StaticClass(), InMIDName));
			ResultMID->SetFlags(InAdditionalObjectFlags);
		}
	}

	check(ResultMID != nullptr);
	return ResultMID;
}

UTextureRenderTarget2D* FTerrainUtils::GetOrCreateTransientRenderTarget2D(
	UTextureRenderTarget2D* InRenderTarget,
	const FName InRenderTargetName,
	const FIntPoint& InSize,
	const ETextureRenderTargetFormat InFormat,
	const FLinearColor& InClearColor,
	bool bInAutoGenerateMipMaps)
{
	const EPixelFormat PixelFormat = GetPixelFormatFromRenderTargetFormat(InFormat);
	if ((InSize.X <= 0) || (InSize.Y <= 0) || (PixelFormat == EPixelFormat::PF_Unknown))
	{
		return nullptr;
	}

	if (IsValid(InRenderTarget))
	{
		if ((InRenderTarget->SizeX == InSize.X) && (InRenderTarget->SizeY == InSize.Y)
			&& (InRenderTarget->GetFormat()
				== PixelFormat) // Watch out : GetFormat() returns a EPixelFormat (non-class enum), so we can't compare with a ETextureRenderTargetFormat
			&& (InRenderTarget->ClearColor == InClearColor)
			&& (InRenderTarget->bAutoGenerateMips == bInAutoGenerateMipMaps))
		{
			return InRenderTarget;
		}
	}

	UTextureRenderTarget2D* NewRenderTarget2D = NewObject<UTextureRenderTarget2D>(
		GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UTextureRenderTarget2D::StaticClass(), InRenderTargetName));
	check(NewRenderTarget2D);
	NewRenderTarget2D->RenderTargetFormat = InFormat;
	NewRenderTarget2D->AddressX = TA_Clamp;
	NewRenderTarget2D->AddressY = TA_Clamp;
	NewRenderTarget2D->ClearColor = InClearColor;
	NewRenderTarget2D->bAutoGenerateMips = bInAutoGenerateMipMaps;
	NewRenderTarget2D->InitAutoFormat(InSize.X, InSize.Y);
	NewRenderTarget2D->UpdateResourceImmediate(true);


	// Flush RHI thread after creating texture render target to make sure that RHIUpdateTextureReference is executed before doing any rendering with it
	// This makes sure that Value->TextureReference.TextureReferenceRHI->GetReferencedTexture() is valid so that FUniformExpressionSet::FillUniformBuffer properly uses the texture for rendering, instead of using a fallback texture
	ENQUEUE_RENDER_COMMAND(FlushRHIThreadToUpdateTextureRenderTargetReference)
	(
		[](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		});

	return NewRenderTarget2D;
}

FGuid FTerrainUtils::StringToGuid(const FString& InStr)
{
	// Compute a 128-bit hash based on the string and use that as a GUID :
	FTCHARToUTF8 Converted(*InStr);
	FMD5 MD5Gen;
	MD5Gen.Update((const uint8*)Converted.Get(), Converted.Length());
	uint32 Digest[4];
	MD5Gen.Final((uint8*)Digest);

	// FGuid::NewGuid() creates a version 4 UUID (at least on Windows), which will have the top 4 bits of the
	// second field set to 0100. We'll set the top bit to 1 in the GUID we create, to ensure that we can never
	// have a collision with other implicitly-generated GUIDs.
	Digest[1] |= 0x80000000;
	return FGuid(Digest[0], Digest[1], Digest[2], Digest[3]);
}

FVector2D FTerrainUtils::RoundUpPowerOfTwo(FVector2D Vector)
{
	return FVector2D {
		FMath::Pow(2.f, UKismetMathLibrary::FCeil(UKismetMathLibrary::Log(Vector.X, 2.f))),
		FMath::Pow(2.f, UKismetMathLibrary::FCeil(UKismetMathLibrary::Log(Vector.Y, 2.f)))
	};
}

TArray<float> FHeightmapUtils::UnpackHeightToFloat(TArray<uint16> InHeightMap)
{
	TArray<float> OutData;
	OutData.SetNum(InHeightMap.Num());

	ParallelFor(
		InHeightMap.Num(),
		[&](int32 Idx)
		{
			const uint8 G = InHeightMap[Idx] & 0xFF;
			const uint8 R = InHeightMap[Idx] >> 8;
			OutData[Idx] = ((R * 255 * 256) + (G * 255));
		},
		!FApp::ShouldUseThreadingForPerformance());

	return OutData;
}

TArray<float> FHeightmapUtils::UnpackHeightToFloat(UTextureRenderTarget2D* InRenderTarget)
{
	if (InRenderTarget)
	{
		ETextureRenderTargetFormat Format = (InRenderTarget->RenderTargetFormat);
		if (FTextureRenderTargetResource* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource())
		{
			FLinearColor LRect = FLinearColor(0, 0, RenderTargetResource->GetSizeX(), RenderTargetResource->GetSizeY());
			if ((Format == (RTF_RGBA16f)) || (Format == (RTF_RGBA32f)) || (Format == (RTF_RGBA8)))
			{
				LRect.R = FMath::Clamp(int(LRect.R), 0, InRenderTarget->SizeX - 1);
				LRect.G = FMath::Clamp(int(LRect.G), 0, InRenderTarget->SizeY - 1);
				LRect.B = FMath::Clamp(int(LRect.B), int(LRect.R + 1), InRenderTarget->SizeX);
				LRect.A = FMath::Clamp(int(LRect.A), int(LRect.G + 1), InRenderTarget->SizeY);

				FIntRect Rect = FIntRect(LRect.R, LRect.G, LRect.B, LRect.A);
				FReadSurfaceDataFlags ReadPixelFlags(RCM_MinMax);
				TArray<FColor> OutLDR;
				RenderTargetResource->ReadPixels(OutLDR, ReadPixelFlags, Rect);

				TArray<float> OutData;
				OutData.SetNum(InRenderTarget->SizeX * InRenderTarget->SizeY);

				for (int32 i = 0; i < OutData.Num(); i++)
				{
					OutData[i] = ((OutLDR[i].R * 255 * 256) + (OutLDR[i].G * 255));
				}

				return OutData;
			}
		}
	}

	return { 0.f };
}

TArray<uint16> FHeightmapUtils::PackHeightTo_uint16(TArray<float> InHeightMap)
{
	TArray<uint16> EncodedHeight;
	EncodedHeight.SetNum(InHeightMap.Num());

	ParallelFor(
		InHeightMap.Num(),
		[&](int32 Idx)
		{
			EncodedHeight[Idx] =
				(uint16)(FMath::Clamp(InHeightMap[Idx] / 255.f, (float)(MIN_uint16), (float)MAX_uint16));
		},
		!FApp::ShouldUseThreadingForPerformance());

	return EncodedHeight;
}


TArray<FColor> FHeightmapUtils::SampleHeightData(UTextureRenderTarget2D* InRenderTarget)
{
	ETextureRenderTargetFormat Format = (InRenderTarget->RenderTargetFormat);
	if (FTextureRenderTargetResource* RenderTargetResource = InRenderTarget->GameThread_GetRenderTargetResource())
	{
		FLinearColor LRect = FLinearColor(0, 0, RenderTargetResource->GetSizeX(), RenderTargetResource->GetSizeY());
		if ((Format == (RTF_RGBA16f)) || (Format == (RTF_RGBA32f)) || (Format == (RTF_RGBA8)))
		{

			FTextureRenderTargetResource* RTResource = InRenderTarget->GameThread_GetRenderTargetResource();

			LRect.R = FMath::Clamp(int(LRect.R), 0, InRenderTarget->SizeX - 1);
			LRect.G = FMath::Clamp(int(LRect.G), 0, InRenderTarget->SizeY - 1);
			LRect.B = FMath::Clamp(int(LRect.B), int(LRect.R + 1), InRenderTarget->SizeX);
			LRect.A = FMath::Clamp(int(LRect.A), int(LRect.G + 1), InRenderTarget->SizeY);
			FIntRect Rect = FIntRect(LRect.R, LRect.G, LRect.B, LRect.A);
			FReadSurfaceDataFlags ReadPixelFlags(RCM_MinMax);
			TArray<FColor> OutLDR;
			RTResource->ReadPixels(OutLDR, ReadPixelFlags, Rect);

			return OutLDR;
		}
	}

	return { FColor(0, 0, 0, 0) };
}

void FPolygonTriangulationUtils::TriangulatePolygon(const TArray<FVector>& Vertices, TArray<int32>& Triangles)
{
	const FVector Center = UKismetMathLibrary::GetVectorArrayAverage(Vertices);
	FVector Normal = FVector::ZeroVector;
	for (int32 ArrayIndex = 0; ArrayIndex < Vertices.Num(); ArrayIndex++)
	{
		const FVector V1 = Vertices[ArrayIndex] - Center;
		const FVector V2 = Vertices[LoopArrayIndex(ArrayIndex, 1, Vertices.Num())] - Center;
		Normal = UKismetMathLibrary::Cross_VectorVector(V1, V2) + Normal;
	}

	UKismetMathLibrary::Vector_Normalize(Normal);
	const FRotator Rot = UKismetMathLibrary::MakeRotFromZ(Normal);

	TArray<FVertIndex> Vert;
	Vert.SetNumUninitialized(Vertices.Num());
	for (int32 ArrayIndex = 0; ArrayIndex < Vertices.Num(); ArrayIndex++)
	{
		const FVector NewVert = UKismetMathLibrary::LessLess_VectorRotator(Vertices[ArrayIndex] - Center, Rot);
		Vert[ArrayIndex] = FVertIndex { ArrayIndex, FVector2D { NewVert.X, NewVert.Y } };
	}
	Vert = FlipPolygon(Vert);
	TArray<FLineSegment> BuildSeg;
	Triangles = RecursiveTriangulate(Vert, Vert, BuildSeg, 0);
}

int32 FPolygonTriangulationUtils::LoopArrayIndex(const int32 Index, const int32 Shift, const int32 ArrayLength)
{
	return (Index + Shift) % ArrayLength;
}

TArray<FPolygonTriangulationUtils::FVertIndex> FPolygonTriangulationUtils::FlipPolygon(const TArray<FVertIndex>& InVert)
{
	TArray<FVertIndex> FlipVert;
	FlipVert.SetNumUninitialized(InVert.Num());
	float Deg = 0.f;

	for (int32 ArrayIndex = 0; ArrayIndex < InVert.Num(); ArrayIndex++)
	{
		const FVertIndex Item1 = InVert[LoopArrayIndex(ArrayIndex, 1, InVert.Num())];
		const FVertIndex Item2 = InVert[LoopArrayIndex(ArrayIndex, 2, InVert.Num())];
		const FVector2D V1 = Item1.Vertex2D - InVert[ArrayIndex].Vertex2D;
		const FVector2D V2 = Item2.Vertex2D - Item1.Vertex2D;

		if (UKismetMathLibrary::CrossProduct2D(V1, V2) >= 0)
		{
			Deg += 360.f
				- UKismetMathLibrary::DegAcos(UKismetMathLibrary::DotProduct2D(V1.GetSafeNormal(), V2.GetSafeNormal()));
		}
		else
		{
			Deg +=
				UKismetMathLibrary::DegAcos(UKismetMathLibrary::DotProduct2D(V1.GetSafeNormal(), V2.GetSafeNormal()));
		}
		FlipVert[ArrayIndex] =
			FVertIndex { InVert[ArrayIndex].Index, InVert[ArrayIndex].Vertex2D * FVector2D { 1.f, -1.f } };
	}

	if (FMath::TruncToInt(Deg) % 180 <= 1 || FMath::TruncToInt(Deg) % 180 >= 179)
	{
		return InVert;
	}
	else
	{
		return FlipVert;
	}
}

TArray<int32> FPolygonTriangulationUtils::RecursiveTriangulate(
	const TArray<FVertIndex>& Vert,
	const TArray<FVertIndex>& FullSetVert,
	const TArray<FLineSegment>& BuiltSeg,
	const int32 Thread)
{
	if (Thread < MAX_uint16 && Vert.Num() > 0)
	{
		TArray<FVertIndex> V = Vert;
		TArray<FLineSegment> Seg = BuiltSeg;

		for (int32 ArrayIndex = 0; ArrayIndex < V.Num(); ArrayIndex++)
		{
			const int32 P1 = LoopArrayIndex(ArrayIndex, 0, V.Num());
			const int32 P2 = LoopArrayIndex(ArrayIndex, 1, V.Num());
			const int32 P3 = LoopArrayIndex(ArrayIndex, 2, V.Num());
			TArray<int32> TmpIdx { V[P1].Index, V[P2].Index, V[P3].Index };

			if (UKismetMathLibrary::CrossProduct2D(V[P2].Vertex2D - V[P1].Vertex2D, V[P3].Vertex2D - V[P2].Vertex2D)
				>= 0)
			{
				bool Skip = true;

				for (int32 IndexFSV = 0; IndexFSV < FullSetVert.Num(); IndexFSV++)
				{
					if (TmpIdx.Find(FullSetVert[IndexFSV].Index) == -1)
					{
						const FVector2D A1 = V[P1].Vertex2D;
						const FVector2D B1 = V[P3].Vertex2D;
						const FVector2D A2 = FullSetVert[IndexFSV].Vertex2D;
						const FVector2D B2 = FullSetVert[LoopArrayIndex(IndexFSV, 1, FullSetVert.Num())].Vertex2D;
						if (LineSegIntersectionTest(A1, B1, A2, B2, true))
						{
							Skip = false;
							break;
						}
						if (PointInTriangle(A2, A1, V[P2].Vertex2D, B1, true))
						{
							Skip = false;
							break;
						}
					}
				}

				if (Skip)
				{
					for (const FLineSegment SegIter : Seg)
					{
						if (LineSegIntersectionTest(V[P1].Vertex2D, V[P3].Vertex2D, SegIter.A, SegIter.B, true))
						{
							Skip = false;
							break;
						}
					}
					if (Skip)
					{
						Seg.Add(FLineSegment { V[P1].Vertex2D, V[P3].Vertex2D });
						V.RemoveAt(P2);

						TArray<int32> Tri = TmpIdx;
						if (Tri.Num() == 3)
						{
							const TArray<int32> NewTris = RecursiveTriangulate(V, FullSetVert, Seg, Thread + 1);
							if (!((Tri[0] == 0) && (Tri[1] == 0) && (Tri[2] == 0)))
								Tri.Append(NewTris);
							else
								Tri = NewTris;
						}
						return Tri;
					}
				}
			}
		}
		return TArray<int32> {};
	}

	return TArray<int32> {};
}

TArray<int32> FPolygonTriangulationUtils::RecursiveTriangulateRev(TArray<int32> Tri, const TArray<int32>& NewTris)
{
	if (Tri.Num() == 3)
	{
		if (!((Tri[0] == 0) && (Tri[1] == 0) && (Tri[2] == 0)))
		{
			Tri.Append(NewTris);
		}
		else
		{
			Tri = NewTris;
		}
	}
	return Tri;
}

bool FPolygonTriangulationUtils::LineSegIntersectionTest(
	const FVector2D& A1,
	const FVector2D& B1,
	const FVector2D& A2,
	const FVector2D& B2,
	const bool bIgnoreSide)
{
	if (bIgnoreSide)
	{
		constexpr float Threshold = -0.000001f;
		if (FVector2D::CrossProduct(A2 - A1, B1 - A1) * FVector2D::CrossProduct(B2 - A1, B1 - A1) > Threshold)
			return false;
		if (FVector2D::CrossProduct(A1 - A2, B2 - A2) * FVector2D::CrossProduct(B1 - A2, B2 - A2) > Threshold)
			return false;
	}
	else
	{
		if (FVector2D::CrossProduct(A1 - A2, B1 - A1) * FVector2D::CrossProduct(B2 - A1, B1 - A1) >= 0)
			return false;
		if (FVector2D::CrossProduct(A1 - A2, B2 - A2) * FVector2D::CrossProduct(B1 - A2, B2 - A2) >= 0)
			return false;
	}
	return true;
}

bool FPolygonTriangulationUtils::PointInTriangle(
	const FVector2D& P,
	const FVector2D& A,
	const FVector2D& B,
	const FVector2D& C,
	bool bIgnoreSide)
{
	constexpr float DefaultThreshold = -0.000001f;
	const FVector2D PA = P - A;
	const FVector2D BA = B - A;
	const FVector2D CA = C - A;
	const FVector2D PB = P - B;
	const FVector2D AB = A - B;
	const FVector2D CB = C - B;
	const FVector2D PC = P - C;
	const FVector2D AC = A - C;
	const FVector2D BC = B - C;

	const float Threshold = bIgnoreSide ? DefaultThreshold : 0.f;
	auto IsNearOrEqual = [Threshold, bIgnoreSide](const float& InValue)
	{
		return bIgnoreSide ? InValue < Threshold : InValue <= Threshold;
	};

	if (IsNearOrEqual(FVector2D::CrossProduct(PA, BA) * FVector2D::CrossProduct(PA, CA))
		&& IsNearOrEqual(FVector2D::CrossProduct(PB, AB) * FVector2D::CrossProduct(PB, CB))
		&& IsNearOrEqual(FVector2D::CrossProduct(PC, AC) * FVector2D::CrossProduct(PC, BC)))
	{
		return true;
	}
	return false;
}
