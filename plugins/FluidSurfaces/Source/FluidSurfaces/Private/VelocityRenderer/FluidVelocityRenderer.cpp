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
#include "FluidVelocityRenderer.h"

#include "Containers/Set.h"
#include "EngineModule.h"
#include "FluidMeshElementCollector.h"
#include "FluidVelocityMeshProcessor.h"
#include "FluidVelocityShaders.h"
#include "IRenderCaptureProvider.h"
#include "MeshPassProcessor.inl"
#include "PrimitiveSceneProxy.h"
#include "RenderTargetPool.h"
#include "Renderer/Private/ScenePrivate.h"

FGlobalDynamicIndexBuffer FFluidVelocityRenderer::DynamicIndexBufferForInitViews;
FGlobalDynamicVertexBuffer FFluidVelocityRenderer::DynamicVertexBufferForInitViews;
TGlobalResource<FGlobalDynamicReadBuffer> FFluidVelocityRenderer::DynamicReadBufferForInitViews;

void FFluidVelocityRenderer::DrawVelocity_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FScene* InScene,
	FTransform InViewTransform,
	float InViewOrthoWidth,
	FTextureRenderTargetResource* InReadbackVelocity,
	TSet<FPrimitiveSceneProxy*> InStaticPrimitives,
	TSet<FPrimitiveSceneProxy*> InDynamicPrimitives,
	const bool bUseSimpleMaterial)
{
	if (!IsInRenderingThread())
		return;

	// Allocate temporary render target

	const FIntPoint TargetSize = InReadbackVelocity->GetSizeXY();
	TRefCountPtr<IPooledRenderTarget> VelocityRenderTarget;
	FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(
		TargetSize,
		EPixelFormat::PF_A16B16G16R16,
		FClearValueBinding::Transparent,
		TexCreate_None,
		TexCreate_ShaderResource | TexCreate_RenderTargetable | TexCreate_UAV,
		true);
	Desc.DebugName = TEXT("Velocity_PooledRenderTarget");
	GRenderTargetPool.FindFreeElement(RHICmdList, Desc, VelocityRenderTarget, Desc.DebugName);

	FTextureRHIRef RenderTargetTexture = VelocityRenderTarget->GetRenderTargetItem().TargetableTexture;

	FVector ViewOrigin = InViewTransform.GetLocation();
	FMatrix ViewRotationMatrix;
	GetViewRotationMatrix(InViewTransform.Rotator(), ViewRotationMatrix);
	FMatrix ProjectionMatrix;
	GetOrthographicProjectionMatrix(TargetSize, InViewOrthoWidth, ProjectionMatrix);

	// Create the view
	FSceneViewFamily::ConstructionValues ViewFamilyInit(InReadbackVelocity, InScene, FEngineShowFlags(ESFIM_Game));
	ViewFamilyInit.SetWorldTimes(
		FApp::GetCurrentTime() - GStartTime,
		FApp::GetDeltaTime(),
		FApp::GetCurrentTime() - GStartTime);
	FSceneViewFamilyContext ViewFamily(ViewFamilyInit);
	ViewFamily.EngineShowFlags.LOD = 0;

	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.SetViewRectangle(FIntRect(0, 0, TargetSize.X, TargetSize.Y));
	ViewInitOptions.ViewOrigin = ViewOrigin;
	ViewInitOptions.ViewRotationMatrix = ViewRotationMatrix;
	ViewInitOptions.ProjectionMatrix = ProjectionMatrix;
	ViewInitOptions.ViewFamily = &ViewFamily;

	GetRendererModule().CreateAndInitSingleView(RHICmdList, &ViewFamily, &ViewInitOptions);

	const FSceneView* View = ViewFamily.Views[0];
	FViewInfo* ViewInfo = (FViewInfo*)ViewFamily.Views[0];

	RHICmdList.Transition(
		FRHITransitionInfo(RenderTargetTexture, ERHIAccess::Unknown, ERHIAccess::DSVWrite | ERHIAccess::DSVRead));

	// No color target bound for the prepass.
	FRHIRenderPassInfo RPInfo;
	RPInfo.DepthStencilRenderTarget.ExclusiveDepthStencil = FExclusiveDepthStencil::DepthNop_StencilNop;
	RPInfo.DepthStencilRenderTarget.Action = EDepthStencilTargetActions::DontLoad_DontStore;
	RPInfo.SubpassHint = ESubpassHint::None;
	RPInfo.ColorRenderTargets->RenderTarget = RenderTargetTexture;
	RPInfo.ColorRenderTargets->ResolveTarget = InReadbackVelocity->TextureRHI;
	RPInfo.ColorRenderTargets->Action = ERenderTargetActions::Clear_DontStore;

	RHICmdList.BeginRenderPass(RPInfo, TEXT("Velocity"));

	FMemMark Mark(FMemStack::Get());
	FMeshPassProcessorRenderState VelocityPassState;
	VelocityPassState.SetBlendState(TStaticBlendState<CW_RGBA>::GetRHI());
	VelocityPassState.SetDepthStencilState(TStaticDepthStencilState<false, CF_DepthNearOrEqual>::GetRHI());

	DrawDynamicMeshPass(
		*View,
		RHICmdList,
		[View, ViewInfo, InScene, InStaticPrimitives, VelocityPassState, InDynamicPrimitives](
			FDynamicPassMeshDrawListContext* DynamicMeshPassContext)
		{
			FFluidOpaqueVelocityMeshProcessor PassMeshProcessor(
				InScene,
				View,
				VelocityPassState,
				DynamicMeshPassContext);

			constexpr uint64 DefaultBatchElementMask = ~0ull;

			for (const FPrimitiveSceneProxy* StaticPrimitive : InStaticPrimitives)
			{
				if (!StaticPrimitive)
					continue;

				TArray<FMeshBatch> MeshBatches;
				StaticPrimitive->GetMeshDescription(0, MeshBatches);

				for (int32 Index = 0; Index < MeshBatches.Num(); ++Index)
				{
					const FMeshBatch& Mesh = MeshBatches[Index];
					//Mesh.MaterialRenderProxy->UpdateUniformExpressionCacheIfNeeded(View->GetFeatureLevel());
					PassMeshProcessor.AddMeshBatch(MeshBatches[Index], DefaultBatchElementMask, StaticPrimitive);
				}
			}

			if (InDynamicPrimitives.Num() > 0)
			{
				FFluidMeshElementCollector Collector(InScene->GetFeatureLevel());
				Collector.ClearViewMeshArrays();
				Collector.AddViewMeshArrays(
					ViewInfo,
					&ViewInfo->DynamicMeshElements,
					&ViewInfo->SimpleElementCollector,
					&ViewInfo->DynamicPrimitiveShaderData,
					ViewInfo->Family->GetFeatureLevel(),
					&DynamicIndexBufferForInitViews,
					&DynamicVertexBufferForInitViews,
					&DynamicReadBufferForInitViews);

				for (const FPrimitiveSceneProxy* DynamicPrimitive : InDynamicPrimitives)
				{
					if (!DynamicPrimitive)
						continue;
					Collector.SetPrimitive(DynamicPrimitive, FHitProxyId());
					DynamicPrimitive->GetDynamicMeshElements({ ViewInfo }, *ViewInfo->Family, 1, Collector);
				}

				TArray<FMeshBatchAndRelevance, SceneRenderingAllocator> ViewMeshBatches;
				Collector.GetMeshBatches(0, ViewMeshBatches);

				for (FMeshBatchAndRelevance& MeshBatchAndRelevance : ViewMeshBatches)
				{
					if (MeshBatchAndRelevance.Mesh->Validate(
							MeshBatchAndRelevance.PrimitiveSceneProxy,
							InScene->GetFeatureLevel()))
					{
						PassMeshProcessor.AddMeshBatch(
							*MeshBatchAndRelevance.Mesh,
							DefaultBatchElementMask,
							MeshBatchAndRelevance.PrimitiveSceneProxy);
					}
				}
			}
		});

	RHICmdList.EndRenderPass();
}

