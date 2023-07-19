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

#include "FluidMeshPassProcessor.h"

#include "EngineModule.h"
#include "MeshPassProcessor.inl"
#include "Renderer/Private/ScenePrivate.h"

IMPLEMENT_MATERIAL_SHADER_TYPE(
	template<>
	,
	TFluidDepthOnlyVS<true>,
	TEXT("/Engine/Private/PositionOnlyDepthVertexShader.usf"),
	TEXT("Main"),
	SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(
	template<>
	,
	TFluidDepthOnlyVS<false>,
	TEXT("/Engine/Private/DepthOnlyVertexShader.usf"),
	TEXT("Main"),
	SF_Vertex);
IMPLEMENT_MATERIAL_SHADER_TYPE(
	template<>
	,
	TFluidDepthOnlyPS<true>,
	TEXT("/Engine/Private/DepthOnlyPixelShader.usf"),
	TEXT("Main"),
	SF_Pixel);
IMPLEMENT_MATERIAL_SHADER_TYPE(
	template<>
	,
	TFluidDepthOnlyPS<false>,
	TEXT("/Engine/Private/DepthOnlyPixelShader.usf"),
	TEXT("Main"),
	SF_Pixel);

IMPLEMENT_SHADERPIPELINE_TYPE_VS(DepthNoPixelPipelineCopy, TFluidDepthOnlyVS<false>, true);
IMPLEMENT_SHADERPIPELINE_TYPE_VS(DepthPosOnlyNoPixelPipelineCopy, TFluidDepthOnlyVS<true>, true);
IMPLEMENT_SHADERPIPELINE_TYPE_VSPS(
	DepthNoColorOutputPipelineCopy,
	TFluidDepthOnlyVS<false>,
	TFluidDepthOnlyPS<false>,
	true);
IMPLEMENT_SHADERPIPELINE_TYPE_VSPS(
	DepthWithColorOutputPipelineCopy,
	TFluidDepthOnlyVS<false>,
	TFluidDepthOnlyPS<true>,
	true);

FFluidDepthPassProcessor::FFluidDepthPassProcessor(
	const FScene* Scene,
	const FSceneView* InViewIfDynamicMeshCommand,
	const FMeshPassProcessorRenderState& InPassDrawRenderState,
	const bool InbRespectUseAsOccluderFlag,
	const EDepthDrawingMode InEarlyZPassMode,
	const bool InbEarlyZPassMovable,
	const bool bDitheredLODFadingOutMaskPass,
	FMeshPassDrawListContext* InDrawListContext,
	const bool bInShadowProjection,
	const bool bInUseSimpleMaterial)
	: FMeshPassProcessor(Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, InDrawListContext)
	, bRespectUseAsOccluderFlag(InbRespectUseAsOccluderFlag)
	, EarlyZPassMode(InEarlyZPassMode)
	, bEarlyZPassMovable(InbEarlyZPassMovable)
	, bDitheredLODFadingOutMaskPass(bDitheredLODFadingOutMaskPass)
	, bShadowProjection(bInShadowProjection)
	, bUseSimpleMaterial(bInUseSimpleMaterial)
{
	PassDrawRenderState = InPassDrawRenderState;
	PassDrawRenderState.SetViewUniformBuffer(InViewIfDynamicMeshCommand->ViewUniformBuffer);
	PassDrawRenderState.SetInstancedViewUniformBuffer(Scene->UniformBuffers.InstancedViewUniformBuffer);
}

void FFluidDepthPassProcessor::AddMeshBatch(
	const FMeshBatch& RESTRICT MeshBatch,
	uint64 BatchElementMask,
	const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
	int32 StaticMeshId)
{
	bool bDraw = MeshBatch.bUseForDepthPass;
	// Filter by occluder flags and settings if required.
	if (bDraw && bRespectUseAsOccluderFlag && !MeshBatch.bUseAsOccluder && EarlyZPassMode < DDM_AllOpaque)
	{
		if (PrimitiveSceneProxy)
		{
			// Only render primitives marked as occluders.
			bDraw = PrimitiveSceneProxy->ShouldUseAsOccluder()
				// Only render static objects unless movable are requested.
				&& (!PrimitiveSceneProxy->IsMovable() || bEarlyZPassMovable);
		}
		else
		{
			bDraw = false;
		}
	}
	// Determine the mesh's material and blend mode.
	if (bDraw)
	{
		const FMaterialRenderProxy* MaterialRenderProxy = MeshBatch.MaterialRenderProxy;
		while (MaterialRenderProxy)
		{
			const FMaterial* Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
			if (Material && Material->GetRenderingThreadShaderMap())
			{
				if (TryAddMeshBatch(
						MeshBatch,
						BatchElementMask,
						PrimitiveSceneProxy,
						StaticMeshId,
						*MaterialRenderProxy,
						*Material))
				{
					break;
				}
			}
			MaterialRenderProxy = MaterialRenderProxy->GetFallback(FeatureLevel);
		}
	}
}

bool FFluidDepthPassProcessor::TryAddMeshBatch(
	const FMeshBatch& MeshBatch,
	uint64 BatchElementMask,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	int32 StaticMeshId,
	const FMaterialRenderProxy& MaterialRenderProxy,
	const FMaterial& Material)
{
	const EBlendMode BlendMode = Material.GetBlendMode();
	const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
	const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(MeshBatch, Material, OverrideSettings);
	const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(MeshBatch, Material, OverrideSettings);
	const bool bIsTranslucent = IsTranslucentBlendMode(BlendMode);

	bool bResult = true;
	if (!bIsTranslucent && (!PrimitiveSceneProxy || PrimitiveSceneProxy->ShouldRenderInDepthPass())
		&& ShouldIncludeDomainInMeshPass(Material.GetMaterialDomain())
		&& ShouldIncludeMaterialInDefaultOpaquePass(Material))
	{
		constexpr bool bClip = true;
		if (BlendMode == BLEND_Opaque && EarlyZPassMode != DDM_MaskedOnly
			&& MeshBatch.VertexFactory->SupportsPositionOnlyStream()
			&& !Material.MaterialModifiesMeshPosition_RenderThread() && Material.WritesEveryPixel())
		{
			const FMaterialRenderProxy& DefaultProxy = *UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
			const FMaterial& DefaultMaterial = *DefaultProxy.GetMaterialNoFallback(FeatureLevel);
			bResult = Process<true>(
				MeshBatch,
				BatchElementMask,
				StaticMeshId,
				BlendMode,
				PrimitiveSceneProxy,
				DefaultProxy,
				DefaultMaterial,
				MeshFillMode,
				bClip ? CM_None : ERasterizerCullMode_Num);
		}
		else
		{
			const bool bMaterialMasked = !Material.WritesEveryPixel() || Material.IsTranslucencyWritingCustomDepth();

			if (bUseSimpleMaterial)
			{
				const FMaterialRenderProxy* EffectiveMaterialRenderProxy =
					UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
				const FMaterial* EffectiveMaterial = EffectiveMaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
				bResult = Process<false>(
					MeshBatch,
					BatchElementMask,
					StaticMeshId,
					BlendMode,
					PrimitiveSceneProxy,
					*EffectiveMaterialRenderProxy,
					*EffectiveMaterial,
					MeshFillMode,
					bClip ? CM_None : MeshCullMode);
			}
			else if (
				(!bMaterialMasked && EarlyZPassMode != DDM_MaskedOnly)
				|| (bMaterialMasked && EarlyZPassMode != DDM_NonMaskedOnly))
			{
				const FMaterialRenderProxy* EffectiveMaterialRenderProxy = &MaterialRenderProxy;
				const FMaterial* EffectiveMaterial = &Material;

				if (!bMaterialMasked && !Material.MaterialModifiesMeshPosition_RenderThread())
				{
					// Override with the default material for opaque materials that are not two sided
					EffectiveMaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
					EffectiveMaterial = EffectiveMaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
					ensureMsgf(EffectiveMaterial, TEXT("Invalid EffectiveMaterial"));
				}

				bResult = Process<false>(
					MeshBatch,
					BatchElementMask,
					StaticMeshId,
					BlendMode,
					PrimitiveSceneProxy,
					*EffectiveMaterialRenderProxy,
					*EffectiveMaterial,
					MeshFillMode,
					bClip ? CM_None : MeshCullMode);
			}
		}
	}

	return bResult;
}

template<bool bPositionOnly, bool bUsesMobileColorValue>
bool GetFluidDepthPassShaders(
	const FMaterial& Material,
	FVertexFactoryType* VertexFactoryType,
	ERHIFeatureLevel::Type FeatureLevel,
	TShaderRef<TFluidDepthOnlyVS<bPositionOnly>>& VertexShader,
	TShaderRef<TFluidDepthOnlyPS<bUsesMobileColorValue>>& PixelShader,
	FShaderPipelineRef& ShaderPipeline)
{
	FMaterialShaderTypes ShaderTypes;
	ShaderTypes.AddShaderType<TFluidDepthOnlyVS<bPositionOnly>>();

	if (bPositionOnly && !bUsesMobileColorValue)
	{
		ShaderTypes.PipelineType = &DepthPosOnlyNoPixelPipelineCopy;
	}
	else
	{
		const bool bNeedsPixelShader = bUsesMobileColorValue || !Material.WritesEveryPixel()
			|| Material.MaterialUsesPixelDepthOffset() || Material.IsTranslucencyWritingCustomDepth();

		if (bNeedsPixelShader)
		{
			ShaderTypes.AddShaderType<TFluidDepthOnlyPS<bUsesMobileColorValue>>();
		}

		if (bNeedsPixelShader)
		{
			if (bUsesMobileColorValue)
				ShaderTypes.PipelineType = &DepthWithColorOutputPipelineCopy;
			else
				ShaderTypes.PipelineType = &DepthNoColorOutputPipelineCopy;
		}
		else
		{
			ShaderTypes.PipelineType = &DepthNoPixelPipelineCopy;
		}
	}

	FMaterialShaders Shaders;
	if (!Material.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))
	{
		return false;
	}

	Shaders.TryGetPipeline(ShaderPipeline);
	Shaders.TryGetVertexShader(VertexShader);
	Shaders.TryGetPixelShader(PixelShader);

	return true;
}

