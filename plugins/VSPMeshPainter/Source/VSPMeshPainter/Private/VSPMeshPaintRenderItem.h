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

#include "CoreMinimal.h"
#include "DynamicMeshBuilder.h"

struct FVSPMPTriangle
{
	FDynamicMeshVertex Vertex0 = FDynamicMeshVertex(FVector::ZeroVector);
	FDynamicMeshVertex Vertex1 = FDynamicMeshVertex(FVector::ZeroVector);
	FDynamicMeshVertex Vertex2 = FDynamicMeshVertex(FVector::ZeroVector);
};

class FVSPMPRenderItem : public FCanvasBaseRenderItem
{
public:
	FVSPMPRenderItem(FMaterialRenderProxy* MaterialProxy);

	virtual bool Render_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		FMeshPassProcessorRenderState& DrawRenderState,
		const FCanvas* Canvas) override;
	virtual bool Render_GameThread(const FCanvas* Canvas, FRenderThreadScope& RenderScope) override;
	void AddTriangle(const FVSPMPTriangle& Triangle);
	void AddTriangles(const TArray<FVSPMPTriangle>& Triangles);
	void Draw(FCanvas& InCanvas);

private:
	TArray<FDynamicMeshVertex> MeshVertices;
	TArray<uint32> MeshIndices;
	FMaterialRenderProxy* MaterialRenderProxy = nullptr;

	static void Render(
		FRHICommandListImmediate& RHICmdList,
		FMeshPassProcessorRenderState& DrawRenderState,
		const FCanvas* Canvas,
		FMaterialRenderProxy* MaterialProxy,
		const TArray<FDynamicMeshVertex>& Vertices,
		const TArray<uint32>& Indices);
};
