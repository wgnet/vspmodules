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
#include "FluidVelocityShaders.h"

#include "FluidSurfaceSubsystem.h"

EPixelFormat FFluidVelocityRendering::GetFormat(EShaderPlatform ShaderPlatform)
{
	// Android platform doesn't support unorm G16R16 format, use G16R16F instead.
	return FDataDrivenShaderPlatformInfo::GetSupportsRayTracing(ShaderPlatform)
		? PF_A16B16G16R16
		: (IsAndroidOpenGLESPlatform(ShaderPlatform) ? PF_G16R16F : PF_G16R16);
}

FRDGTextureDesc FFluidVelocityRendering::GetRenderTargetDesc(EShaderPlatform ShaderPlatform)
{
	const FIntPoint BufferSize = UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize();
	return FRDGTextureDesc::Create2D(
		BufferSize,
		GetFormat(ShaderPlatform),
		FClearValueBinding::Transparent,
		TexCreate_RenderTargetable | TexCreate_UAV | TexCreate_ShaderResource);
}

bool FFluidVelocityRendering::IsSeparateVelocityPassSupported(EShaderPlatform ShaderPlatform)
{
	return GPixelFormats[GetFormat(ShaderPlatform)].Supported;
}

bool FFluidVelocityRendering::BasePassCanOutputVelocity(EShaderPlatform ShaderPlatform)
{
	return IsUsingBasePassVelocity(ShaderPlatform);
}

bool FFluidVelocityRendering::BasePassCanOutputVelocity(ERHIFeatureLevel::Type FeatureLevel)
{
	EShaderPlatform ShaderPlatform = GetFeatureLevelShaderPlatform(FeatureLevel);
	return BasePassCanOutputVelocity(ShaderPlatform);
}

bool FFluidVelocityRendering::IsSeparateVelocityPassRequiredByVertexFactory(
	EShaderPlatform ShaderPlatform,
	bool bVertexFactoryUsesStaticLighting)
{
	// A separate pass is required if the base pass can't do it.
	const bool bBasePassVelocityNotSupported = !BasePassCanOutputVelocity(ShaderPlatform);

	// Meshes with static lighting need a separate velocity pass, but only if we are using selective render target outputs.
	const bool bVertexFactoryRequiresSeparateVelocityPass =
		IsUsingSelectiveBasePassOutputs(ShaderPlatform) && bVertexFactoryUsesStaticLighting;

	return bBasePassVelocityNotSupported || bVertexFactoryRequiresSeparateVelocityPass;
}
