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

#include "TestVelocityPass.h"

#include "Animation/SkeletalMeshActor.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineModule.h"
#include "FluidVelocityRenderer.h"
#include "FluidVelocityShaders.h"
#include "Renderer/Private/MeshDrawCommands.h"
#include "Renderer/Private/ScenePrivate.h"

ATestVelocityPass::ATestVelocityPass()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Origin"));

	DelegateHandle.Reset();
	DelegateHandle = GetRendererModule().RegisterPostOpaqueRenderDelegate(FPostOpaqueRenderDelegate::CreateLambda(
		[this](const FPostOpaqueRenderParameters& Parameters)
		{
			if (!bStartTest)
				return;

			if ((StaticMeshActor == nullptr) || (StaticMeshActor->GetStaticMeshComponent()->GetStaticMesh() == nullptr)
				|| (VelocityRT == nullptr))
				return;

			FTextureRenderTargetResource* Resource = VelocityRT->GetRenderTargetResource();
			const FTransform Transform = GetActorTransform();

			TSet<FPrimitiveSceneProxy*> StaticPrimitives;
			StaticMeshActor->GetStaticMeshComponent()->bHasMotionBlurVelocityMeshes = 1;
			StaticPrimitives.Add(StaticMeshActor->GetStaticMeshComponent()->SceneProxy);

			TSet<FPrimitiveSceneProxy*> DynamicPrimitives;
			if (SkeletalMeshActor)
			{
				DynamicPrimitives.Add(SkeletalMeshActor->GetSkeletalMeshComponent()->SceneProxy);
			}
			FRHICommandListImmediate& RHICmdList = *Parameters.RHICmdList;

			FFluidVelocityRenderer::DrawVelocity_RenderThread(
				RHICmdList,
				GetWorld()->Scene->GetRenderScene(),
				Transform,
				OrthoWidth,
				Resource,
				StaticPrimitives,
				DynamicPrimitives,
				true);
		}));
}