void FFluidVelocityRenderer::GetViewRotationMatrix(FRotator InRotator, FMatrix& OutViewRotationMatrix)
{
	OutViewRotationMatrix = FInverseRotationMatrix(InRotator)
		* FMatrix(FPlane(1, 0, 0, 0), FPlane(0, -1, 0, 0), FPlane(0, 0, -1, 0), FPlane(0, 0, 0, 1));
}

void FFluidVelocityRenderer::GetOrthographicProjectionMatrix(
	FIntPoint RenderTargetSize,
	float InOrthoWidth,
	FMatrix& OutProjectionMatrix)
{
	float const XAxisMultiplier = 1.0f;
	float const YAxisMultiplier = RenderTargetSize.X / (float)RenderTargetSize.Y;

	ensureMsgf((int32)ERHIZBuffer::IsInverted, TEXT("Check NearPlane and Projection Matrix"));
	const float OrthoWidth = InOrthoWidth / 2.0f;
	const float OrthoHeight = InOrthoWidth / 2.0f * XAxisMultiplier / YAxisMultiplier;

	const float NearPlane = 0;
	const float FarPlane = WORLD_MAX / 8.0f;

	const float ZScale = 1.0f / (FarPlane - NearPlane);
	const float ZOffset = -NearPlane;

	OutProjectionMatrix = FReversedZOrthoMatrix(OrthoWidth, OrthoHeight, ZScale, ZOffset);
}
