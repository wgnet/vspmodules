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

#include "JumpFlood.h"
#include "ShapeRenderer.h"

#include "Engine/TextureRenderTarget2D.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"
#include "ShaderParameterStruct.h"

#define LOCTEXT_NAMESPACE "FTerraformerRenderModule"
#define NUM_THREADS_PER_GROUP_DIMENSION 32

class FJumpFloodCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FJumpFloodCS);
	SHADER_USE_PARAMETER_STRUCT(FJumpFloodCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D<float4>, BufferRW)
	SHADER_PARAMETER_TEXTURE(Texture2D, SeedEdges)
	SHADER_PARAMETER(int32, Index)
	SHADER_PARAMETER(int32, ClipShape)
	SHADER_PARAMETER(FIntRect, RenderRegion)
	RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FJumpFloodCS, "/Plugin/TerrainTools/JumpFloodCS.usf", "MainCS", SF_Compute);

class FDetectEdgesCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FDetectEdgesCS);
	SHADER_USE_PARAMETER_STRUCT(FDetectEdgesCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D<float4>, BufferRW)
	SHADER_PARAMETER_TEXTURE(Texture2D, SeedEdges)
	SHADER_PARAMETER(float, BrushPosition)
	SHADER_PARAMETER(FIntRect, RenderRegion)
	RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FDetectEdgesCS, "/Plugin/TerrainTools/DetectEdgesCS.usf", "MainCS", SF_Compute);

class FBlurEdgesCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FBlurEdgesCS);
	SHADER_USE_PARAMETER_STRUCT(FBlurEdgesCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D<float4>, BufferRW)
	SHADER_PARAMETER_TEXTURE(Texture2D, Tex)
	SHADER_PARAMETER(FIntRect, RenderRegion)
	RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FBlurEdgesCS, "/Plugin/TerrainTools/BlurEdgesCS.usf", "MainCS", SF_Compute);

class FCacheDistanceFieldCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FCacheDistanceFieldCS);
	SHADER_USE_PARAMETER_STRUCT(FCacheDistanceFieldCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D<float4>, BufferRW)
	SHADER_PARAMETER_TEXTURE(Texture2D, JumpFloodTex)
	SHADER_PARAMETER_TEXTURE(Texture2D, DepthShapeTex)
	SHADER_PARAMETER_SAMPLER(SamplerState, JumpFloodSampler)
	SHADER_PARAMETER_SAMPLER(SamplerState, DepthShapeSampler)
	SHADER_PARAMETER(uint32, BlurOffset)
	SHADER_PARAMETER(float, BlurScale)
	SHADER_PARAMETER(FVector2D, TerrainRes)
	SHADER_PARAMETER(FVector2D, WorldSize)
	SHADER_PARAMETER(FVector2D, UVOffset)
	SHADER_PARAMETER(float, TerrainZLocation)
	SHADER_PARAMETER(float, TerrainZScale)
	SHADER_PARAMETER(float, BrushPosition)
	SHADER_PARAMETER(float, ZOffset)
	SHADER_PARAMETER(uint32, FalloffWidth)
	SHADER_PARAMETER(uint32, bInvert)
	SHADER_PARAMETER(uint32, bUseBlur)
	SHADER_PARAMETER(FIntRect, RenderRegion)
	RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FCacheDistanceFieldCS, "/Plugin/TerrainTools/CacheDistanceFieldCS.usf", "MainCS", SF_Compute);

class FBlendAngleFalloffCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FBlendAngleFalloffCS);
	SHADER_USE_PARAMETER_STRUCT(FBlendAngleFalloffCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D<float4>, BufferRW)
	SHADER_PARAMETER_TEXTURE(Texture2D, DistanceFieldCache)
	SHADER_PARAMETER_TEXTURE(Texture2D, TerrainHeightCache)
	SHADER_PARAMETER_SAMPLER(SamplerState, DistanceFieldCacheSampler)
	SHADER_PARAMETER_SAMPLER(SamplerState, TerrainHeightCacheSampler)
	SHADER_PARAMETER(FVector2D, UVOffset)
	SHADER_PARAMETER(FVector2D, WorldSize)
	SHADER_PARAMETER(FVector2D, TerrainRes)
	SHADER_PARAMETER(float, TerrainZScale)
	// Angle Controls
	SHADER_PARAMETER(float, FalloffTangent)
	SHADER_PARAMETER(int32, FalloffWidth)
	SHADER_PARAMETER(float, EdgeCenterOffset)
	// Smoothing
	SHADER_PARAMETER(float, InnerSmoothThreshold)
	SHADER_PARAMETER(float, OuterSmoothThreshold)
	// Terracing
	SHADER_PARAMETER(float, TerracingAlpha)
	SHADER_PARAMETER(float, TerracingSmoothness)
	SHADER_PARAMETER(float, TerracingSpacing)
	SHADER_PARAMETER(float, TerracingMask)
	SHADER_PARAMETER(float, TerracingOffset)
	// Optional
	SHADER_PARAMETER(uint32, CapShapeInterior)
	SHADER_PARAMETER(uint32, BlendMode)
	RENDER_TARGET_BINDING_SLOTS()
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	static bool ShouldCache(EShaderPlatform Platform)
	{
		return IsFeatureLevelSupported(Platform, ERHIFeatureLevel::SM5);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_X"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Y"), NUM_THREADS_PER_GROUP_DIMENSION);
		OutEnvironment.SetDefine(TEXT("THREADGROUPSIZE_Z"), 1);
	}
};

