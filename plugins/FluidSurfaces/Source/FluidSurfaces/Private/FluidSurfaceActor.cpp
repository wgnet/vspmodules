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

#include "FluidSurfaceActor.h"

#include "DepthRenderer/FluidDepthRenderer.h"
#include "FlowMapActor.h"
#include "FluidSurfaceSubsystem.h"
#include "FluidSurfaceUtils.h"
#include "ImpulseForcesRenderer.h"
#include "VelocityRenderer/FluidVelocityRenderer.h"

#include "Animation/SkeletalMeshActor.h"
#include "Components/BoxComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Landscape.h"
#include "Materials/MaterialParameterCollection.h"
#include "PhysicsEngine/PhysicsSettings.h"
#include "PrimitiveSceneInfo.h"
#include "Renderer/Private/ScenePrivate.h"

DECLARE_STATS_GROUP(TEXT("FluidSimulation"), STATGROUP_FluidSimulation, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("Total (GameThread)"), STAT_SimulationTotal, STATGROUP_FluidSimulation);
DECLARE_CYCLE_STAT(TEXT("DrawImpulse (GameThread)"), STAT_ImpulseForces, STATGROUP_FluidSimulation);
DECLARE_CYCLE_STAT(TEXT("FluidSimulation (RenderThread)"), STAT_SimulationFluid, STATGROUP_FluidSimulation);
DECLARE_CYCLE_STAT(TEXT("RenderDepth (RenderThread)"), STAT_RenderDepth, STATGROUP_FluidSimulation);

namespace FluidSurfaceActorLocal
{
	const TCHAR* ParameterCollectionPath(TEXT("/FluidSurfaces/Data/MPC_FluidSimulation.MPC_FluidSimulation"));
	const TCHAR* SimulationRTPath(TEXT("/FluidSurfaces/Data/RT_SimulationData.RT_SimulationData"));
	const TCHAR* SplashTexturePath(TEXT("/FluidSurfaces/Data/T_BlastDecal.T_BlastDecal"));
	constexpr float SimulationVolumeHeight = 128.f;
}

AFluidSurfaceActor::AFluidSurfaceActor()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	PrimaryActorTick.bAllowTickOnDedicatedServer = false;
	PrimaryActorTick.TickGroup = TG_PostPhysics;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	RootComponent->SetAbsolute(true, true, true);

	const FName CollisionProfileName = UFluidSurfaceSubsystem::GetCollisionProfileName();
	SurfaceCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("SimulationArea"));
	SurfaceCollision->SetupAttachment(RootComponent);
	SurfaceCollision->SetCollisionProfileName(CollisionProfileName);
	//SurfaceCollision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SurfaceCollision->SetGenerateOverlapEvents(false);
	SurfaceCollision->SetCanEverAffectNavigation(false);
	SurfaceCollision->bTraceComplexOnMove = false;

	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UMaterialParameterCollection> ParameterCollection;
		ConstructorHelpers::FObjectFinderOptional<UTextureRenderTarget2D> SimulationRT;
		ConstructorHelpers::FObjectFinderOptional<UTexture2D> SplashTexture;

		FConstructorStatics()
			: ParameterCollection(FluidSurfaceActorLocal::ParameterCollectionPath)
			, SimulationRT(FluidSurfaceActorLocal::SimulationRTPath)
			, SplashTexture(FluidSurfaceActorLocal::SplashTexturePath)
		{
		}
	};

	static FConstructorStatics ConstructorStatics;
	FluidSimulationMPC = ConstructorStatics.ParameterCollection.Get();
	SimulationRT = ConstructorStatics.SimulationRT.Get();
	SplashTexture = ConstructorStatics.SplashTexture.Get();
}

#if WITH_EDITOR
void AFluidSurfaceActor::SnapBounds()
{
	if (TargetLandscape.IsValid())
	{
		LandscapePosition = TargetLandscape->GetActorLocation();
		LandscapeSize = (TargetLandscape->GetBoundingRect() + 1).Max * TargetLandscape->GetActorScale().GetMin();
		LandscapeScale = TargetLandscape->GetActorScale();
	}
	UpdateSimulationData();
}
#endif

void AFluidSurfaceActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	UpdateSimulationData();
}

void AFluidSurfaceActor::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		Destroy();
	}
}

void AFluidSurfaceActor::BeginPlay()
{
	Super::BeginPlay();

	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
		return;

	AllocateRTs();
	UFluidSurfaceSubsystem::Get(GetWorld())->SetFluidSurfaceActor(this);
	SetActorTickEnabled(true);
}

