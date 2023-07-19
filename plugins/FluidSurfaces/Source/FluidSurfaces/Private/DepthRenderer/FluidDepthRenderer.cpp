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

#include "FluidDepthRenderer.h"

#include "EngineModule.h"
#include "FluidMeshElementCollector.h"
#include "FluidMeshPassProcessor.h"
#include "MeshPassProcessor.inl"
#include "Renderer/Private/ScenePrivate.h"
#include "Rendering/SkeletalMeshRenderData.h"
#include "SkeletalRenderPublic.h"

TRefCountPtr<IPooledRenderTarget> FFluidDepthRenderer::DepthStencilTarget;
FGlobalDynamicIndexBuffer FFluidDepthRenderer::DynamicIndexBufferForInitViews;
FGlobalDynamicVertexBuffer FFluidDepthRenderer::DynamicVertexBufferForInitViews;
TGlobalResource<FGlobalDynamicReadBuffer> FFluidDepthRenderer::DynamicReadBufferForInitViews;

FFluidDepthRenderer::FFluidDepthRenderer()
{
}

DECLARE_GPU_STAT_NAMED(FluidDepth_Render, TEXT("FluidDepth_Render"));
void FFluidDepthRenderer::DrawDepth_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FScene* InScene,
	FTransform InViewTransform,
	float InViewOrthoWidth,
	FTextureRenderTargetResource* InReadbackResource,
	TSet<FPrimitiveSceneProxy*> InStaticPrimitives,
	TSet<FPrimitiveSceneProxy*> InDynamicPrimitives,
	const bool bUseSimpleMaterial)
{
	SCOPED_NAMED_EVENT(FluidDepth_Render, FColor::Black);
	SCOPED_GPU_STAT(RHICmdList, FluidDepth_Render)

	const FIntPoint TargetSize = InReadbackResource->GetSizeXY();

	// Allocate temporary render target
	if (!DepthStencilTarget.IsValid())
	{
		FPooledRenderTargetDesc Desc = FPooledRenderTargetDesc::Create2DDesc(
			TargetSize,
			PF_DepthStencil,
			FClearValueBinding::DepthFar,
			TexCreate_None,
			TexCreate_ShaderResource | TexCreate_DepthStencilTargetable,
			true);
		Desc.DebugName = TEXT("FluidDepth_PooledRenderTarget");
		GRenderTargetPool.FindFreeElement(RHICmdList, Desc, DepthStencilTarget, Desc.DebugName);
	}

	FTextureRHIRef RenderTargetTexture = DepthStencilTarget->GetRenderTargetItem().TargetableTexture;

	FVector ViewOrigin = InViewTransform.GetLocation();
	FMatrix ViewRotationMatrix;
	GetViewRotationMatrix(InViewTransform.Rotator(), ViewRotationMatrix);
	FMatrix ProjectionMatrix;
	GetOrthographicProjectionMatrix(TargetSize, InViewOrthoWidth, ProjectionMatrix);

	// Create the view
	FSceneViewFamily::ConstructionValues ViewFamilyInit(nullptr, InScene, FEngineShowFlags(ESFIM_Game));
	ViewFamilyInit.SetWorldTimes(
		FApp::GetCurrentTime() - GStartTime,
		FApp::GetDeltaTime(),
		FApp::GetCurrentTime() - GStartTime);
	FSceneViewFamilyContext ViewFamily(ViewFamilyInit);
	ViewFamily.EngineShowFlags.LOD = 0; // TODO Need some LODs optimization here
	ViewFamily.EngineShowFlags.DisableAdvancedFeatures();
	ViewFamily.EngineShowFlags.MotionBlur = 0;

	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = &ViewFamily;
	ViewInitOptions.SetViewRectangle(FIntRect(0, 0, TargetSize.X, TargetSize.Y));
	ViewInitOptions.ViewOrigin = ViewOrigin;
	ViewInitOptions.ViewRotationMatrix = ViewRotationMatrix;
	ViewInitOptions.ProjectionMatrix = ProjectionMatrix;

	// TODO Need some optimization here
	//GetRendererModule().CreateAndInitSingleView(RHICmdList, &ViewFamily, &ViewInitOptions);
	//FViewInfo* ViewInfo = (FViewInfo*)ViewFamily.Views[0];
	FViewInfo View(ViewInitOptions);
	FViewInfo* ViewInfo = &View;
	ViewInfo->bIsSceneCapture = true;

	FBox VolumeBounds[TVC_MAX];
	FSceneRenderTargets& SceneContext = FSceneRenderTargets::Get(FRHICommandListExecutor::GetImmediateCommandList());
	TUniquePtr<FViewUniformShaderParameters> CachedViewUniformShaderParameters =
		MakeUnique<FViewUniformShaderParameters>();

	View.SetupUniformBufferParameters(SceneContext, VolumeBounds, TVC_MAX, *CachedViewUniformShaderParameters);

	View.ViewUniformBuffer = TUniformBufferRef<FViewUniformShaderParameters>::CreateUniformBufferImmediate(
		*CachedViewUniformShaderParameters,
		UniformBuffer_SingleFrame);

	//IRenderCaptureProvider::Get().BeginCapture( &RHICmdList );

	const FRHITransitionInfo TransitionInfo {
		RenderTargetTexture,
		ERHIAccess::Unknown,
		ERHIAccess::DSVWrite | ERHIAccess::DSVRead
	};
	RHICmdList.Transition(TransitionInfo);

	// No color target bound for the prepass.
	FRHIRenderPassInfo RPInfo;
	RPInfo.DepthStencilRenderTarget.DepthStencilTarget = RenderTargetTexture;
	RPInfo.DepthStencilRenderTarget.ResolveTarget = InReadbackResource->TextureRHI;
	RPInfo.DepthStencilRenderTarget.ExclusiveDepthStencil = FExclusiveDepthStencil::DepthWrite_StencilWrite;
	RPInfo.DepthStencilRenderTarget.Action =
		MakeDepthStencilTargetActions(ERenderTargetActions::Clear_Store, ERenderTargetActions::Clear_Store);

	RHICmdList.BeginRenderPass(RPInfo, TEXT("FluidDepth_RenderPass"));

	// TODO Custom ViewRect
	//RHICmdList.SetViewport(SceneView->UnscaledViewRect.Min.X, SceneView->UnscaledViewRect.Min.Y, 0.0f, SceneView->UnscaledViewRect.Max.X, SceneView->UnscaledViewRect.Max.Y, 1.0f);
	//RHICmdList.SetScissorRect(false, 0, 0, 0, 0);

	// Disable color writes, enable depth tests and writes.
	FMeshPassProcessorRenderState PassDrawRenderState(*ViewInfo);
	PassDrawRenderState.SetBlendState(TStaticBlendState<CW_NONE>::GetRHI());
	PassDrawRenderState.SetDepthStencilState(TStaticDepthStencilState<true, CF_DepthNearOrEqual>::GetRHI());

	DrawDynamicMeshPass(
		*ViewInfo,
		RHICmdList,
		[&InScene, &ViewInfo, &PassDrawRenderState, &InStaticPrimitives, &InDynamicPrimitives, bUseSimpleMaterial](
			FDynamicPassMeshDrawListContext* DynamicMeshPassContext)
		{
			FFluidDepthPassProcessor PassMeshProcessor(
				InScene,
				ViewInfo,
				PassDrawRenderState,
				true,
				InScene->EarlyZPassMode,
				InScene->bEarlyZPassMovable,
				false,
				DynamicMeshPassContext,
				false,
				bUseSimpleMaterial);

			constexpr uint64 DefaultBatchElementMask = ~0ull;

			/* -------------------------- Batch StaticMeshes -------------------------- */
			for (const FPrimitiveSceneProxy* StaticPrimitive : InStaticPrimitives)
			{
				if (!StaticPrimitive)
					continue;
				TArray<FMeshBatch> MeshBatch;
				StaticPrimitive->GetMeshDescription(0, MeshBatch);
				for (int32 Index = 0; Index < MeshBatch.Num(); ++Index)
				{
					if (MeshBatch[Index].Validate(StaticPrimitive, InScene->GetFeatureLevel()))
					{
						PassMeshProcessor.AddMeshBatch(MeshBatch[Index], DefaultBatchElementMask, StaticPrimitive);
					}
				}
			}

			/* -------------------------- Batch DynamicMeshes -------------------------- */
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

				for (const FMeshBatchAndRelevance& MeshBatchAndRelevance : ViewMeshBatches)
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

	//IRenderCaptureProvider::Get().EndCapture( &RHICmdList );
}

bool FFluidDepthRenderer::IsSceneReadyToRender(FScene* Scene)
{
	return Scene != nullptr && Scene->GetRenderScene() != nullptr && Scene->GetRenderScene()->GetFrameNumber() > 1;
}

void FFluidDepthRenderer::GetViewRotationMatrix(FRotator InRotator, FMatrix& OutViewRotationMatrix)
{
	OutViewRotationMatrix = FInverseRotationMatrix(InRotator)
		* FMatrix(FPlane(1, 0, 0, 0), FPlane(0, -1, 0, 0), FPlane(0, 0, -1, 0), FPlane(0, 0, 0, 1));
}

void FFluidDepthRenderer::GetOrthographicProjectionMatrix(
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
