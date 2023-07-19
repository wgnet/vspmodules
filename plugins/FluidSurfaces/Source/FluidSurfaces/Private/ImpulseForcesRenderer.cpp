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

#include "ImpulseForcesRenderer.h"

#include "ClearQuad.h"
#include "CommonRenderResources.h"
#include "Engine/TextureRenderTarget2D.h"

#define MAX_INSTANCE_GROUP_COUNT 128

class FForceQuadVertexBuffer : public FVertexBuffer
{
public:
	void InitRHI()
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(4);

		Vertices[0].Position = FVector4(-1, 1, 0, 1);
		Vertices[0].UV = FVector2D(0, 0);

		Vertices[1].Position = FVector4(1, 1, 0, 1);
		Vertices[1].UV = FVector2D(1, 0);

		Vertices[2].Position = FVector4(-1, -1, 0, 1);
		Vertices[2].UV = FVector2D(0, 1);

		Vertices[3].Position = FVector4(1, -1, 0, 1);
		Vertices[3].UV = FVector2D(1, 1);

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI = RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static, CreateInfo);
	}
};

TGlobalResource<FForceQuadVertexBuffer> GSimpleScreenVertexBuffer;

class FForcePassThroughVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FForcePassThroughVS);
	SHADER_USE_PARAMETER_STRUCT(FForcePassThroughVS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return !IsMobilePlatform(Parameters.Platform);
	}

	static void ModifyCompilationEnvironment(
		const FGlobalShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FGlobalShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetDefine(TEXT("MAX_INSTANCE_GROUP_COUNT"), MAX_INSTANCE_GROUP_COUNT);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_ARRAY(FVector4, Impulses, [MAX_INSTANCE_GROUP_COUNT])
	END_SHADER_PARAMETER_STRUCT()
};

class FForcePassThroughPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FForcePassThroughPS);
	SHADER_USE_PARAMETER_STRUCT(FForcePassThroughPS, FGlobalShader);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	SHADER_PARAMETER_TEXTURE(Texture2D, Splash)
	SHADER_PARAMETER_SAMPLER(SamplerState, SplashSampler)
	END_SHADER_PARAMETER_STRUCT()
};

IMPLEMENT_GLOBAL_SHADER(FForcePassThroughVS, "/Plugin/FluidSurfaces/ImpulseForcesVS.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FForcePassThroughPS, "/Plugin/FluidSurfaces/ImpulseForcesPS.usf", "MainPS", SF_Pixel);

void FImpulseForcesRenderer::DrawImpulseForces_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FTextureRenderTargetResource* Resource,
	const UTexture2D* SplashTexture,
	const TArray<FVector4>& Impulses)
{
	QUICK_SCOPE_CYCLE_COUNTER(STAT_FluidSurface_DrawImpulseForces);
	SCOPED_DRAW_EVENT(RHICmdList, FluidSurface_DrawImpulseForces);

	if (!SplashTexture || !SplashTexture->GetResource())
	{
		return;
	}

	if (Impulses.Num() <= 0)
	{
		FRHIRenderPassInfo RPInfo(Resource->GetRenderTargetTexture(), ERenderTargetActions::DontLoad_Store);
		TransitionRenderPassTargets(RHICmdList, RPInfo);
		RHICmdList.BeginRenderPass(RPInfo, TEXT("ClearRT"));
		DrawClearQuad(RHICmdList, FLinearColor::Black);
		RHICmdList.EndRenderPass();

		RHICmdList.Transition(
			FRHITransitionInfo(Resource->GetRenderTargetTexture(), ERHIAccess::RTV, ERHIAccess::SRVMask));
		return;
	}

	FRHIRenderPassInfo RenderPassInfo(Resource->GetRenderTargetTexture(), ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("FluidSurfaces_OutputToRenderTarget"));

	auto ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	TShaderMapRef<FForcePassThroughVS> VertexShader(ShaderMap);
	TShaderMapRef<FForcePassThroughPS> PixelShader(ShaderMap);

	// Set the graphic pipeline state.
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Always>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	// Setup the vertex shader
	FForcePassThroughVS::FParameters PassParametersVS;
	for (int32 Index = 0; Index < FMath::Min(Impulses.Num(), MAX_INSTANCE_GROUP_COUNT); Index++)
	{
		PassParametersVS.Impulses[Index] = Impulses[Index];
	}
	SetShaderParameters(RHICmdList, VertexShader, VertexShader.GetVertexShader(), PassParametersVS);

	// Setup the pixel shader
	FForcePassThroughPS::FParameters PassParametersPS;
	const FSamplerStateInitializerRHI BufferSamplerInitializer(SF_Trilinear, AM_Clamp, AM_Clamp, AM_Clamp);
	const FSamplerStateRHIRef SamplerState = RHICreateSamplerState(BufferSamplerInitializer);
	const FTextureResource* SplashTextureResource = SplashTexture->GetResource();

	PassParametersPS.Splash = SplashTextureResource->GetTexture2DRHI();
	PassParametersPS.SplashSampler = SamplerState;
	SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PassParametersPS);

	// Draw
	RHICmdList.SetStreamSource(0, GSimpleScreenVertexBuffer.VertexBufferRHI, 0);
	RHICmdList.DrawPrimitive(0, 2, Impulses.Num());

	RHICmdList.EndRenderPass();
}

#undef MAX_INSTANCE_GROUP_COUNT
