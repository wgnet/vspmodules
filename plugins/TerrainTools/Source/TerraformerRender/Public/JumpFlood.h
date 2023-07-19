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

struct TERRAFORMERRENDER_API FJumpFloodData
{
	FTextureRenderTargetResource* ShapeResource;
	FTextureRenderTargetResource* JumpFloodA;
	FTextureRenderTargetResource* JumpFloodB;
	bool bSmoothEdgesPass;
	int32 SmoothEdgesPasses;
	int32 RequiredPasses;
	float BrushPosition;
	bool bClipShape;
	FIntRect RenderRegion;
};

class TERRAFORMERRENDER_API FJumpFloodPassProcessor
{
public:
	static void JumpStep_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FTextureRenderTargetResource* Source,
		const FTextureRenderTargetResource* Target,
		const bool bClipShape,
		const int32 Pass,
		FIntRect RenderRegion);

	static void DetectEdges_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FTextureRenderTargetResource* Source,
		const FTextureRenderTargetResource* Target,
		float BrushPos,
		FIntRect RenderRegion);

	static void SmoothEdges_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FTextureRenderTargetResource* Source,
		const FTextureRenderTargetResource* Target,
		FIntRect RenderRegion);
};

struct TERRAFORMERRENDER_API FCacheDistanceFieldData
{
	FTextureRenderTargetResource* JumpFlood;
	FTextureRenderTargetResource* DepthShape;
	FTextureRenderTargetResource* DistanceFieldCache;
	uint32 BlurOffset;
	float BlurScale;
	FVector2D TerrainRes;
	FVector2D WorldSize;
	FVector2D UVOffset;
	float TerrainZLocation;
	float TerrainZScale;
	float BrushPosition;
	float ZOffset;
	uint32 FalloffWidth;
	bool bInvert;
	bool bUseBlur;
	FIntRect RenderRegion;
};

class TERRAFORMERRENDER_API FCacheDistanceFieldPassProcessor
{
public:
	static void CacheDistanceField_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FCacheDistanceFieldData& InData,
		FTextureRenderTargetResource* JumpFlood,
		FTextureRenderTargetResource* DepthShape,
		FTextureRenderTargetResource* Target,
		FIntRect RenderRegion);
};

struct TERRAFORMERRENDER_API FBlendAngleFalloffData
{
	FTextureRenderTargetResource* TerrainHeightCache;
	FTextureRenderTargetResource* DistanceFieldCache;
	FTextureRenderTargetResource* OutputResource;
	TOptional<uint32> CapShape;
	uint32 BlendMode;
	FVector2D UVOffset;
	FVector2D WorldSize;
	FVector2D TerrainRes;
	float TerrainZScale;
	float FalloffTangent;
	uint32 FalloffWidth;
	float EdgeCenterOffset;
	float InnerSmoothThreshold;
	float OuterSmoothThreshold;
	float TerracingAlpha;
	float TerracingSmoothness;
	float TerracingSpacing;
	float TerracingMask;
	float TerracingOffset;
};

class TERRAFORMERRENDER_API FBlendAngleFalloffPassProcessor
{
public:
	static void BlendAngleFalloff_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FBlendAngleFalloffData& InData);
};