#define IMPLEMENT_GetFluidDepthPassShaders(bPositionOnly, bUsesMobileColorValue)  \
	template bool GetFluidDepthPassShaders<bPositionOnly, bUsesMobileColorValue>( \
		const FMaterial& Material,                                                \
		FVertexFactoryType* VertexFactoryType,                                    \
		ERHIFeatureLevel::Type FeatureLevel,                                      \
		TShaderRef<TFluidDepthOnlyVS<bPositionOnly>>& VertexShader,               \
		TShaderRef<TFluidDepthOnlyPS<bUsesMobileColorValue>>& PixelShader,        \
		FShaderPipelineRef& ShaderPipeline);

IMPLEMENT_GetFluidDepthPassShaders(true, false);
IMPLEMENT_GetFluidDepthPassShaders(false, false);
IMPLEMENT_GetFluidDepthPassShaders(false, true);

template<bool bPositionOnly>
bool FFluidDepthPassProcessor::Process(
	const FMeshBatch& MeshBatch,
	uint64 BatchElementMask,
	int32 StaticMeshId,
	EBlendMode BlendMode,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMaterialRenderProxy& MaterialRenderProxy,
	const FMaterial& MaterialResource,
	ERasterizerFillMode MeshFillMode,
	ERasterizerCullMode MeshCullMode)
{
	const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

	TMeshProcessorShaders<TFluidDepthOnlyVS<bPositionOnly>, FBaseHS, FBaseDS, TFluidDepthOnlyPS<false>>
		DepthPassShaders;

	FShaderPipelineRef ShaderPipeline;

	if (!GetFluidDepthPassShaders<bPositionOnly, false>(
			MaterialResource,
			VertexFactory->GetType(),
			FeatureLevel,
			DepthPassShaders.VertexShader,
			DepthPassShaders.PixelShader,
			ShaderPipeline))
	{
		return true;
	}

	FMeshPassProcessorRenderState DrawRenderState(PassDrawRenderState);

	FDepthOnlyShaderElementData ShaderElementData(0.0f);
	ShaderElementData.InitializeMeshMaterialData(
		ViewIfDynamicMeshCommand,
		PrimitiveSceneProxy,
		MeshBatch,
		StaticMeshId,
		true);

	constexpr bool EarlyZSortMasked = true;
	FMeshDrawCommandSortKey SortKey;
	if (EarlyZSortMasked)
	{
		SortKey.BasePass.VertexShaderHash = PointerHash(DepthPassShaders.VertexShader.GetShader()) & 0xFFFF;
		SortKey.BasePass.PixelShaderHash = PointerHash(DepthPassShaders.PixelShader.GetShader());
		SortKey.BasePass.Masked = BlendMode == EBlendMode::BLEND_Masked ? 1 : 0;
	}
	else
	{
		SortKey.Generic.VertexShaderHash = PointerHash(DepthPassShaders.VertexShader.GetShader());
		SortKey.Generic.PixelShaderHash = PointerHash(DepthPassShaders.PixelShader.GetShader());
	}

	FMeshPassProcessor::BuildMeshDrawCommands(
		MeshBatch,
		BatchElementMask,
		PrimitiveSceneProxy,
		MaterialRenderProxy,
		MaterialResource,
		DrawRenderState,
		DepthPassShaders,
		MeshFillMode,
		MeshCullMode,
		SortKey,
		bPositionOnly ? EMeshPassFeatures::PositionOnly : EMeshPassFeatures::Default,
		ShaderElementData);

	return true;
}