void AFluidSurfaceActor::TickActor(float DeltaTime, ELevelTick TickType, FActorTickFunction& ThisTickFunction)
{
	Super::TickActor(DeltaTime, TickType, ThisTickFunction);

	SCOPE_CYCLE_COUNTER(STAT_SimulationTotal);

	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
	{
		return;
	}

#if WITH_EDITOR
	if (GetWorld()->WorldType == EWorldType::PIE)
	{
		if (GetWorld()->GetNetMode() == ENetMode::NM_Client)
		{
			if (GPlayInEditorID > 1)
				return;
		}
	}
#endif

	if (!UFluidSurfaceSubsystem::IsFluidSimulationEnabled())
	{
		return;
	}

	if (!UFluidSurfaceSubsystem::Get(GetWorld()))
	{
		return;
	}

	if (SimulationWorldSize != UFluidSurfaceSubsystem::GetFluidSimulationWorldSize())
	{
		SimulationWorldSize = UFluidSurfaceSubsystem::GetFluidSimulationWorldSize();
		return;
	}

	if (SimulationRT->SizeX != UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize()
		|| SimulationRT->SizeY != UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize())
	{
		AllocateRTs();
		return;
	}

	TSet<FPrimitiveSceneProxy*> CacheStaticPrimitives;
	TSet<FPrimitiveSceneProxy*> CacheDynamicPrimitives;

	const TSet<UPrimitiveComponent*> PrimitiveComponents =
		UFluidSurfaceSubsystem::Get(GetWorld())->GetCachedPrimitives();

	for (const UPrimitiveComponent* PrimitiveComponent : PrimitiveComponents)
	{
		if (!IsValid(PrimitiveComponent))
			continue;

		if (!IsValid(PrimitiveComponent) || PrimitiveComponent->IsPendingKill() || !PrimitiveComponent->IsVisible())
			continue;

		if (bBoxCollisionDetection)
		{
			const FVector BoxHalfExtent = FVector(SimulationWorldSize / 2.f, SimulationWorldSize / 2.f, 32.f);
			const FBox FluidShape { SimulationOrigin - BoxHalfExtent, SimulationOrigin + BoxHalfExtent };

			if (!FluidShape.Intersect(PrimitiveComponent->Bounds.GetBox()))
				continue;
		}
		else
		{
			SurfaceCollision->SetBoxExtent(FVector(SimulationWorldSize / 2.f, SimulationWorldSize / 2.f, 10.f), false);
			const FVector PrimBoxExtent = PrimitiveComponent->Bounds.BoxExtent;
			const FVector PrimOrigin = PrimitiveComponent->Bounds.Origin;
			const FQuat PrimQuat = PrimitiveComponent->GetComponentRotation().Quaternion();
			const FCollisionShape PrimShape = FCollisionShape::MakeBox(PrimBoxExtent);

			if (!SurfaceCollision->OverlapComponent(PrimOrigin, PrimQuat, PrimShape))
				continue;
		}

		FPrimitiveSceneProxy* SceneProxy { nullptr };
		if (const UStaticMeshComponent* StaticMeshComponent = Cast<UStaticMeshComponent>(PrimitiveComponent))
		{
			if (StaticMeshComponent->GetStaticMesh()
				&& PrimitiveComponent->GetComponentVelocity().Size() > MinSimulationSpeed)
			{
				SceneProxy = StaticMeshComponent->SceneProxy;
			}

			if (!SceneProxy)
				continue;

			CacheStaticPrimitives.Add(SceneProxy);
		}

		if (const USkeletalMeshComponent* SkeletalMeshComponent = Cast<USkeletalMeshComponent>(PrimitiveComponent))
		{
			if (const ACharacter* Character = Cast<ACharacter>(SkeletalMeshComponent->GetOwner()))
			{
				if (Character->GetMesh() && Character->GetCharacterMovement()->Velocity.Size() > MinSimulationSpeed)
				{
					SceneProxy = Character->GetMesh()->SceneProxy;
				}
			}
			else
			{
				if (SkeletalMeshComponent->SkeletalMesh && SkeletalMeshComponent->IsSimulatingPhysics()
					&& SkeletalMeshComponent->GetComponentVelocity().Size() > MinSimulationSpeed)
				{
					SceneProxy = SkeletalMeshComponent->SceneProxy;
				}
			}

			if (!SceneProxy)
				continue;

			CacheDynamicPrimitives.Add(SceneProxy);
		}
	}

	if (DrawDepth(
			CacheStaticPrimitives,
			CacheDynamicPrimitives,
			DepthRT,
			FTransform { FQuat::Identity, SimulationOrigin, FVector::OneVector },
			SimulationWorldSize))
	{
		DrawImpulseForces();
		Simulate(DeltaTime);
		UpdateSimulationData();
	}
}

