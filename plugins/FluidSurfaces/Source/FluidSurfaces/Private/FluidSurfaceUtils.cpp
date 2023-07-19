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
#include "FluidSurfaceUtils.h"

#include "EngineUtils.h"
#include "Landscape.h"
#include "PhysicsEngine/PhysicsSettings.h"

UTextureRenderTarget2D* FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
	UTextureRenderTarget2D* InRenderTarget,
	FName InRenderTargetName,
	const FIntPoint& InSize,
	ETextureRenderTargetFormat InFormat,
	const FLinearColor& InClearColor,
	bool bInAutoGenerateMipMaps)
{
	EPixelFormat PixelFormat = GetPixelFormatFromRenderTargetFormat(InFormat);
	if ((InSize.X <= 0) || (InSize.Y <= 0) || (PixelFormat == EPixelFormat::PF_Unknown))
	{
		return nullptr;
	}

	if (IsValid(InRenderTarget))
	{
		if ((InRenderTarget->SizeX == InSize.X) && (InRenderTarget->SizeY == InSize.Y)
			&& (InRenderTarget->GetFormat() == PixelFormat) && (InRenderTarget->ClearColor == InClearColor)
			&& (InRenderTarget->bAutoGenerateMips == bInAutoGenerateMipMaps))
		{
			return InRenderTarget;
		}
	}

	UTextureRenderTarget2D* NewRenderTarget2D = NewObject<UTextureRenderTarget2D>(
		GetTransientPackage(),
		MakeUniqueObjectName(GetTransientPackage(), UTextureRenderTarget2D::StaticClass(), InRenderTargetName));
	check(NewRenderTarget2D);
	NewRenderTarget2D->RenderTargetFormat = InFormat;
	NewRenderTarget2D->ClearColor = InClearColor;
	NewRenderTarget2D->bAutoGenerateMips = bInAutoGenerateMipMaps;
	NewRenderTarget2D->InitAutoFormat(InSize.X, InSize.Y);
	NewRenderTarget2D->UpdateResourceImmediate(true);

	// Flush RHI thread after creating texture render target to make sure that RHIUpdateTextureReference is executed before doing any rendering with it
	// This makes sure that Value->TextureReference.TextureReferenceRHI->GetReferencedTexture() is valid
	// so that FUniformExpressionSet::FillUniformBuffer properly uses the texture for rendering, instead of using a fallback texture
	ENQUEUE_RENDER_COMMAND(FlushRHIThreadToUpdateTextureRenderTargetReference)
	(
		[](FRHICommandListImmediate& RHICmdList)
		{
			RHICmdList.ImmediateFlush(EImmediateFlushType::FlushRHIThread);
		});

	return NewRenderTarget2D;
}

FCollisionQueryParams FFluidSurfaceUtils::ConfigureCollisionParams(
	FName TraceTag,
	bool bTraceComplex,
	const TArray<AActor*>& ActorsToIgnore,
	bool bIgnoreSelf,
	const UObject* WorldContextObject)
{
	FCollisionQueryParams Params(TraceTag, SCENE_QUERY_STAT_ONLY(FluidSurface_Trace), bTraceComplex);
	Params.bReturnPhysicalMaterial = true;
	Params.bReturnFaceIndex = !UPhysicsSettings::Get()->bSuppressFaceRemapTable;
	Params.MobilityType = EQueryMobilityType::Dynamic;
	Params.AddIgnoredActors(ActorsToIgnore);
	if (bIgnoreSelf)
	{
		const AActor* IgnoreActor = Cast<AActor>(WorldContextObject);
		if (IgnoreActor)
		{
			Params.AddIgnoredActor(IgnoreActor);
		}
		else
		{
			const UObject* CurrentObject = WorldContextObject;
			while (CurrentObject)
			{
				CurrentObject = CurrentObject->GetOuter();
				IgnoreActor = Cast<AActor>(CurrentObject);
				if (IgnoreActor)
				{
					Params.AddIgnoredActor(IgnoreActor);
					break;
				}
			}
		}
	}

	return Params;
}

FCollisionObjectQueryParams FFluidSurfaceUtils::ConfigureCollisionObjectParams(
	const TArray<TEnumAsByte<EObjectTypeQuery> >& ObjectTypes)
{
	TArray<TEnumAsByte<ECollisionChannel> > CollisionObjectTraces;
	CollisionObjectTraces.AddUninitialized(ObjectTypes.Num());

	for (auto Iter = ObjectTypes.CreateConstIterator(); Iter; ++Iter)
	{
		CollisionObjectTraces[Iter.GetIndex()] = UEngineTypes::ConvertToCollisionChannel(*Iter);
	}

	FCollisionObjectQueryParams ObjectParams;
	for (auto Iter = CollisionObjectTraces.CreateConstIterator(); Iter; ++Iter)
	{
		const ECollisionChannel& Channel = (*Iter);
		if (FCollisionObjectQueryParams::IsValidObjectQuery(Channel))
		{
			ObjectParams.AddObjectTypesToQuery(Channel);
		}
		else
		{
			UE_LOG(LogBlueprintUserMessages, Warning, TEXT("%d isn't valid object type"), (int32)Channel);
		}
	}

	return ObjectParams;
}

void FFluidSurfaceUtils::GetLandscapeCenterAndExtent(
	const ALandscape* InLandscape,
	FVector& OutCenter,
	FVector& OutExtent,
	int32& OutSubsections,
	int32& OutQuadsSize)
{
#if WITH_EDITOR
	const FIntRect LandscapeRect = InLandscape->GetBoundingRect();
	const FVector MidPoint = FVector(LandscapeRect.Min, 0.f) + FVector(LandscapeRect.Size(), 0.f) * 0.5f;
	const int32 NumSubsections = InLandscape->NumSubsections;
	const int32 ComponentSizeQuads = InLandscape->ComponentSizeQuads;

	OutCenter = InLandscape->GetTransform().TransformPosition(MidPoint);
	OutExtent = FVector(LandscapeRect.Size(), 0.f) * InLandscape->GetActorScale() * 0.5f;
#else
	InLandscape->GetActorBounds(false, OutCenter, OutExtent);
	const int32 NumSubsections = InLandscape->NumSubsections;
	const int32 ComponentSizeQuads = InLandscape->ComponentSizeQuads;
#endif
	OutSubsections = NumSubsections;
	OutQuadsSize = ComponentSizeQuads;
}

float FFluidSurfaceUtils::GetLandscapeHeightAtLocation(const ALandscape* InLandscape, const FVector& InLocation)
{
	if (InLandscape)
	{
		const TOptional<float> Height = InLandscape->GetHeightAtLocation(InLocation);
		if (Height.IsSet())
			return Height.GetValue();
	}
	return 0.f;
}

ALandscapeProxy* FFluidSurfaceUtils::FindLandscape(const AActor* ContextActor)
{
	if (ContextActor != nullptr)
	{
		const FBox WaterBodyAABB = ContextActor->GetComponentsBoundingBox();
		for (TActorIterator<ALandscapeProxy> It(ContextActor->GetWorld()); It; ++It)
		{
			if (WaterBodyAABB.Intersect(It->GetComponentsBoundingBox()))
			{
				return *It;
			}
		}
	}
	return nullptr;
}
