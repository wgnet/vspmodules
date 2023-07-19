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
#include "VSPMeshPaintRenderItem.h"
#include "EngineModule.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "MeshPaintingToolset/Public/MeshTexturePaintingTool.h"
#include "VSPCheck.h"
#include "Renderer/Public/MeshPassProcessor.h"


FVSPMPRenderItem::FVSPMPRenderItem(FMaterialRenderProxy* MaterialProxy) : MaterialRenderProxy(MaterialProxy)
{
}

bool FVSPMPRenderItem::Render_RenderThread(
	FRHICommandListImmediate& RHICmdList,
	FMeshPassProcessorRenderState& DrawRenderState,
	const FCanvas* Canvas)
{
	Render(RHICmdList, DrawRenderState, Canvas, MaterialRenderProxy, MeshVertices, MeshIndices);

	return true;
}

bool FVSPMPRenderItem::Render_GameThread(const FCanvas* Canvas, FRenderThreadScope& RenderScope)
{
	RenderScope.EnqueueRenderCommand(
		[Canvas, MaterialProxy = MaterialRenderProxy, Vertices = MeshVertices, Indices = MeshIndices](
			FRHICommandListImmediate& RHICmdList)
		{
			FMeshPassProcessorRenderState State;
			Render(RHICmdList, State, Canvas, MaterialProxy, Vertices, Indices);
		});

	return true;
}

void FVSPMPRenderItem::AddTriangles(const TArray<FVSPMPTriangle>& Triangles)
{
	for (const FVSPMPTriangle& Triangle : Triangles)
	{
		AddTriangle(Triangle);
	}
}

void FVSPMPRenderItem::AddTriangle(const FVSPMPTriangle& Triangle)
{
	const uint32 Index0 = MeshVertices.Add(Triangle.Vertex0);
	const uint32 Index1 = MeshVertices.Add(Triangle.Vertex1);
	const uint32 Index2 = MeshVertices.Add(Triangle.Vertex2);

	MeshIndices.Append({ Index0, Index1, Index2 });
}

void FVSPMPRenderItem::Draw(FCanvas& InCanvas)
{
	VSPCheck(InCanvas.GetRenderTarget());
	FCanvas::FCanvasSortElement& Element = InCanvas.GetSortElement(InCanvas.TopDepthSortKey());
	Element.RenderBatchArray.Add(this);
}

void FVSPMPRenderItem::Render(
	FRHICommandListImmediate& RHICmdList,
	FMeshPassProcessorRenderState& DrawRenderState,
	const FCanvas* Canvas,
	FMaterialRenderProxy* MaterialProxy,
	const TArray<FDynamicMeshVertex>& Vertices,
	const TArray<uint32>& Indices)
{
	const FRenderTarget* CanvasRenderTarget = Canvas->GetRenderTarget();
	const FIntRect ViewRect(FIntPoint(0, 0), CanvasRenderTarget->GetSizeXY());

	TUniquePtr<const FSceneViewFamily> ViewFamily = MakeUnique<const FSceneViewFamily>(
		FSceneViewFamily::ConstructionValues(CanvasRenderTarget, nullptr, FEngineShowFlags(ESFIM_Editor))
			.SetWorldTimes(0, 0, 0)
			.SetGammaCorrection(CanvasRenderTarget->GetDisplayGamma()));

	FSceneViewInitOptions ViewInitOptions;
	ViewInitOptions.ViewFamily = ViewFamily.Get();
	ViewInitOptions.SetViewRectangle(ViewRect);
	ViewInitOptions.ViewOrigin = FVector::ZeroVector;
	ViewInitOptions.ViewRotationMatrix = FMatrix::Identity;
	ViewInitOptions.ProjectionMatrix = Canvas->GetTransformStack().Top().GetMatrix();
	ViewInitOptions.BackgroundColor = FLinearColor::Black;
	ViewInitOptions.OverlayColor = FLinearColor::White;

	TUniquePtr<const FSceneView> View = MakeUnique<const FSceneView>(ViewInitOptions);


	if (Vertices.Num() && Indices.Num())
	{
		FMeshBuilderOneFrameResources Resources;
		FDynamicMeshBuilder DynamicMeshBuilder(View->GetFeatureLevel(), MAX_STATIC_TEXCOORDS, 0);
		{
			DynamicMeshBuilder.AddVertices(Vertices);
			DynamicMeshBuilder.AddTriangles(Indices);
		}

		FMeshBatch MeshBatch;
		DynamicMeshBuilder.GetMeshElement(
			FMatrix::Identity,
			MaterialProxy,
			SDPG_Foreground,
			true,
			false,
			0,
			Resources,
			MeshBatch);
		MeshBatch.MaterialRenderProxy = MaterialProxy;

		DrawRenderState.SetBlendState(TStaticBlendState<CW_RGBA>::GetRHI());
		DrawRenderState.SetDepthStencilState(TStaticDepthStencilState<false, CF_Always>::GetRHI());

		check(Resources.IsValidForRendering());

		GetRendererModule().DrawTileMesh(RHICmdList, DrawRenderState, *View, MeshBatch, false, FHitProxyId());
	}
}
