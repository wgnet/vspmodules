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


#include "ShapeRenderer.h"

#include "CommonRenderResources.h"
#include "Engine/TextureRenderTarget2D.h"
#include "RenderTargetPool.h"


class FTerraformerVertexBuffer : public FVertexBuffer
{
public:
	void InitVertexBufferRHI(const TArray<FVector> InVertices)
	{
		TResourceArray<FFilterVertex, VERTEXBUFFER_ALIGNMENT> Vertices;
		Vertices.SetNumUninitialized(InVertices.Num());

		for (int32 i = 0; i < InVertices.Num(); i++)
		{
			Vertices[i].Position = FVector4(InVertices[i], 0);
			Vertices[i].UV = FVector2D(InVertices[i].X, InVertices[i].Y);
		}

		FRHIResourceCreateInfo CreateInfo(&Vertices);
		VertexBufferRHI =
			RHICreateVertexBuffer(Vertices.GetResourceDataSize(), BUF_Static | BUF_VertexBuffer, CreateInfo);
	}
};

TGlobalResource<FTerraformerVertexBuffer> GTerraformerVertexBuffer;

class FTerraformerPassThroughVS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTerraformerPassThroughVS);

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return true;
	}

	FTerraformerPassThroughVS()
	{
	}

	FTerraformerPassThroughVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FGlobalShader(Initializer)
	{
	}
};

class FTerraformerPixelShaderPS : public FGlobalShader
{
public:
	DECLARE_GLOBAL_SHADER(FTerraformerPixelShaderPS);
	SHADER_USE_PARAMETER_STRUCT(FTerraformerPixelShaderPS, FGlobalShader);

	BEGIN_SHADER_PARAMETER_STRUCT(FParameters, )
	END_SHADER_PARAMETER_STRUCT()

	static bool ShouldCompilePermutation(const FGlobalShaderPermutationParameters& Parameters)
	{
		return IsFeatureLevelSupported(Parameters.Platform, ERHIFeatureLevel::SM5);
	}
};

IMPLEMENT_GLOBAL_SHADER(FTerraformerPassThroughVS, "/Plugin/TerrainTools/ShapeDepthVS.usf", "MainVS", SF_Vertex);
IMPLEMENT_GLOBAL_SHADER(FTerraformerPixelShaderPS, "/Plugin/TerrainTools/ShapeDepthPS.usf", "MainPS", SF_Pixel);

bool FTerraformerRenderUtils::CreatePooledRenderTarget(
	FRHICommandListImmediate& RHICmdList,
	TRefCountPtr<IPooledRenderTarget>& InPooledRenderTarget,
	const EPixelFormat InFormat,
	const FIntPoint InSize,
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

void FDrawShapePassProcessor::DrawAreaShape_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FTerraformerShapeDrawData& DrawData)
{
	FTextureRenderTargetResource* Resource = DrawData.RenderTarget->GetRenderTargetResource();
	FRHIRenderPassInfo RenderPassInfo(Resource->GetRenderTargetTexture(), ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("Terraformer_DrawAreaShape"));

	const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FTerraformerPassThroughVS> VertexShader(ShaderMap);
	const TShaderMapRef<FTerraformerPixelShaderPS> PixelShader(ShaderMap);

	// Set the graphic pipeline state.
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Never>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	GraphicsPSOInit.PrimitiveType = PT_TriangleList;
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	// Setup the pixel shader
	FTerraformerPixelShaderPS::FParameters PassParameters;
	SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PassParameters);

	// Draw
	GTerraformerVertexBuffer.InitVertexBufferRHI(DrawData.Vertices);
	RHICmdList.SetStreamSource(0, GTerraformerVertexBuffer.VertexBufferRHI, 0);
	RHICmdList.DrawPrimitive(0, DrawData.GetNumPrimitives(), 1);

	RHICmdList.EndRenderPass();
}

void FDrawShapePassProcessor::DrawPathShape_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	const FTerraformerShapeDrawData& DrawData)
{
	FTextureRenderTargetResource* Resource = DrawData.RenderTarget->GetRenderTargetResource();
	FRHIRenderPassInfo RenderPassInfo(Resource->GetRenderTargetTexture(), ERenderTargetActions::Clear_Store);
	RHICmdList.BeginRenderPass(RenderPassInfo, TEXT("Terraformer_DrawPathShape"));

	const FGlobalShaderMap* ShaderMap = GetGlobalShaderMap(GMaxRHIFeatureLevel);
	const TShaderMapRef<FTerraformerPassThroughVS> VertexShader(ShaderMap);
	const TShaderMapRef<FTerraformerPixelShaderPS> PixelShader(ShaderMap);

	// Set the graphic pipeline state.
	FGraphicsPipelineStateInitializer GraphicsPSOInit;
	RHICmdList.ApplyCachedRenderTargets(GraphicsPSOInit);
	GraphicsPSOInit.BlendState = TStaticBlendState<>::GetRHI();
	GraphicsPSOInit.RasterizerState = TStaticRasterizerState<FM_Solid, CM_None>::GetRHI();
	GraphicsPSOInit.DepthStencilState = TStaticDepthStencilState<false, CF_Never>::GetRHI();
	GraphicsPSOInit.BoundShaderState.VertexDeclarationRHI = GFilterVertexDeclaration.VertexDeclarationRHI;
	GraphicsPSOInit.BoundShaderState.VertexShaderRHI = VertexShader.GetVertexShader();
	GraphicsPSOInit.BoundShaderState.PixelShaderRHI = PixelShader.GetPixelShader();
	GraphicsPSOInit.PrimitiveType = PT_TriangleStrip;
	SetGraphicsPipelineState(RHICmdList, GraphicsPSOInit);

	// Setup the pixel shader
	FTerraformerPixelShaderPS::FParameters PassParameters;
	SetShaderParameters(RHICmdList, PixelShader, PixelShader.GetPixelShader(), PassParameters);

	// Draw
	GTerraformerVertexBuffer.InitVertexBufferRHI(DrawData.Vertices);
	RHICmdList.SetStreamSource(0, GTerraformerVertexBuffer.VertexBufferRHI, 0);
	RHICmdList.DrawPrimitive(DrawData.GetBaseVertexIndex(), DrawData.GetNumPrimitives(), 1);

	RHICmdList.EndRenderPass();
}

#undef USE_INDEX_BUFFER