IMPLEMENT_GLOBAL_SHADER(FBlendAngleFalloffCS, "/Plugin/TerrainTools/BlendAngleFalloffCS.usf", "MainCS", SF_Compute);

void FJumpFloodPassProcessor::JumpStep_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FTextureRenderTargetResource* Source,
	const FTextureRenderTargetResource* Target,
	const bool bClipShape,
	const int32 Pass,
	FIntRect RenderRegion)
{
	const FIntPoint Size = FIntPoint(Source->GetSizeX(), Source->GetSizeY());
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	const EPixelFormat Format = GetPixelFormatFromRenderTargetFormat(RTF_RGBA32f);
	FTerraformerRenderUtils::CreatePooledRenderTarget(
		RHICmdList,
		PooledRenderTarget,
		Format,
		Size,
		TEXT("JumpFloodStep"));

	const FUnorderedAccessViewRHIRef OutputUAV = PooledRenderTarget->GetRenderTargetItem().UAV;

	RHICmdList.Transition(FRHITransitionInfo(OutputUAV, ERHIAccess::ERWBarrier, ERHIAccess::UAVCompute));

	FJumpFloodCS::FParameters PassParameters;
	PassParameters.BufferRW = OutputUAV;
	PassParameters.SeedEdges = Source->GetTexture2DRHI();
	PassParameters.Index = (Pass + 1);
	PassParameters.ClipShape = bClipShape;
	PassParameters.RenderRegion = RenderRegion;

	const TShaderMapRef<FJumpFloodCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	const int32 DividendX = RenderRegion.Max.X - RenderRegion.Min.X;
	const int32 DividendY = RenderRegion.Max.Y - RenderRegion.Min.Y;
	const FIntVector GroupCounts = FIntVector(
		FMath::DivideAndRoundUp(DividendX, NUM_THREADS_PER_GROUP_DIMENSION),
		FMath::DivideAndRoundUp(DividendY, NUM_THREADS_PER_GROUP_DIMENSION),
		1);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);

	RHICmdList.CopyToResolveTarget(PooledRenderTarget->GetShaderResourceRHI(), Target->TextureRHI, FResolveParams());

	OutputUAV->Release();
}

void FJumpFloodPassProcessor::DetectEdges_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FTextureRenderTargetResource* Source,
	const FTextureRenderTargetResource* Target,
	float BrushPos,
	FIntRect RenderRegion)
{
	const FIntPoint Size = FIntPoint(Source->GetSizeX(), Source->GetSizeY());
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	const EPixelFormat Format = GetPixelFormatFromRenderTargetFormat(RTF_RGBA32f);
	FTerraformerRenderUtils::CreatePooledRenderTarget(
		RHICmdList,
		PooledRenderTarget,
		Format,
		Size,
		TEXT("DetectEdges"));

	const FUnorderedAccessViewRHIRef OutputUAV = PooledRenderTarget->GetRenderTargetItem().UAV;

	RHICmdList.Transition(FRHITransitionInfo(OutputUAV, ERHIAccess::ERWBarrier, ERHIAccess::UAVCompute));

	FDetectEdgesCS::FParameters PassParameters;
	PassParameters.BufferRW = OutputUAV;
	PassParameters.SeedEdges = Source->GetTexture2DRHI();
	PassParameters.BrushPosition = BrushPos;
	PassParameters.RenderRegion = RenderRegion;

	const TShaderMapRef<FDetectEdgesCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	// TODO RenderRegion
	//const int32 DividendX = RenderRegion.Max.X - RenderRegion.Min.X;
	//const int32 DividendY = RenderRegion.Max.Y - RenderRegion.Min.Y;
	const FIntVector GroupCounts = FIntVector(
		FMath::DivideAndRoundUp(Size.X, NUM_THREADS_PER_GROUP_DIMENSION),
		FMath::DivideAndRoundUp(Size.Y, NUM_THREADS_PER_GROUP_DIMENSION),
		1);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);

	RHICmdList.CopyToResolveTarget(PooledRenderTarget->GetShaderResourceRHI(), Target->TextureRHI, FResolveParams());

	OutputUAV->Release();
}

