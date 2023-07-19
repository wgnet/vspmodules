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

struct TERRAFORMERRENDER_API FTerraformerRenderUtils
{
	static bool CreatePooledRenderTarget(
		FRHICommandListImmediate& RHICmdList,
		TRefCountPtr<IPooledRenderTarget>& InPooledRenderTarget,
		EPixelFormat InFormat,
		FIntPoint InSize,
		const TCHAR* DebugName);
};

struct TERRAFORMERRENDER_API FTerraformerShapeDrawData
{
	UTextureRenderTarget2D* RenderTarget;
	TArray<FVector> Vertices;
	TArray<int32> Indexes;

	int32 GetBaseVertexIndex() const
	{
		if (Indexes.IsValidIndex(0))
			return Indexes[0];
		return 0;
	}

	int32 GetNumPrimitives() const
	{
		return FMath::DivideAndRoundUp(Indexes.Num(), 3);
	}

	int32 GetMaxIndexValue() const
	{
		int32 MaxValue = 0;
		for (const int32 Index : Indexes)
			if (MaxValue < Index)
				MaxValue = Index;
		return MaxValue;
	}
};

class TERRAFORMERRENDER_API FDrawShapePassProcessor
{
public:
	static void DrawAreaShape_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FTerraformerShapeDrawData& DrawData);
	static void DrawPathShape_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FTerraformerShapeDrawData& DrawData);
};
