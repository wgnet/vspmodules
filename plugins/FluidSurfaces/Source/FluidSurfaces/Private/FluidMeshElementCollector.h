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

class FFluidMeshElementCollector : public FMeshElementCollector
{
public:
	explicit FFluidMeshElementCollector(ERHIFeatureLevel::Type InFeatureLevel) : FMeshElementCollector(InFeatureLevel)
	{
	}

	void ClearViewMeshArrays()
	{
		Views.Empty();
		MeshBatches.Empty();
		SimpleElementCollectors.Empty();
		MeshIdInPrimitivePerView.Empty();
		DynamicPrimitiveShaderDataPerView.Empty();
		NumMeshBatchElementsPerView.Empty();
		DynamicIndexBuffer = nullptr;
		DynamicVertexBuffer = nullptr;
		DynamicReadBuffer = nullptr;
	}

	void AddViewMeshArrays(
		FSceneView* InView,
		TArray<FMeshBatchAndRelevance, SceneRenderingAllocator>* ViewMeshes,
		FSimpleElementCollector* ViewSimpleElementCollector,
		TArray<FPrimitiveUniformShaderParameters>* InDynamicPrimitiveShaderData,
		ERHIFeatureLevel::Type InFeatureLevel,
		FGlobalDynamicIndexBuffer* InDynamicIndexBuffer,
		FGlobalDynamicVertexBuffer* InDynamicVertexBuffer,
		FGlobalDynamicReadBuffer* InDynamicReadBuffer)
	{
		Views.Add(InView);
		MeshIdInPrimitivePerView.Add(0);
		MeshBatches.Add(ViewMeshes);
		NumMeshBatchElementsPerView.Add(0);
		SimpleElementCollectors.Add(ViewSimpleElementCollector);
		DynamicPrimitiveShaderDataPerView.Add(InDynamicPrimitiveShaderData);

		ensureMsgf(InDynamicIndexBuffer && InDynamicVertexBuffer && InDynamicReadBuffer, TEXT("Invalid Buffer"));
		DynamicIndexBuffer = InDynamicIndexBuffer;
		DynamicVertexBuffer = InDynamicVertexBuffer;
		DynamicReadBuffer = InDynamicReadBuffer;
	}

	TChunkedArray<FMeshBatch> GetMeshBatchStorage()
	{
		return MeshBatchStorage;
	}

	void SetPrimitive(const FPrimitiveSceneProxy* InPrimitiveSceneProxy, FHitProxyId DefaultHitProxyId)
	{
		ensureMsgf(
			InPrimitiveSceneProxy,
			TEXT("Invalid PrimitiveSceneProxy: %s"),
			*InPrimitiveSceneProxy->GetLevelName().ToString());
		PrimitiveSceneProxy = InPrimitiveSceneProxy;

		for (int32 ViewIndex = 0; ViewIndex < MeshIdInPrimitivePerView.Num(); ++ViewIndex)
		{
			MeshIdInPrimitivePerView[ViewIndex] = 0;
		}
	}

	void GetMeshBatches(
		const int32 InViewIndex,
		TArray<FMeshBatchAndRelevance, SceneRenderingAllocator>& OutMeshBatches)
	{
		OutMeshBatches = *MeshBatches[InViewIndex];
	}
};