void FJumpFloodPassProcessor::SmoothEdges_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FTextureRenderTargetResource* Source,
	const FTextureRenderTargetResource* Target,
	FIntRect RenderRegion)
{
	const FIntPoint Size = FIntPoint(Source->GetSizeX(), Source->GetSizeY());
	const EPixelFormat Format = GetPixelFormatFromRenderTargetFormat(RTF_RGBA32f);
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	FTerraformerRenderUtils::CreatePooledRenderTarget(RHICmdList, PooledRenderTarget, Format, Size, TEXT("BlurEdges"));

	const FUnorderedAccessViewRHIRef OutputUAV = PooledRenderTarget->GetRenderTargetItem().UAV;

	RHICmdList.Transition(FRHITransitionInfo(OutputUAV, ERHIAccess::ERWBarrier, ERHIAccess::UAVCompute));

	FBlurEdgesCS::FParameters PassParameters;
	PassParameters.BufferRW = OutputUAV;
	PassParameters.Tex = Source->GetTexture2DRHI();
	PassParameters.RenderRegion = RenderRegion;

	const TShaderMapRef<FBlurEdgesCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	// TODO RenderRegion
	const int32 DividendX = RenderRegion.Max.X - RenderRegion.Min.X;
	const int32 DividendY = RenderRegion.Max.Y - RenderRegion.Min.Y;
	const FIntVector GroupCounts = FIntVector(
		FMath::DivideAndRoundUp(DividendX, NUM_THREADS_PER_GROUP_DIMENSION),
		FMath::DivideAndRoundUp(DividendY, NUM_THREADS_PER_GROUP_DIMENSION),
		1);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);

	RHICmdList.CopyToResolveTarget(PooledRenderTarget->GetShaderResourceRHI(), Target->TextureRHI, FResolveParams());

	OutputUAV->Release();
}

void FCacheDistanceFieldPassProcessor::CacheDistanceField_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FCacheDistanceFieldData& InData,
	FTextureRenderTargetResource* JumpFlood,
	FTextureRenderTargetResource* DepthShape,
	FTextureRenderTargetResource* Target,
	FIntRect RenderRegion)
{
	const FIntPoint Size = FIntPoint(JumpFlood->GetSizeX(), JumpFlood->GetSizeY());
	const EPixelFormat Format = GetPixelFormatFromRenderTargetFormat(RTF_RGBA32f);
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	FTerraformerRenderUtils::CreatePooledRenderTarget(
		RHICmdList,
		PooledRenderTarget,
		Format,
		Size,
		TEXT("CacheDistanceField"));

	const FUnorderedAccessViewRHIRef OutputUAV = PooledRenderTarget->GetRenderTargetItem().UAV;
	const FSamplerStateInitializerRHI BufferSamplerInitializer(SF_Point, AM_Clamp, AM_Clamp, AM_Clamp);
	const FSamplerStateRHIRef SamplerState = RHICreateSamplerState(BufferSamplerInitializer);

	RHICmdList.Transition(FRHITransitionInfo(OutputUAV, ERHIAccess::ERWBarrier, ERHIAccess::UAVCompute));

	FCacheDistanceFieldCS::FParameters PassParameters;
	PassParameters.BufferRW = OutputUAV;
	PassParameters.JumpFloodTex = JumpFlood->GetTexture2DRHI();
	PassParameters.DepthShapeTex = DepthShape->GetTexture2DRHI();
	PassParameters.JumpFloodSampler = SamplerState;
	PassParameters.DepthShapeSampler = SamplerState;
	PassParameters.BlurOffset = InData.BlurOffset;
	PassParameters.BlurScale = InData.BlurScale;
	PassParameters.TerrainRes = InData.TerrainRes;
	PassParameters.WorldSize = InData.WorldSize;
	PassParameters.UVOffset = FVector2D::ZeroVector;
	PassParameters.TerrainZLocation = InData.TerrainZLocation;
	PassParameters.TerrainZScale = InData.TerrainZScale;
	PassParameters.BrushPosition = InData.BrushPosition;
	PassParameters.ZOffset = InData.ZOffset;
	PassParameters.FalloffWidth = InData.FalloffWidth;
	PassParameters.bInvert = (int32)InData.bInvert;
	PassParameters.bUseBlur = (int32)InData.bUseBlur;
	PassParameters.RenderRegion = RenderRegion;

	const TShaderMapRef<FCacheDistanceFieldCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	const int32 DividendX = RenderRegion.Max.X - RenderRegion.Min.X;
	const int32 DividendY = RenderRegion.Max.Y - RenderRegion.Min.Y;
	const FIntVector GroupCounts = FIntVector(
		FMath::DivideAndRoundUp(DividendX, NUM_THREADS_PER_GROUP_DIMENSION),
		FMath::DivideAndRoundUp(DividendY, NUM_THREADS_PER_GROUP_DIMENSION),
		1);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);

	RHICmdList.CopyToResolveTarget(PooledRenderTarget->GetShaderResourceRHI(), Target->TextureRHI, FResolveParams());

	OutputUAV->Release();
}

