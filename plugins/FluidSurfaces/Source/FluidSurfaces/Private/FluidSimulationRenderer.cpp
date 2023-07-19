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
#include "FluidSimulationRenderer.h"

#include "Engine/TextureRenderTarget2D.h"
#include "FluidSurfaceSubsystem.h"
#include "RenderGraphUtils.h"
#include "RenderTargetPool.h"

#define NUM_THREADS_PER_GROUP_DIMENSION 32

class FFluidSimulationCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFluidSimulationCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidSimulationCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D<float4>, FlowUAV)
	SHADER_PARAMETER_TEXTURE(Texture2D, BufferA)
	SHADER_PARAMETER_TEXTURE(Texture2D, BufferB)
	SHADER_PARAMETER_TEXTURE(Texture2D, BufferF)
	SHADER_PARAMETER_TEXTURE(Texture2D, BufferI)
	SHADER_PARAMETER_SAMPLER(SamplerState, BufferASampler)
	SHADER_PARAMETER_SAMPLER(SamplerState, BufferBSampler)
	SHADER_PARAMETER_SAMPLER(SamplerState, BufferFSampler)
	SHADER_PARAMETER_SAMPLER(SamplerState, BufferISampler)
	SHADER_PARAMETER(FVector, PreviousOffsets1)
	SHADER_PARAMETER(FVector, PreviousOffsets2)
	SHADER_PARAMETER(float, RippleFoamErasure)
	SHADER_PARAMETER(float, FoamDamping)
	SHADER_PARAMETER(float, FluidDamping)
	SHADER_PARAMETER(float, TravelSpeed)
	SHADER_PARAMETER(float, SimulationWorldSize)
	SHADER_PARAMETER(float, ForceMultiplier)
	SHADER_PARAMETER(float, ImpulseMultiplier)
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

IMPLEMENT_GLOBAL_SHADER(FFluidSimulationCS, "/Plugin/FluidSurfaces/FluidSimulationCS.usf", "MainCS", SF_Compute);

class FFluidNormalsCS : public FGlobalShader
{
	DECLARE_GLOBAL_SHADER(FFluidNormalsCS);
	SHADER_USE_PARAMETER_STRUCT(FFluidNormalsCS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_UAV(RWTexture2D<float4>, NormalUAV)
	SHADER_PARAMETER_TEXTURE(Texture2D, BufferA)
	SHADER_PARAMETER_SAMPLER(SamplerState, BufferASampler)
	SHADER_PARAMETER(float, SimulationWorldSize)
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

IMPLEMENT_GLOBAL_SHADER(FFluidNormalsCS, "/Plugin/FluidSurfaces/FluidNormalsCS.usf", "MainCS", SF_Compute);

void FFluidSimulationRenderer::Simulate_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FFluidSimulationShaderParameters& DrawParameters,
	int64 Frame)
{
	check(IsInRenderingThread());
	if (IsValid(DrawParameters.GetBufferA(Frame)) || IsValid(DrawParameters.GetBufferB(Frame))
		|| IsValid(DrawParameters.GetBufferC(Frame)) || IsValid(DrawParameters.BufferF)
		|| IsValid(DrawParameters.BufferI) || IsValid(DrawParameters.BufferN))
	{
		if (DrawParameters.GetBufferA(Frame)->RenderTargetFormat != RTF_RG16f)
		{
			return;
		}

		if (DrawParameters.BufferN->RenderTargetFormat != RTF_RGBA16f)
		{
			return;
		}

		const bool bNeedUpdate = CacheTextureSize.X != UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize()
			|| CacheTextureSize.X != UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize();

		// Allocate temporary render target BufferA
		if (!CSFlowOutput.IsValid() || bNeedUpdate)
		{
			const FIntPoint BufferSize = FIntPoint(UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize());
			const EPixelFormat Format = GetPixelFormatFromRenderTargetFormat(RTF_RG16f);
			CreatePooledRenderTarget(RHICmdList, CSFlowOutput, Format, BufferSize, TEXT("FluidSurfaces_CSFlowOutput"));
		}

		// Allocate temporary render target BufferN
		if (!CSNormalOutput.IsValid() || bNeedUpdate)
		{
			const FIntPoint BufferSize = FIntPoint(UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize());
			const EPixelFormat Format = GetPixelFormatFromRenderTargetFormat(RTF_RGBA16f);
			CreatePooledRenderTarget(
				RHICmdList,
				CSNormalOutput,
				Format,
				BufferSize,
				TEXT("FluidSurfaces_CSNormalOutput"));
		}
		CacheTextureSize = FIntPoint(DrawParameters.GetBufferA(Frame)->SizeX, DrawParameters.GetBufferA(Frame)->SizeY);

		ComputeFlow_RenderThread(
			RHICmdList,
			DrawParameters,
			CSFlowOutput->GetRenderTargetItem().UAV,
			DrawParameters.GetBufferB(Frame)->GetRenderTargetResource()->GetTexture2DRHI(),
			DrawParameters.GetBufferC(Frame)->GetRenderTargetResource()->GetTexture2DRHI(),
			DrawParameters.BufferF->GetRenderTargetResource()->GetTexture2DRHI(),
			DrawParameters.BufferI->GetRenderTargetResource()->GetTexture2DRHI());

		RHICmdList.CopyToResolveTarget(
			CSFlowOutput->GetShaderResourceRHI(),
			DrawParameters.GetBufferA(Frame)->GetRenderTargetResource()->TextureRHI,
			FResolveParams());

		if (!DrawParameters.bComputeNormals)
			return;

		ComputeNormals_RenderThread(
			RHICmdList,
			DrawParameters,
			CSNormalOutput->GetRenderTargetItem().UAV,
			DrawParameters.GetBufferA(Frame)->GetRenderTargetResource()->GetTexture2DRHI());

		RHICmdList.CopyToResolveTarget(
			CSNormalOutput->GetShaderResourceRHI(),
			DrawParameters.BufferN->GetRenderTargetResource()->TextureRHI,
			FResolveParams());
	}
}

DECLARE_GPU_STAT_NAMED(FluidSimulation_Render, TEXT("FluidSimulation_Render"));

void FFluidSimulationRenderer::ComputeFlow_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FFluidSimulationShaderParameters& DrawParameters,
	FUnorderedAccessViewRHIRef InFlowUAV,
	FTexture2DRHIRef InBufferA,
	FTexture2DRHIRef InBufferB,
	FTexture2DRHIRef InBufferF,
	FTexture2DRHIRef InBufferI)
{
	SCOPED_NAMED_EVENT(FluidSimulation_Render, FColor::Blue);
	SCOPED_GPU_STAT(RHICmdList, FluidSimulation_Render)

	RHICmdList.Transition(FRHITransitionInfo(InFlowUAV, ERHIAccess::ERWBarrier, ERHIAccess::UAVCompute));

	const FSamplerStateInitializerRHI BufferSamplerInitializer(SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp);
	const FSamplerStateRHIRef SamplerState = RHICreateSamplerState(BufferSamplerInitializer);

	FFluidSimulationCS::FParameters PassParameters;
	PassParameters.FlowUAV = InFlowUAV;
	PassParameters.BufferA = InBufferA;
	PassParameters.BufferB = InBufferB;
	PassParameters.BufferF = InBufferF;
	PassParameters.BufferI = InBufferI;
	PassParameters.BufferASampler = SamplerState;
	PassParameters.BufferBSampler = SamplerState;
	PassParameters.BufferFSampler = SamplerState;
	PassParameters.BufferISampler = SamplerState;
	PassParameters.PreviousOffsets1 = DrawParameters.PreviousOffsets1;
	PassParameters.PreviousOffsets2 = DrawParameters.PreviousOffsets2;
	PassParameters.RippleFoamErasure = DrawParameters.RippleFoamErasure;
	PassParameters.FoamDamping = DrawParameters.FoamDamping;
	PassParameters.FluidDamping = DrawParameters.FluidDamping;
	PassParameters.TravelSpeed = DrawParameters.TravelSpeed;
	PassParameters.SimulationWorldSize = DrawParameters.SimulationWorldSize;
	PassParameters.ForceMultiplier = DrawParameters.ForceMultiplier;
	PassParameters.ImpulseMultiplier = DrawParameters.ImpulseMultiplier;

	const TShaderMapRef<FFluidSimulationCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	const FIntVector GroupCounts = FIntVector(
		FMath::DivideAndRoundUp((int32)InBufferA->GetSizeX(), NUM_THREADS_PER_GROUP_DIMENSION),
		FMath::DivideAndRoundUp((int32)InBufferA->GetSizeY(), NUM_THREADS_PER_GROUP_DIMENSION),
		1);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);
}