bool AFluidSurfaceActor::DrawDepth(
	TSet<FPrimitiveSceneProxy*> InStaticPrimitives,
	TSet<FPrimitiveSceneProxy*> InDynamicPrimitives,
	UTextureRenderTarget2D* InDepth,
	FTransform InTransform,
	float InOrthoWidth) const
{
	if (IsValid(InDepth) && (InDepth->RenderTargetFormat == RTF_R32f))
	{
		FTextureRenderTargetResource* Resource = InDepth->GameThread_GetRenderTargetResource();
		FScene* Scene = GetWorld()->Scene->GetRenderScene();
		if (Scene == nullptr || Resource == nullptr) // run with -nullRHI?
		{
			return false;
		}
		const FTransform Transform = InTransform;
		float OrthoWidth = InOrthoWidth;

		ENQUEUE_RENDER_COMMAND(FluidDepth_Render)
		(
			[Scene, Transform, OrthoWidth, Resource, InStaticPrimitives, InDynamicPrimitives](
				FRHICommandListImmediate& RHICmdList)
			{
				SCOPE_CYCLE_COUNTER(STAT_RenderDepth);
				FFluidDepthRenderer::DrawDepth_RenderThread(
					RHICmdList,
					Scene,
					Transform,
					OrthoWidth,
					Resource,
					InStaticPrimitives,
					InDynamicPrimitives,
					true);
			});

		return true;
	}
	return false;
}

void AFluidSurfaceActor::DrawVelocity(
	TSet<FPrimitiveSceneProxy*> InStaticPrimitives,
	TSet<FPrimitiveSceneProxy*> InDynamicPrimitives,
	UTextureRenderTarget2D* InRenderTarget,
	FTransform InTransform,
	float InOrthoWidth) const
{
	if (IsValid(InRenderTarget) && (InRenderTarget->RenderTargetFormat == RTF_RGBA16f))
	{
		FTextureRenderTargetResource* Resource = InRenderTarget->GameThread_GetRenderTargetResource();
		FScene* Scene = GetWorld()->Scene->GetRenderScene();
		if (Scene == nullptr || Resource == nullptr) // run with -nullRHI?
		{
			return;
		}
		const FTransform Transform = InTransform;
		float OrthoWidth = InOrthoWidth;

		ENQUEUE_RENDER_COMMAND(FluidDepth_Render)
		(
			[Scene, Transform, OrthoWidth, Resource, InStaticPrimitives, InDynamicPrimitives](
				FRHICommandListImmediate& RHICmdList)
			{
				SCOPE_CYCLE_COUNTER(STAT_RenderDepth);
				FFluidVelocityRenderer::DrawVelocity_RenderThread(
					RHICmdList,
					Scene,
					Transform,
					OrthoWidth,
					Resource,
					InStaticPrimitives,
					InDynamicPrimitives,
					true);
			});
	}
}

void AFluidSurfaceActor::Simulate(const float DeltaTime)
{
	const int32 Iterations = SynchronizeFluidSimulation.SynchronizeSimulationSubsteps(DeltaTime);
	const int64 FrameCount = UKismetSystemLibrary::GetFrameCount();
	const int32 IterationsPerSecond = UFluidSurfaceSubsystem::GetFluidSimulationVirtualFPS();
	const int32 SubstepsPerFrame = UFluidSurfaceSubsystem::GetFluidSimulationMaxSubstepsPerFrame();

	SynchronizeFluidSimulation.SetIterationsPerSecond(IterationsPerSecond);
	SynchronizeFluidSimulation.SetIterationsPerFrameMax(SubstepsPerFrame);
	SynchronizeFluidSimulation.SetIterationsRememberedMax(SubstepsPerFrame);

	for (int32 IterPass = 0; IterPass < Iterations; IterPass++)
	{
		UpdateGridPosition();
		FFluidSimulationShaderParameters DrawParameters {
			{ RippleRTs[0], RippleRTs[1], RippleRTs[2] },
			DepthRT,
			ImpulseRT,
			SimulationRT,
			PreviousFrameOffset1,
			PreviousFrameOffset2,
			RippleFoamErasure,
			FoamDamping,
			FluidDamping,
			TravelSpeed,
			SimulationWorldSize,
			ForceMultiplier,
			ImpulseMultiplier,
			Iterations,
			IterPass < (Iterations - 1) ? false : true
		};

		ENQUEUE_RENDER_COMMAND(FluidSurface_Simulate)
		(
			[Params = MoveTemp(DrawParameters),
			 CurrentPass = SynchronizeFluidSimulation.GetCurrentPass()](FRHICommandListImmediate& RHICmdList)
			{
				SCOPE_CYCLE_COUNTER(STAT_SimulationFluid);
				FFluidSimulationRenderer FluidSimulationRenderer;
				FluidSimulationRenderer.Simulate_RenderThread(RHICmdList, Params, CurrentPass);
			});

		SynchronizeFluidSimulation.IncrementPass();
	}
}

float AFluidSurfaceActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (GetWorld()->GetNetMode() == NM_DedicatedServer)
		return DamageAmount;

	if (DamageAmount > KINDA_SMALL_NUMBER)
	{
		if (DamageEvent.IsOfType(FPointDamageEvent::ClassID))
		{
			const float PointDamage = DamageAmount;
			const FPointDamageEvent& PointDamageEvent = static_cast<const FPointDamageEvent&>(DamageEvent);
			const FVector HitLocation = PointDamageEvent.HitInfo.ImpactPoint;
			const UPrimitiveComponent* Component = PointDamageEvent.HitInfo.GetComponent();

			if (IsValid(Component) && Component->GetOwner()->IsA(AFlowMapActor::StaticClass()) && (ImpulseRT != nullptr)
				&& GetSimulationVolume().IsInside(HitLocation))
			{
				const FVector Point = GetSimulationVolume().GetClosestPointTo(HitLocation);
				const FluidSurfaceDamageData Data {
					Point,
					PointDamage,
					FMath::Min(DamageRadiusMin / SimulationWorldSize, DamageRadiusMax)
				};
				DamageData.Add(Data);
				bNeedUpdateImpulseData = true;
			}
		}

		if (DamageEvent.IsOfType(FRadialDamageEvent::ClassID))
		{
			const FRadialDamageEvent& RadialDamageEvent = static_cast<const FRadialDamageEvent&>(DamageEvent);
			const FVector Origin = RadialDamageEvent.Origin;
			const float Radius = RadialDamageEvent.Params.GetMaxRadius();
			const float RadialDamage = DamageAmount;
			for (const FHitResult& Iter : RadialDamageEvent.ComponentHits)
			{
				const UPrimitiveComponent* Component = Iter.GetComponent();
				if (IsValid(Component) && Component->GetOwner()->IsA(AFlowMapActor::StaticClass())
					&& (ImpulseRT != nullptr))
				{
					const FVector Point = GetSimulationVolume().GetClosestPointTo(Origin);
					const float Lenght = FVector(Origin - Point).Size();
					const float RadialDistance = FMath::Sqrt(FMath::Square(Lenght) + FMath::Square(Lenght));
					if (RadialDistance > SimulationWorldSize / 2.f)
						continue;
					const float ForceRadius =
						FMath::Clamp(Radius, DamageRadiusMin, DamageRadiusMax) / SimulationWorldSize;
					const FluidSurfaceDamageData Data { Point, RadialDamage, ForceRadius };
					DamageData.Add(Data);
					bNeedUpdateImpulseData = true;
					break;
				}
			}
		}
	}

	return 0;
}

float AFluidSurfaceActor::GetWaterLevel() const
{
	return GetActorLocation().Z;
}

void AFluidSurfaceActor::AllocateRTs()
{
	NormalResolution = UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize();

	if (bUseSimData && SimulationRT)
	{
		SimulationRT->ResizeTarget(NormalResolution, NormalResolution);
	}
	else
	{
		SimulationRT = FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
			SimulationRT,
			TEXT("SimData"),
			FIntPoint(NormalResolution),
			RTF_RGBA16f);
	}

	ForcesRT = FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
		ForcesRT,
		TEXT("Forces"),
		FIntPoint(NormalResolution),
		RTF_RGBA16f);

	DepthRT = FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
		DepthRT,
		TEXT("Depth"),
		FIntPoint(DepthResolution),
		RTF_R32f);

	VelocityRT = FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
		DepthRT,
		TEXT("Depth"),
		FIntPoint(VelocityResolution),
		RTF_RGBA16f);

	ImpulseRT = FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
		ImpulseRT,
		TEXT("Impulse"),
		FIntPoint(ImpulseResolution),
		RTF_RGBA8);

	RippleRTs.SetNum(3);
	RippleRTs[0] = FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
		RippleRTs[0],
		TEXT("RippleRT_A"),
		FIntPoint(NormalResolution),
		RTF_RG16f);

	RippleRTs[1] = FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
		RippleRTs[1],
		TEXT("RippleRT_B"),
		FIntPoint(NormalResolution),
		RTF_RG16f);

	RippleRTs[2] = FFluidSurfaceUtils::GetOrCreateTransientRenderTarget2D(
		RippleRTs[2],
		TEXT("RippleRT_C"),
		FIntPoint(NormalResolution),
		RTF_RG16f);
}