void FBlendAngleFalloffPassProcessor::BlendAngleFalloff_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FBlendAngleFalloffData& InData)
{
	FTextureRenderTargetResource* DistanceFieldCache = InData.DistanceFieldCache;
	FTextureRenderTargetResource* TerrainHeightCache = InData.TerrainHeightCache;

	const FIntPoint Size = FIntPoint(InData.OutputResource->GetSizeX(), InData.OutputResource->GetSizeY());
	const EPixelFormat Format = GetPixelFormatFromRenderTargetFormat(RTF_RGBA8);
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	FTerraformerRenderUtils::CreatePooledRenderTarget(
		RHICmdList,
		PooledRenderTarget,
		Format,
		Size,
		TEXT("BlendAngleFalloff"));

	const FUnorderedAccessViewRHIRef OutputUAV = PooledRenderTarget->GetRenderTargetItem().UAV;
	const FSamplerStateInitializerRHI BufferSamplerInitializer(SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp);
	const FSamplerStateRHIRef SamplerState = RHICreateSamplerState(BufferSamplerInitializer);

	RHICmdList.Transition(FRHITransitionInfo(OutputUAV, ERHIAccess::ERWBarrier, ERHIAccess::UAVCompute));

	FBlendAngleFalloffCS::FParameters PassParameters;
	PassParameters.BufferRW = OutputUAV;
	PassParameters.TerrainHeightCache = TerrainHeightCache->GetTexture2DRHI();
	PassParameters.DistanceFieldCache = DistanceFieldCache->GetTexture2DRHI();
	PassParameters.TerrainHeightCacheSampler = SamplerState;
	PassParameters.DistanceFieldCacheSampler = SamplerState;

	PassParameters.CapShapeInterior = InData.CapShape.GetValue();
	PassParameters.BlendMode = InData.BlendMode;

	PassParameters.UVOffset = InData.UVOffset;
	PassParameters.WorldSize = InData.WorldSize;
	PassParameters.TerrainRes = InData.TerrainRes;
	PassParameters.TerrainZScale = InData.TerrainZScale;

	PassParameters.FalloffTangent = InData.FalloffTangent;
	PassParameters.FalloffWidth = InData.FalloffWidth;
	PassParameters.EdgeCenterOffset = InData.EdgeCenterOffset;

	PassParameters.InnerSmoothThreshold = InData.InnerSmoothThreshold;
	PassParameters.OuterSmoothThreshold = InData.OuterSmoothThreshold;

	PassParameters.TerracingAlpha = InData.TerracingAlpha;
	PassParameters.TerracingSmoothness = InData.TerracingSmoothness;
	PassParameters.TerracingSpacing = InData.TerracingSpacing;
	PassParameters.TerracingMask = InData.TerracingMask;
	PassParameters.TerracingOffset = InData.TerracingOffset;

	const TShaderMapRef<FBlendAngleFalloffCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	const FIntVector GroupCounts = FIntVector(
		FMath::DivideAndRoundUp(Size.X, NUM_THREADS_PER_GROUP_DIMENSION),
		FMath::DivideAndRoundUp(Size.Y, NUM_THREADS_PER_GROUP_DIMENSION),
		1);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);

	RHICmdList.CopyToResolveTarget(
		PooledRenderTarget->GetShaderResourceRHI(),
		InData.OutputResource->TextureRHI,
		FResolveParams());
	// TODO Release After Landscape update
	//OutputUAV->Release();
}

#undef NUM_THREADS_PER_GROUP_DIMENSION
#undef LOCTEXT_NAMESPACE
