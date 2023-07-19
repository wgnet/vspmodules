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

struct FFluidSimulationShaderParameters
{
	TArray<UTextureRenderTarget2D*, TInlineAllocator<3>> RippleRTs;
	UTextureRenderTarget2D* BufferF;
	UTextureRenderTarget2D* BufferI;
	UTextureRenderTarget2D* BufferN;
	FVector PreviousOffsets1 = FVector::ZeroVector;
	FVector PreviousOffsets2 = FVector::ZeroVector;
	float RippleFoamErasure = 0.1f;
	float FoamDamping = 0.998f;
	float FluidDamping = 0.01f;
	float TravelSpeed = 0.9f;
	float SimulationWorldSize = 2048.f;
	float ForceMultiplier = 10.f;
	float ImpulseMultiplier = 50.f;
	int32 Iterations = 1;
	bool bComputeNormals = true;

	UTextureRenderTarget2D* GetBufferA(const int64 Frame) const
	{
		return RippleRTs[(Frame + 0) % 3];
	}
	UTextureRenderTarget2D* GetBufferB(const int64 Frame) const
	{
		return RippleRTs[(Frame + 2) % 3];
	}
	UTextureRenderTarget2D* GetBufferC(const int64 Frame) const
	{
		return RippleRTs[(Frame + 1) % 3];
	}
};

class FFluidSimulationRenderer
{
public:
	void Simulate_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FFluidSimulationShaderParameters& DrawParameters,
		int64 Frame);

private:
	TRefCountPtr<IPooledRenderTarget> CSFlowOutput;
	TRefCountPtr<IPooledRenderTarget> CSNormalOutput;
	FIntPoint CacheTextureSize;

	static void ComputeFlow_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FFluidSimulationShaderParameters& DrawParameters,
		FUnorderedAccessViewRHIRef InFlowUAV,
		FTexture2DRHIRef InBufferA,
		FTexture2DRHIRef InBufferB,
		FTexture2DRHIRef InBufferF,
		FTexture2DRHIRef InBufferI);

	static void ComputeNormals_RenderThread(
		FRHICommandListImmediate& RHICmdList,
		const FFluidSimulationShaderParameters& DrawParameters,
		FUnorderedAccessViewRHIRef InNormalUAV,
		FTexture2DRHIRef InBufferA);

	static bool CreatePooledRenderTarget(
		FRHICommandListImmediate& RHICmdList,
		TRefCountPtr<IPooledRenderTarget>& InPooledRenderTarget,
		EPixelFormat InFormat,
		FIntPoint InSize,
		const TCHAR* DebugName);
};
