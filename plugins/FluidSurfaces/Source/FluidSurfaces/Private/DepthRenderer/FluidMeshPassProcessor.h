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
#include "MeshPassProcessor.h"
#include "Renderer/Private/DepthRendering.h"


template<bool bUsePositionOnlyStream>
class TFluidDepthOnlyVS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(TFluidDepthOnlyVS, MeshMaterial);

protected:
	TFluidDepthOnlyVS()
	{
	}

	TFluidDepthOnlyVS(const FMeshMaterialShaderType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
	}

public:
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		// Only the local vertex factory supports the position-only stream
		if (bUsePositionOnlyStream)
		{
			return Parameters.VertexFactoryType->SupportsPositionOnly()
				&& Parameters.MaterialParameters.bIsSpecialEngineMaterial;
		}

		if (IsTranslucentBlendMode(Parameters.MaterialParameters.BlendMode))
		{
			return Parameters.MaterialParameters.bIsTranslucencyWritingCustomDepth;
		}

		// Only compile for the default material and masked materials
		return (
			Parameters.MaterialParameters.bIsSpecialEngineMaterial || !Parameters.MaterialParameters.bWritesEveryPixel
			|| Parameters.MaterialParameters.bMaterialMayModifyMeshPosition);
	}

	void GetShaderBindings(
		const FScene* Scene,
		ERHIFeatureLevel::Type FeatureLevel,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		const FMeshPassProcessorRenderState& DrawRenderState,
		const FDepthOnlyShaderElementData& ShaderElementData,
		FMeshDrawSingleShaderBindings& ShaderBindings) const
	{
		FMeshMaterialShader::GetShaderBindings(
			Scene,
			FeatureLevel,
			PrimitiveSceneProxy,
			MaterialRenderProxy,
			Material,
			DrawRenderState,
			ShaderElementData,
			ShaderBindings);
	}
};

template<bool bUsesMobileColorValue>
class TFluidDepthOnlyPS : public FMeshMaterialShader
{
	DECLARE_SHADER_TYPE(TFluidDepthOnlyPS, MeshMaterial);

public:
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		if (IsTranslucentBlendMode(Parameters.MaterialParameters.BlendMode))
		{
			return Parameters.MaterialParameters.bIsTranslucencyWritingCustomDepth;
		}

		return
			// Compile for materials that are masked, avoid generating permutation for other platforms if bUsesMobileColorValue is true
			((!Parameters.MaterialParameters.bWritesEveryPixel
			  || Parameters.MaterialParameters.bHasPixelDepthOffsetConnected)
			 && (!bUsesMobileColorValue || IsMobilePlatform(Parameters.Platform)))
			// Mobile uses material pixel shader to write custom stencil to color target
			|| (IsMobilePlatform(Parameters.Platform)
				&& (Parameters.MaterialParameters.bIsDefaultMaterial
					|| Parameters.MaterialParameters.bMaterialMayModifyMeshPosition));
	}

	TFluidDepthOnlyPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
		MobileColorValue.Bind(Initializer.ParameterMap, TEXT("MobileColorValue"));
	}

	static void ModifyCompilationEnvironment(
		const FMaterialShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FMeshMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);

		OutEnvironment.SetDefine(TEXT("ALLOW_DEBUG_VIEW_MODES"), AllowDebugViewmodes(Parameters.Platform));
		if (IsMobilePlatform(Parameters.Platform))
		{
			OutEnvironment.SetDefine(TEXT("OUTPUT_MOBILE_COLOR_VALUE"), bUsesMobileColorValue ? 1u : 0u);
		}
		else
		{
			OutEnvironment.SetDefine(TEXT("OUTPUT_MOBILE_COLOR_VALUE"), 0u);
		}
		OutEnvironment.SetDefine(TEXT("SCENE_TEXTURES_DISABLED"), 1u);
	}

	TFluidDepthOnlyPS()
	{
	}

	void GetShaderBindings(
		const FScene* Scene,
		ERHIFeatureLevel::Type FeatureLevel,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material,
		const FMeshPassProcessorRenderState& DrawRenderState,
		const FDepthOnlyShaderElementData& ShaderElementData,
		FMeshDrawSingleShaderBindings& ShaderBindings) const
	{
		FMeshMaterialShader::GetShaderBindings(
			Scene,
			FeatureLevel,
			PrimitiveSceneProxy,
			MaterialRenderProxy,
			Material,
			DrawRenderState,
			ShaderElementData,
			ShaderBindings);

		ShaderBindings.Add(MobileColorValue, ShaderElementData.MobileColorValue);
	}

	LAYOUT_FIELD(FShaderParameter, MobileColorValue);
};

template<bool bPositionOnly, bool bUsesMobileColorValue>
bool GetFluidDepthPassShaders(
	const FMaterial& Material,
	FVertexFactoryType* VertexFactoryType,
	ERHIFeatureLevel::Type FeatureLevel,
	TShaderRef<TFluidDepthOnlyVS<bPositionOnly> >& VertexShader,
	TShaderRef<TFluidDepthOnlyPS<bUsesMobileColorValue> >& PixelShader,
	FShaderPipelineRef& ShaderPipeline);

class FLUIDSURFACES_API FFluidDepthPassProcessor : public FMeshPassProcessor
{
public:
	FFluidDepthPassProcessor(
		const FScene* Scene,
		const FSceneView* InViewIfDynamicMeshCommand,
		const FMeshPassProcessorRenderState& InPassDrawRenderState,
		const bool InbRespectUseAsOccluderFlag,
		const EDepthDrawingMode InEarlyZPassMode,
		const bool InbEarlyZPassMovable,
		/** Whether this mesh processor is being reused for rendering a pass that marks all fading out pixels on the screen */
		const bool bDitheredLODFadingOutMaskPass,
		FMeshPassDrawListContext* InDrawListContext,
		const bool bShadowProjection = false,
		const bool bUseSimpleMaterial = false);

	virtual void AddMeshBatch(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId = -1) override final;

private:
	bool TryAddMeshBatch(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId,
		const FMaterialRenderProxy& MaterialRenderProxy,
		const FMaterial& Material);

	template<bool bPositionOnly>
	bool Process(
		const FMeshBatch& MeshBatch,
		uint64 BatchElementMask,
		int32 StaticMeshId,
		EBlendMode BlendMode,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
		const FMaterial& RESTRICT MaterialResource,
		ERasterizerFillMode MeshFillMode,
		ERasterizerCullMode MeshCullMode);

	FMeshPassProcessorRenderState PassDrawRenderState;

	const bool bRespectUseAsOccluderFlag;
	const EDepthDrawingMode EarlyZPassMode;
	const bool bEarlyZPassMovable;
	const bool bDitheredLODFadingOutMaskPass;
	const bool bShadowProjection;
	const bool bUseSimpleMaterial;
};
