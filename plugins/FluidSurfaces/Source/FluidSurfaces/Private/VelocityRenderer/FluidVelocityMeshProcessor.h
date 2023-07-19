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

#include "HitProxies.h"
#include "RHI.h"
#include "RendererInterface.h"

class FPrimitiveSceneInfo;
class FPrimitiveSceneProxy;
class FScene;
class FStaticMeshBatch;
class FViewInfo;

struct FPrimitiveViewRelevance;

class FLUIDSURFACES_API FFluidVelocityMeshProcessor : public FMeshPassProcessor
{
public:
	FFluidVelocityMeshProcessor(
		const FScene* Scene,
		const FSceneView* InViewIfDynamicMeshCommand,
		const FMeshPassProcessorRenderState& InPassDrawRenderState,
		FMeshPassDrawListContext* InDrawListContext);

	FMeshPassProcessorRenderState PassDrawRenderState;

	/* Checks whether the primitive should emit velocity for the current view by comparing screen space size against a threshold. */
	static bool PrimitiveHasVelocityForView(const FViewInfo& View, const FPrimitiveSceneProxy* PrimitiveSceneProxy);

protected:
	bool Process(
		const FMeshBatch& MeshBatch,
		uint64 BatchElementMask,
		int32 StaticMeshId,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		const FMaterialRenderProxy& RESTRICT MaterialRenderProxy,
		const FMaterial& RESTRICT MaterialResource,
		ERasterizerFillMode MeshFillMode,
		ERasterizerCullMode MeshCullMode);
};

class FLUIDSURFACES_API FFluidOpaqueVelocityMeshProcessor : public FFluidVelocityMeshProcessor
{
public:
	FFluidOpaqueVelocityMeshProcessor(
		const FScene* Scene,
		const FSceneView* InViewIfDynamicMeshCommand,
		const FMeshPassProcessorRenderState& InPassDrawRenderState,
		FMeshPassDrawListContext* InDrawListContext);

	/** Returns true if the object is capable of having velocity for any frame. */
	static bool PrimitiveCanHaveVelocity(
		EShaderPlatform ShaderPlatform,
		const FPrimitiveSceneProxy* PrimitiveSceneProxy);

	/** Returns true if the primitive has velocity for the current frame. */
	static bool PrimitiveHasVelocityForFrame(const FPrimitiveSceneProxy* PrimitiveSceneProxy);

private:
	bool TryAddMeshBatch(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId,
		const FMaterialRenderProxy* MaterialRenderProxy,
		const FMaterial* Material);

public:
	virtual void AddMeshBatch(
		const FMeshBatch& RESTRICT MeshBatch,
		uint64 BatchElementMask,
		const FPrimitiveSceneProxy* RESTRICT PrimitiveSceneProxy,
		int32 StaticMeshId = -1) override final;
};
