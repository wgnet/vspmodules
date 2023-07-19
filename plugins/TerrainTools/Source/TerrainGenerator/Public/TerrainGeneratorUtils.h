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

class UMaterialInstanceDynamic;
class UMaterialInterface;
class UTextureRenderTarget2D;


struct TERRAINGENERATOR_API FTerrainUtils
{
	static UMaterialInstanceDynamic* GetOrCreateTransientMID(
		UMaterialInstanceDynamic* InMID,
		FName InMIDName,
		UMaterialInterface* InMaterialInterface,
		EObjectFlags InAdditionalObjectFlags = RF_NoFlags);

	static UTextureRenderTarget2D* GetOrCreateTransientRenderTarget2D(
		UTextureRenderTarget2D* InRenderTarget,
		FName InRenderTargetName,
		const FIntPoint& InSize,
		ETextureRenderTargetFormat InFormat,
		const FLinearColor& InClearColor = FLinearColor::Black,
		bool bInAutoGenerateMipMaps = false);

	static FGuid StringToGuid(const FString& InStr);

	static FVector2D RoundUpPowerOfTwo(FVector2D Vector);
};

struct TERRAINGENERATOR_API FHeightmapUtils
{
	static TArray<float> UnpackHeightToFloat(TArray<uint16> InHeightMap);
	static TArray<float> UnpackHeightToFloat(UTextureRenderTarget2D* InRenderTarget);
	static TArray<uint16> PackHeightTo_uint16(TArray<float> InHeightMap);
	static TArray<FColor> SampleHeightData(UTextureRenderTarget2D* InRenderTarget);
};

struct TERRAINGENERATOR_API FPolygonTriangulationUtils
{
	static void TriangulatePolygon(const TArray<FVector>& Vertices, TArray<int32>& Triangles);

private:
	struct FVertIndex
	{
		int32 Index;
		FVector2D Vertex2D;
	};

	struct FLineSegment
	{
		FVector2D A;
		FVector2D B;
	};

	static int32 LoopArrayIndex(int32 Index, int32 Shift, int32 ArrayLength);
	static TArray<FVertIndex> FlipPolygon(const TArray<FVertIndex>& InVert);
	static TArray<int32> RecursiveTriangulate(
		const TArray<FVertIndex>& Vert,
		const TArray<FVertIndex>& FullSetVert,
		const TArray<FLineSegment>& BuiltSeg,
		int32 Thread = 0);
	static TArray<int32> RecursiveTriangulateRev(TArray<int32> Tri, const TArray<int32>& NewTris);
	static bool LineSegIntersectionTest(
		const FVector2D& A1,
		const FVector2D& B1,
		const FVector2D& A2,
		const FVector2D& B2,
		bool bIgnoreSide = true);
	static bool PointInTriangle(
		const FVector2D& P,
		const FVector2D& A,
		const FVector2D& B,
		const FVector2D& C,
		bool bIgnoreSide = true);
};
