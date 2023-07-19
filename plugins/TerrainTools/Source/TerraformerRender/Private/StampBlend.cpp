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
#include "StampBlend.h"

#include "Engine/TextureRenderTarget2D.h"
#include "RenderGraphUtils.h"
#include "ShaderParameterStruct.h"
#include "ShapeRenderer.h"

#define LOCTEXT_NAMESPACE "FTerraformerRenderModule"
#define NUM_THREADS_PER_GROUP_DIMENSION 32

class FBlendBrushStampCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FBlendBrushStampCS);
	SHADER_USE_PARAMETER_STRUCT(FBlendBrushStampCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D<float4>, BufferRW)
	SHADER_PARAMETER_TEXTURE(Texture2D, BrushStampTexture)
	SHADER_PARAMETER_TEXTURE(Texture2D, TerrainHeightCache)
	SHADER_PARAMETER_SAMPLER(SamplerState, BrushStampTextureSampler)
	SHADER_PARAMETER_SAMPLER(SamplerState, TerrainHeightCacheSampler)
	SHADER_PARAMETER(FVector, BrushPosition)
	SHADER_PARAMETER(FVector, BrushExtent)
	SHADER_PARAMETER(FVector, BrushScale)
	SHADER_PARAMETER(float, BrushAngle)
	SHADER_PARAMETER(FVector, TerrainOrigin)
	SHADER_PARAMETER(FVector, TerrainExtent)
	SHADER_PARAMETER(FVector, TerrainScale)
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

IMPLEMENT_GLOBAL_SHADER(FBlendBrushStampCS, "/Plugin/TerrainTools/BlendBrushStampCS.usf", "MainCS", SF_Compute);

void FBrushStampPassProcessor::BlendBrushStamp_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FBrushStampData& InData,
	UTextureRenderTarget2D* OutputRT)
{
	FTextureResource* BrushStampTexture = InData.BrushStampTexture->GetResource();
	FTextureRenderTargetResource* TerrainHeightCache = InData.TerrainHeightCache->GetRenderTargetResource();
	FTextureRenderTargetResource* OutputRenderTarget = OutputRT->GetRenderTargetResource();

	const FIntPoint Size = FIntPoint(OutputRenderTarget->GetSizeX(), OutputRenderTarget->GetSizeY());
	const EPixelFormat PixelFormat = OutputRT->GetFormat();
	TRefCountPtr<IPooledRenderTarget> PooledRenderTarget;
	FTerraformerRenderUtils::CreatePooledRenderTarget(
		RHICmdList,
		PooledRenderTarget,
		PixelFormat,
		Size,
		TEXT("BrushStamp"));

	const FUnorderedAccessViewRHIRef OutputUAV = PooledRenderTarget->GetRenderTargetItem().UAV;
	const FSamplerStateInitializerRHI BufferSamplerInitializer(SF_AnisotropicLinear, AM_Border, AM_Border, AM_Border);
	const FSamplerStateRHIRef SamplerState = RHICreateSamplerState(BufferSamplerInitializer);

	RHICmdList.Transition(FRHITransitionInfo(OutputUAV, ERHIAccess::ERWBarrier, ERHIAccess::UAVCompute));

	FBlendBrushStampCS::FParameters PassParameters;
	PassParameters.BufferRW = OutputUAV;
	PassParameters.BrushStampTexture = BrushStampTexture->GetTexture2DRHI();
	PassParameters.BrushStampTextureSampler = SamplerState;
	PassParameters.TerrainHeightCache = TerrainHeightCache->GetTexture2DRHI();
	PassParameters.TerrainHeightCacheSampler = SamplerState;
	PassParameters.BrushPosition = InData.BrushPosition;
	PassParameters.BrushExtent = InData.BrushExtent;
	PassParameters.BrushScale = InData.BrushScale;
	PassParameters.BrushAngle = InData.BrushAngle;
	PassParameters.TerrainOrigin = InData.TerrainOrigin;
	PassParameters.TerrainExtent = InData.TerrainExtent;
	PassParameters.TerrainScale = InData.TerrainScale;
	PassParameters.BlendMode = InData.BlendMode;

	const TShaderMapRef<FBlendBrushStampCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));
	const FIntVector GroupCounts = FIntVector(
		FMath::DivideAndRoundUp(Size.X, NUM_THREADS_PER_GROUP_DIMENSION),
		FMath::DivideAndRoundUp(Size.Y, NUM_THREADS_PER_GROUP_DIMENSION),
		1);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);

	RHICmdList.CopyToResolveTarget(
		PooledRenderTarget->GetShaderResourceRHI(),
		OutputRenderTarget->TextureRHI,
		FResolveParams());
	// TODO Release after landscape update
	//OutputUAV->Release();
}
#undef NUM_THREADS_PER_GROUP_DIMENSION
#undef LOCTEXT_NAMESPACE
