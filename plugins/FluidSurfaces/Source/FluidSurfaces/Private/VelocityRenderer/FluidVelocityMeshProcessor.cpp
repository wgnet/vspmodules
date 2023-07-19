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
#include "FluidVelocityMeshProcessor.h"

#include "FluidVelocityShaders.h"
#include "MeshPassProcessor.inl"
#include "Renderer/Private/ScenePrivate.h"

IMPLEMENT_SHADER_TYPE(
	,
	FFluidVelocityVS,
	TEXT("/Engine/Private/VelocityShader.usf"),
	TEXT("MainVertexShader"),
	SF_Vertex);
IMPLEMENT_SHADER_TYPE(, FFluidVelocityHS, TEXT("/Engine/Private/VelocityShader.usf"), TEXT("MainHull"), SF_Hull);
IMPLEMENT_SHADER_TYPE(, FFluidVelocityDS, TEXT("/Engine/Private/VelocityShader.usf"), TEXT("MainDomain"), SF_Domain);
IMPLEMENT_SHADER_TYPE(
	,
	FFluidVelocityPS,
	TEXT("/Engine/Private/VelocityShader.usf"),
	TEXT("MainPixelShader"),
	SF_Pixel);
IMPLEMENT_SHADERPIPELINE_TYPE_VSPS(FluidVelocityPipeline, FFluidVelocityVS, FFluidVelocityPS, true);

bool GetFluidVelocityPassShaders(
	const FMaterial& Material,
	FVertexFactoryType* VertexFactoryType,
	ERHIFeatureLevel::Type FeatureLevel,
	TShaderRef<FFluidVelocityHS>& HullShader,
	TShaderRef<FFluidVelocityDS>& DomainShader,
	TShaderRef<FFluidVelocityVS>& VertexShader,
	TShaderRef<FFluidVelocityPS>& PixelShader)
{
	const EMaterialTessellationMode MaterialTessellationMode = Material.GetTessellationMode();

	const bool bNeedsHSDS = RHISupportsTessellation(GShaderPlatformForFeatureLevel[FeatureLevel])
		&& VertexFactoryType->SupportsTessellationShaders() && MaterialTessellationMode != MTM_NoTessellation;

	FMaterialShaderTypes ShaderTypes;

	if (bNeedsHSDS)
	{
		ShaderTypes.AddShaderType<FFluidVelocityDS>();
		ShaderTypes.AddShaderType<FFluidVelocityHS>();
	}
	else
	{
		// Don't use pipeline if we have hull/domain shaders
		ShaderTypes.PipelineType = &FluidVelocityPipeline;
	}

	ShaderTypes.AddShaderType<FFluidVelocityVS>();
	ShaderTypes.AddShaderType<FFluidVelocityPS>();

	FMaterialShaders Shaders;
	if (!Material.TryGetShaders(ShaderTypes, VertexFactoryType, Shaders))
	{
		return false;
	}

	Shaders.TryGetVertexShader(VertexShader);
	Shaders.TryGetPixelShader(PixelShader);
	Shaders.TryGetHullShader(HullShader);
	Shaders.TryGetDomainShader(DomainShader);
	return true;
}

FFluidVelocityMeshProcessor::FFluidVelocityMeshProcessor(
	const FScene* Scene,
	const FSceneView* InViewIfDynamicMeshCommand,
	const FMeshPassProcessorRenderState& InPassDrawRenderState,
	FMeshPassDrawListContext* InDrawListContext)
	: FMeshPassProcessor(Scene, Scene->GetFeatureLevel(), InViewIfDynamicMeshCommand, InDrawListContext)
{
	PassDrawRenderState = InPassDrawRenderState;
	PassDrawRenderState.SetViewUniformBuffer(
		InViewIfDynamicMeshCommand->ViewUniformBuffer /*Scene->UniformBuffers.ViewUniformBuffer*/);
	PassDrawRenderState.SetInstancedViewUniformBuffer(Scene->UniformBuffers.InstancedViewUniformBuffer);
}