DECLARE_GPU_STAT_NAMED(FluidImpulse_Render, TEXT("FluidImpulse_Render"));

void AFluidSurfaceActor::DrawImpulseForces()
{
	SCOPE_CYCLE_COUNTER(STAT_ImpulseForces);

	const TArray<FluidSurfaceDamageData> DamageDadaLocal = DamageData;
	const FIntPoint Size = { ImpulseRT->SizeX, ImpulseRT->SizeY };
	TArray<FVector4> DrawImpulse;

	const FTransform VolumeTransform = FTransform { FQuat::Identity, SimulationOrigin, FVector::OneVector };
	const FVector VolumeExtent = FVector(SimulationWorldSize * 0.5f);
	for (const FluidSurfaceDamageData& Iter : DamageDadaLocal)
	{
		const FVector Pos = VolumeTransform.InverseTransformPosition(Iter.Location);
		const FVector ForcePosition = ((Pos / VolumeExtent) + 1) * 0.5f;
		DrawImpulse.Add(FVector4(ForcePosition, Iter.Radius));
	}

	const FTextureRenderTargetResource* RenderTargetResource = ImpulseRT->GameThread_GetRenderTargetResource();
	ENQUEUE_RENDER_COMMAND(FluidSurface_UpdateImpulseData)
	(
		[Resource = RenderTargetResource, Tex = SplashTexture, Data = DrawImpulse](FRHICommandListImmediate& RHICmdList)
		{
			FImpulseForcesRenderer::DrawImpulseForces_RenderThread(RHICmdList, Resource, Tex, Data);
		});

	bNeedUpdateImpulseData = false;
	DamageData.Empty();
}

void AFluidSurfaceActor::UpdateSimulationData() const
{
	if (UWorld* World = GetWorld())
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			World,
			FluidSimulationMPC,
			TEXT("SimLocation"),
			SimulationOrigin);

		UKismetMaterialLibrary::SetScalarParameterValue(
			World,
			FluidSimulationMPC,
			TEXT("SimWorldSize"),
			SimulationWorldSize);

		UKismetMaterialLibrary::SetScalarParameterValue(World, FluidSimulationMPC, TEXT("WaterLevel"), WaterLevel);

		UKismetMaterialLibrary::SetVectorParameterValue(
			World,
			FluidSimulationMPC,
			TEXT("LandscapePosition"),
			LandscapePosition);

		UKismetMaterialLibrary::SetVectorParameterValue(
			World,
			FluidSimulationMPC,
			TEXT("LandscapeSize"),
			FLinearColor(LandscapeSize.X, LandscapeSize.Y, 0, 0));
	}
}

void AFluidSurfaceActor::UpdateGridPosition()
{
	if (bUseCameraTransform)
	{
		const APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(GetWorld(), 0);
		const FVector CameraLocation = CameraManager->GetCameraLocation();
		const FRotator CameraRotation = CameraManager->GetCameraRotation();
		FVector X, Y, Z;
		UKismetMathLibrary::GetAxes(CameraRotation, X, Y, Z);
		SimulationOrigin = (SimulationWorldSize / 2.f) * X;
		SimulationOrigin += CameraLocation;
		SimulationOrigin.Z = CameraLocation.Z;
	}
	else if (const ACharacter* Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		SimulationOrigin = FVector(FVector2D(Character->GetActorLocation()), GetWaterLevel());
	}

	PreviousFrameOffset2 = (PreviousFrameLocation2 - SimulationOrigin) / SimulationWorldSize;
	PreviousFrameOffset1 = (PreviousFrameLocation1 - SimulationOrigin) / SimulationWorldSize;
	PreviousFrameLocation2 = PreviousFrameLocation1;
	PreviousFrameLocation1 = SimulationOrigin;
	SetActorLocation(FVector(SimulationOrigin.X, SimulationOrigin.Y, GetWaterLevel()));
}

FBox AFluidSurfaceActor::GetSimulationVolume() const
{
	const FVector VolumeExtent =
		FVector(SimulationWorldSize, SimulationWorldSize, FluidSurfaceActorLocal::SimulationVolumeHeight) / 2.f;
	const FVector BoxMin = FVector(SimulationOrigin - VolumeExtent);
	const FVector BoxMax = FVector(SimulationOrigin + VolumeExtent);
	return FBox(BoxMin, BoxMax);
}