void FFluidSimulationRenderer::ComputeNormals_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FFluidSimulationShaderParameters& DrawParameters,
	FUnorderedAccessViewRHIRef InNormalUAV,
	FTexture2DRHIRef InBufferA)
{
	RHICmdList.Transition(FRHITransitionInfo(InNormalUAV, ERHIAccess::ERWBarrier, ERHIAccess::UAVCompute));
	const FSamplerStateInitializerRHI BufferSamplerInitializer(SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp);
	const FSamplerStateRHIRef SamplerState = RHICreateSamplerState(BufferSamplerInitializer);

	FFluidNormalsCS::FParameters PassParameters;
	PassParameters.NormalUAV = InNormalUAV;
	PassParameters.BufferA = InBufferA;
	PassParameters.BufferASampler = SamplerState;
	PassParameters.SimulationWorldSize = DrawParameters.SimulationWorldSize;

	const TShaderMapRef<FFluidNormalsCS> ComputeShader(GetGlobalShaderMap(GMaxRHIFeatureLevel));

	const FIntVector GroupCounts = FIntVector(
		FMath::DivideAndRoundUp((int32)InBufferA->GetSizeX(), NUM_THREADS_PER_GROUP_DIMENSION),
		FMath::DivideAndRoundUp((int32)InBufferA->GetSizeY(), NUM_THREADS_PER_GROUP_DIMENSION),
		1);

	FComputeShaderUtils::Dispatch(RHICmdList, ComputeShader, PassParameters, GroupCounts);
}

bool FFluidSimulationRenderer::CreatePooledRenderTarget(
	FRHICommandListImmediate& RHICmdList,
	TRefCountPtr<IPooledRenderTarget>& InPooledRenderTarget,
	EPixelFormat InFormat,
	FIntPoint InSize,
	const TCHAR* DebugName)
{
	FPooledRenderTargetDesc PooledRenderTargetDesc(FPooledRenderTargetDesc::Create2DDesc(
		InSize,
		InFormat,
		FClearValueBinding::None,
		TexCreate_None,
		TexCreate_ShaderResource | TexCreate_UAV,
		false));
	PooledRenderTargetDesc.DebugName = DebugName;
	return GRenderTargetPool.FindFreeElement(
		RHICmdList,
		PooledRenderTargetDesc,
		InPooledRenderTarget,
		PooledRenderTargetDesc.DebugName);
}

#undef NUM_THREADS_PER_GROUP_DIMENSION