bool FFluidVelocityMeshProcessor::PrimitiveHasVelocityForView(
	const FViewInfo& View,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy)
{
	// Skip camera cuts which effectively reset velocity for the new frame.
	if (View.bCameraCut && !View.PreviousViewTransform.IsSet())
	{
		return false;
	}

	const FBoxSphereBounds& PrimitiveBounds = PrimitiveSceneProxy->GetBounds();
	const float LODFactorDistanceSquared = (PrimitiveBounds.Origin - View.ViewMatrices.GetViewOrigin()).SizeSquared()
		* FMath::Square(View.LODDistanceFactor);

	// The minimum projected screen radius for a primitive to be drawn in the velocity pass, as a fraction of half the horizontal screen width (likely to be 0.08f)
	const float MinScreenRadiusForVelocityPass =
		View.FinalPostProcessSettings.MotionBlurPerObjectSize * (2.0f / 100.0f);
	const float MinScreenRadiusForVelocityPassSquared = FMath::Square(MinScreenRadiusForVelocityPass);

	// Skip primitives that only cover a small amount of screen space, motion blur on them won't be noticeable.
	if (FMath::Square(PrimitiveBounds.SphereRadius) <= MinScreenRadiusForVelocityPassSquared * LODFactorDistanceSquared)
	{
		return false;
	}

	return true;
}

bool FFluidVelocityMeshProcessor::Process(
	const FMeshBatch& MeshBatch,
	uint64 BatchElementMask,
	int32 StaticMeshId,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy,
	const FMaterialRenderProxy& MaterialRenderProxy,
	const FMaterial& MaterialResource,
	ERasterizerFillMode MeshFillMode,
	ERasterizerCullMode MeshCullMode)
{
	const FVertexFactory* VertexFactory = MeshBatch.VertexFactory;

	TMeshProcessorShaders<FFluidVelocityVS, FFluidVelocityHS, FFluidVelocityDS, FFluidVelocityPS> VelocityPassShaders;

	if (!GetFluidVelocityPassShaders(
			MaterialResource,
			VertexFactory->GetType(),
			FeatureLevel,
			VelocityPassShaders.HullShader,
			VelocityPassShaders.DomainShader,
			VelocityPassShaders.VertexShader,
			VelocityPassShaders.PixelShader))
	{
		return false;
	}

	FMeshMaterialShaderElementData ShaderElementData;
	ShaderElementData.InitializeMeshMaterialData(
		ViewIfDynamicMeshCommand,
		PrimitiveSceneProxy,
		MeshBatch,
		StaticMeshId,
		false);
	const FMeshDrawCommandSortKey SortKey =
		CalculateMeshStaticSortKey(VelocityPassShaders.VertexShader, VelocityPassShaders.PixelShader);

	BuildMeshDrawCommands(
		MeshBatch,
		BatchElementMask,
		PrimitiveSceneProxy,
		MaterialRenderProxy,
		MaterialResource,
		PassDrawRenderState,
		VelocityPassShaders,
		MeshFillMode,
		MeshCullMode,
		SortKey,
		EMeshPassFeatures::Default,
		ShaderElementData);

	return true;
}

FFluidOpaqueVelocityMeshProcessor::FFluidOpaqueVelocityMeshProcessor(
	const FScene* Scene,
	const FSceneView* InViewIfDynamicMeshCommand,
	const FMeshPassProcessorRenderState& InPassDrawRenderState,
	FMeshPassDrawListContext* InDrawListContext)
	: FFluidVelocityMeshProcessor(Scene, InViewIfDynamicMeshCommand, InPassDrawRenderState, InDrawListContext)
{
}

bool FFluidOpaqueVelocityMeshProcessor::PrimitiveCanHaveVelocity(
	EShaderPlatform ShaderPlatform,
	const FPrimitiveSceneProxy* PrimitiveSceneProxy)
{

	if (!FFluidVelocityRendering::IsSeparateVelocityPassSupported(ShaderPlatform)
		|| !PlatformSupportsVelocityRendering(ShaderPlatform))
	{
		return false;
	}

	if (!PrimitiveSceneProxy->IsMovable())
	{
		return false;
	}

	// * Whether the vertex factory for this primitive requires that it render in the separate velocity pass, as opposed to the base pass.
	// * In cases where the base pass is rendering opaque velocity for a particular mesh batch, we want to filter it out from this pass,
	// * which performs a separate draw call to render velocity.
	/*
	const bool bIsSeparateVelocityPassRequiredByVertexFactory =
		FFluidVelocityRendering::IsSeparateVelocityPassRequiredByVertexFactory(
			ShaderPlatform,
			PrimitiveSceneProxy->HasStaticLighting());
	if (!bIsSeparateVelocityPassRequiredByVertexFactory)
	{
		return false;
	}
	*/
	return true;
}

