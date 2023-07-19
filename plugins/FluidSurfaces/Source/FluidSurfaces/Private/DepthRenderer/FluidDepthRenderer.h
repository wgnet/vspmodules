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


class FLUIDSURFACES_API FFluidDepthRenderer final
{
public:
	FFluidDepthRenderer();

	static void DrawDepth_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		FScene* InScene,
		FTransform InViewTransform,
		float InViewOrthoWidth,
		FTextureRenderTargetResource* InReadbackResource,
		TSet<FPrimitiveSceneProxy*> InStaticPrimitives,
		TSet<FPrimitiveSceneProxy*> InDynamicPrimitives,
		const bool bUseSimpleMaterial = false);

	static bool IsSceneReadyToRender(FScene* Scene);
	static void GetViewRotationMatrix(FRotator InRotator, FMatrix& OutViewRotationMatrix);
	static void GetOrthographicProjectionMatrix(
		FIntPoint RenderTargetSize,
		float InOrthoWidth,
		FMatrix& OutProjectionMatrix);

private:
	static TRefCountPtr<IPooledRenderTarget> DepthStencilTarget;
	static FGlobalDynamicIndexBuffer DynamicIndexBufferForInitViews;
	static FGlobalDynamicVertexBuffer DynamicVertexBufferForInitViews;
	static TGlobalResource<FGlobalDynamicReadBuffer> DynamicReadBufferForInitViews;
};
