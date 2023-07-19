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

#include "MeshMaterialShader.h"

struct FFluidVelocityRendering
{
	static EPixelFormat GetFormat(EShaderPlatform ShaderPlatform);
	static FRDGTextureDesc GetRenderTargetDesc(EShaderPlatform ShaderPlatform);

	// Returns true if the separate velocity pass is enabled.
	static bool IsSeparateVelocityPassSupported(EShaderPlatform ShaderPlatform);

	// Returns true if the velocity can be output in the BasePass.
	static bool BasePassCanOutputVelocity(EShaderPlatform ShaderPlatform);

	// Returns true if the velocity can be output in the BasePass. Only valid for the current platform.
	static bool BasePassCanOutputVelocity(ERHIFeatureLevel::Type FeatureLevel);

	// Returns true if a separate velocity pass is required (i.e. not rendered by the base pass) given the provided vertex factory settings.
	static bool IsSeparateVelocityPassRequiredByVertexFactory(
		EShaderPlatform ShaderPlatform,
		bool bVertexFactoryUsesStaticLighting);
};

class FFluidVelocityVS : public FMeshMaterialShader
{
public:
	DECLARE_SHADER_TYPE(FFluidVelocityVS, MeshMaterial);
	/* 
	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		// Compile for default material.
		const bool bIsDefault = Parameters.MaterialParameters.bIsSpecialEngineMaterial;

		// Compile for masked materials.
		const bool bIsMasked = !Parameters.MaterialParameters.bWritesEveryPixel;

		// Compile for opaque and two-sided materials.
		const bool bIsOpaqueAndTwoSided = (Parameters.MaterialParameters.bIsTwoSided && !IsTranslucentBlendMode(Parameters.MaterialParameters.BlendMode));

		// Compile for materials which modify meshes.
		const bool bMayModifyMeshes = Parameters.MaterialParameters.bMaterialMayModifyMeshPosition;

		const bool bHasPlatformSupport = PlatformSupportsVelocityRendering(Parameters.Platform);
		
		
		 // Any material with a vertex factory incompatible with base pass velocity generation must generate
		 // permutations for this shader. Shaders which don't fall into this set are considered "simple" enough
		 // to swap against the default material. This massively simplifies the calculations.
		 
		const bool bIsSeparateVelocityPassRequired = (bIsDefault || bIsMasked || bIsOpaqueAndTwoSided || bMayModifyMeshes) &&
			FFluidVelocityRendering::IsSeparateVelocityPassRequiredByVertexFactory(Parameters.Platform, Parameters.VertexFactoryType->SupportsStaticLighting());
		
		// The material may explicitly override and request that it be rendered into the velocity pass.
		const bool bIsSeparateVelocityPassRequiredByMaterial = Parameters.MaterialParameters.bIsTranslucencyWritingVelocity;

		return bHasPlatformSupport && (bIsSeparateVelocityPassRequired || bIsSeparateVelocityPassRequiredByMaterial);
	}
*/
	FFluidVelocityVS() = default;
	FFluidVelocityVS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
	}
};

class FFluidVelocityHS : public FMeshMaterialShader
{
public:
	DECLARE_SHADER_TYPE(FFluidVelocityHS, MeshMaterial);

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		if (!RHISupportsTessellation(Parameters.Platform))
		{
			return false;
		}

		if (Parameters.VertexFactoryType && !Parameters.VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return false;
		}

		if (Parameters.MaterialParameters.TessellationMode == MTM_NoTessellation)
		{
			// Material controls use of tessellation
			return false;
		}

		return true;
	}
	FFluidVelocityHS() = default;
	FFluidVelocityHS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
	}
};

class FFluidVelocityDS : public FMeshMaterialShader
{
public:
	DECLARE_SHADER_TYPE(FFluidVelocityDS, MeshMaterial);

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		if (!RHISupportsTessellation(Parameters.Platform))
		{
			return false;
		}

		if (Parameters.VertexFactoryType && !Parameters.VertexFactoryType->SupportsTessellationShaders())
		{
			// VF can opt out of tessellation
			return false;
		}

		if (Parameters.MaterialParameters.TessellationMode == MTM_NoTessellation)
		{
			// Material controls use of tessellation
			return false;
		}

		return true;
	}

	FFluidVelocityDS() = default;
	FFluidVelocityDS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
	}
};

class FFluidVelocityPS : public FMeshMaterialShader
{
public:
	DECLARE_SHADER_TYPE(FFluidVelocityPS, MeshMaterial);

	static bool ShouldCompilePermutation(const FMeshMaterialShaderPermutationParameters& Parameters)
	{
		return FFluidVelocityVS::ShouldCompilePermutation(Parameters);
	}

	static void ModifyCompilationEnvironment(
		const FMaterialShaderPermutationParameters& Parameters,
		FShaderCompilerEnvironment& OutEnvironment)
	{
		FMaterialShader::ModifyCompilationEnvironment(Parameters, OutEnvironment);
		OutEnvironment.SetRenderTargetOutputFormat(0, FFluidVelocityRendering::GetFormat(Parameters.Platform));
	}

	FFluidVelocityPS() = default;
	FFluidVelocityPS(const ShaderMetaType::CompiledShaderInitializerType& Initializer)
		: FMeshMaterialShader(Initializer)
	{
	}
};