bool FFluidOpaqueVelocityMeshProcessor::PrimitiveHasVelocityForFrame(const FPrimitiveSceneProxy* PrimitiveSceneProxy)
{
	if (!PrimitiveSceneProxy->AlwaysHasVelocity())
	{
		// Check if the primitive has moved.
		const FPrimitiveSceneInfo* PrimitiveSceneInfo = PrimitiveSceneProxy->GetPrimitiveSceneInfo();
		const FScene* Scene = PrimitiveSceneInfo->Scene;
		const FMatrix& LocalToWorld = PrimitiveSceneProxy->GetLocalToWorld();
		FMatrix PreviousLocalToWorld = LocalToWorld;

		Scene->VelocityData.GetComponentPreviousLocalToWorld(
			PrimitiveSceneInfo->PrimitiveComponentId,
			PreviousLocalToWorld);

		if (LocalToWorld.Equals(PreviousLocalToWorld, 0.0001f))
		{
			// Hasn't moved (treat as background by not rendering any special velocities)
			return false;
		}
	}

	return true;
}

bool FFluidOpaqueVelocityMeshProcessor::TryAddMeshBatch(
	const FMeshBatch& RESTRICT MeshBatch,
	uint64 BatchElementMask,
	const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
	int32 StaticMeshId,
	const FMaterialRenderProxy* MaterialRenderProxy,
	const FMaterial* Material)
{
	const EBlendMode BlendMode = Material->GetBlendMode();
	const bool bIsNotTranslucent = BlendMode == BLEND_Opaque || BlendMode == BLEND_Masked;

	bool bResult = true;
	//if (MeshBatch.bUseForMaterial && bIsNotTranslucent && ShouldIncludeMaterialInDefaultOpaquePass(*Material))
	{
		// This is specifically done *before* the material swap, as swapped materials may have different fill / cull modes.
		const FMeshDrawingPolicyOverrideSettings OverrideSettings = ComputeMeshOverrideSettings(MeshBatch);
		const ERasterizerFillMode MeshFillMode = ComputeMeshFillMode(MeshBatch, *Material, OverrideSettings);
		const ERasterizerCullMode MeshCullMode = ComputeMeshCullMode(MeshBatch, *Material, OverrideSettings);

		/**
		 * Materials without masking or custom vertex modifications can be swapped out
		 * for the default material, which simplifies the shader. However, the default
		 * material also does not support being two-sided.
		 */
		const bool bSwapWithDefaultMaterial = Material->WritesEveryPixel() && !Material->IsTwoSided()
			&& !Material->MaterialModifiesMeshPosition_RenderThread();

		if (bSwapWithDefaultMaterial)
		{
			MaterialRenderProxy = UMaterial::GetDefaultMaterial(MD_Surface)->GetRenderProxy();
			Material = MaterialRenderProxy->GetMaterialNoFallback(FeatureLevel);
		}

		check(Material && MaterialRenderProxy);

		bResult = Process(
			MeshBatch,
			BatchElementMask,
			StaticMeshId,
			PrimitiveSceneProxy,
			*MaterialRenderProxy,
			*Material,
			MeshFillMode,
			MeshCullMode);
	}
	return bResult;
}

void FFluidOpaqueVelocityMeshProcessor::AddMeshBatch(
	const FMeshBatch& RESTRICT MeshBatch,
	uint64 BatchElementMask,
	const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
	int32 StaticMeshId)
{
	const EShaderPlatform ShaderPlatform = GetFeatureLevelShaderPlatform(FeatureLevel);

	if (!PrimitiveCanHaveVelocity(ShaderPlatform, PrimitiveSceneProxy))
	{
		return;
	}

	if (ViewIfDynamicMeshCommand)
	{
		if (!PrimitiveHasVelocityForFrame(PrimitiveSceneProxy))
		{
			return;
		}

		checkSlow(ViewIfDynamicMeshCommand->bIsViewInfo);
		FViewInfo* ViewInfo = (FViewInfo*)ViewIfDynamicMeshCommand;

		if (!PrimitiveHasVelocityForView(*ViewInfo, PrimitiveSceneProxy))
		{
			return;
		}
	}

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
					MaterialRenderProxy,
					Material))
			{
				break;
			}
		}

		MaterialRenderProxy = MaterialRenderProxy->GetFallback(FeatureLevel);
	}
}
