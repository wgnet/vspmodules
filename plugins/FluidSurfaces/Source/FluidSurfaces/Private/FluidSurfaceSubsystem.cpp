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

#include "FluidSurfaceSubsystem.h"

#include "EngineUtils.h"
#include "FluidSurfaceActor.h"
#include "FluidSurfaceUtils.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Landscape.h"
#include "PhysicsEngine/PhysicsSettings.h"
#if WITH_EDITOR
	#include "DrawDebugHelpers.h"
#endif

static TAutoConsoleVariable<int32> CVarFluidSimulationEnabled(
	TEXT("r.FluidSimulation.Enabled"),
	1,
	TEXT("If all FluidSimulation rendering is enabled or disabled"),
	ECVF_Scalability);

static int32 FluidSimulationRenderTargetSize = 2048;
static FAutoConsoleVariableRef CVarFluidSimulationRenderTargetSize(
	TEXT("r.FluidSimulation.RenderTargetSize"),
	FluidSimulationRenderTargetSize,
	TEXT("Size for square fluid simulation render target"),
	ECVF_Scalability);

static int32 FluidSimulationWorldSize = 20480;
static FAutoConsoleVariableRef CVarFluidSimulationWorldSize(
	TEXT("r.FluidSimulation.WorldSize"),
	FluidSimulationWorldSize,
	TEXT("Size for square fluid simulation WorldSize"),
	ECVF_Scalability);

static int32 VirtualFPS = 60;
static FAutoConsoleVariableRef CVarFluidSimulationVirtualFPS(
	TEXT("r.FluidSimulation.VirtualFPS"),
	FluidSimulationWorldSize,
	TEXT("Target FPS for fluid simulation"),
	ECVF_Scalability);

static int32 MaxSubstepsPerFrame = 4;
static FAutoConsoleVariableRef CVarFluidSimulationMaxSubstepsPerFrame(
	TEXT("r.FluidSimulation.MaxSubstepsPerFrame"),
	FluidSimulationWorldSize,
	TEXT("Max fluid simulation Substeps per Frame"),
	ECVF_Scalability);

static TAutoConsoleVariable<int32> CVarFluidSimulationDebug(
	TEXT("r.FluidSimulation.Debug"),
	0,
	TEXT("Display size for square fluid simulation WorldSize"),
	ECVF_SetByConsole);

UFluidSurfaceSubsystem::UFluidSurfaceSubsystem()
{
}

bool UFluidSurfaceSubsystem::IsTickable() const
{
	return GetWorld()->GetNetMode() != NM_DedicatedServer;
}

bool UFluidSurfaceSubsystem::IsTickableInEditor() const
{
	return false;
}

void UFluidSurfaceSubsystem::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (GetWorld()->GetNetMode() != NM_DedicatedServer)
	{
		UWorld* World = GetWorld();
		if (!IsValid(World))
			return;

		if (const ACharacter* Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
		{
			const FVector Start = Character->GetActorLocation();

			if (bOverlapAsync == false)
				return;

			TWeakObjectPtr<UFluidSurfaceSubsystem> ThisWeakPtr = this;
			const float Radius = FluidSimulationWorldSize / 2.f;

			const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes {
				UEngineTypes::ConvertToObjectType(ECC_PhysicsBody),
				UEngineTypes::ConvertToObjectType(ECC_Pawn)
			};

			const FCollisionObjectQueryParams ObjectQueryParams =
				FFluidSurfaceUtils::ConfigureCollisionObjectParams(ObjectTypes);

			const FCollisionQueryParams Params = FFluidSurfaceUtils::ConfigureCollisionParams(
				TEXT("FluidSimulation_AsyncSweep"),
				false,
				TArray<AActor*> {},
				true,
				GetWorld());

#if WITH_EDITOR
			if (CVarFluidSimulationDebug.GetValueOnAnyThread())
			{
				DrawDebugSphere(World, Start, Radius, 32, FColor::Red, false, 0.f, 0, 12);
			}
#endif
			bOverlapAsync = false;
			AsyncTask(
				ENamedThreads::AnyBackgroundThreadNormalTask,
				[ThisWeakPtr, Start, ObjectQueryParams, Radius, Params]()
				{
					TRACE_CPUPROFILER_EVENT_SCOPE(FluidSurfaceSubsystem_OverlapAsync)

					if (!ThisWeakPtr.IsValid())
						return;

					TSet<UPrimitiveComponent*> OutComponents;
					TArray<FOverlapResult> OutOverlapResults;
					ThisWeakPtr->GetWorld()->OverlapMultiByObjectType(
						OutOverlapResults,
						Start,
						FQuat::Identity,
						ObjectQueryParams,
						FCollisionShape::MakeSphere(Radius),
						Params);

					for (const FOverlapResult& Result : OutOverlapResults)
					{
						if (Result.Component.IsValid())
						{
							if (Result.Component->IsA(UStaticMeshComponent::StaticClass()))
								OutComponents.Add(Result.GetComponent());
							if (Result.Component->IsA(USkeletalMeshComponent::StaticClass()))
								OutComponents.Add(Result.GetComponent());
						}
					}

					AsyncTask(
						ENamedThreads::GameThread,
						[ThisWeakPtr, OutOverlapResults, OutComponents]()
						{
							ThisWeakPtr->bOverlapAsync = ThisWeakPtr->OutPrimsSignature.ExecuteIfBound(OutComponents);
						});
				});
		}
	}
}

TStatId UFluidSurfaceSubsystem::GetStatId() const
{
	RETURN_QUICK_DECLARE_CYCLE_STAT(FluidSurfaceSubsystem, STATGROUP_Tickables);
}

void UFluidSurfaceSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	FConsoleVariableDelegate NotifyScalabilityChanged =
		FConsoleVariableDelegate::CreateUObject(this, &UFluidSurfaceSubsystem::NotifyScalabilityChangedInternal);
	CVarFluidSimulationRenderTargetSize->SetOnChangedCallback(NotifyScalabilityChanged);
	CVarFluidSimulationEnabled->SetOnChangedCallback(NotifyScalabilityChanged);
	CVarFluidSimulationWorldSize->SetOnChangedCallback(NotifyScalabilityChanged);
	CVarFluidSimulationVirtualFPS->SetOnChangedCallback(NotifyScalabilityChanged);
	CVarFluidSimulationMaxSubstepsPerFrame->SetOnChangedCallback(NotifyScalabilityChanged);
	//CVarFluidSimulationDebug->SetOnChangedCallback( NotifyScalabilityChanged );

	UCollisionProfile::Get()->OnLoadProfileConfig.AddUObject(this, &UFluidSurfaceSubsystem::OnLoadProfileConfig);
	AddFluidSurfaceCollisionProfile();

	OutPrimsSignature.BindUObject(this, &UFluidSurfaceSubsystem::CachePrimitives);
}

UFluidSurfaceSubsystem* UFluidSurfaceSubsystem::Get(const UWorld* InWorld)
{
	if (InWorld)
	{
		return InWorld->GetSubsystem<UFluidSurfaceSubsystem>();
	}
	return nullptr;
}

void UFluidSurfaceSubsystem::SetFluidSurfaceActor(AFluidSurfaceActor* Actor) const
{
	if (IsValid(Actor))
	{
		FluidSurfaceActor = Actor;
	}
}

AFluidSurfaceActor* UFluidSurfaceSubsystem::GetFluidSurfaceActor() const
{
	if (UWorld* World = GetWorld())
	{
		const TActorIterator<AFluidSurfaceActor> It(World);
		FluidSurfaceActor = It ? *It : nullptr;

		return FluidSurfaceActor;
	}

	return nullptr;
}

int32 UFluidSurfaceSubsystem::GetFluidSimulationRenderTargetSize()
{
	return FluidSimulationRenderTargetSize;
}

int32 UFluidSurfaceSubsystem::GetFluidSimulationWorldSize()
{
	return FluidSimulationWorldSize;
}

int32 UFluidSurfaceSubsystem::GetFluidSimulationVirtualFPS()
{
	return FMath::Clamp(VirtualFPS, 8, 120);
}

int32 UFluidSurfaceSubsystem::GetFluidSimulationMaxSubstepsPerFrame()
{
	return FMath::Clamp(MaxSubstepsPerFrame, 0, 8);
}

bool UFluidSurfaceSubsystem::IsFluidSimulationEnabled()
{
	return CVarFluidSimulationEnabled->GetBool();
}

FName UFluidSurfaceSubsystem::GetCollisionProfileName()
{
	return TEXT("FluidSurface");
}

void UFluidSurfaceSubsystem::CachePrimitives(TSet<UPrimitiveComponent*> InComponents)
{
	CachedPrimitives = MoveTemp(InComponents);
	OnCachedPrimitivesUpdate.Broadcast();
}

void UFluidSurfaceSubsystem::NotifyScalabilityChangedInternal(IConsoleVariable* CVar) const
{
	OnFluidSurfaceScalabilityChanged.Broadcast();
}

void UFluidSurfaceSubsystem::OnLoadProfileConfig(UCollisionProfile* CollisionProfile)
{
	ensureMsgf(CollisionProfile == UCollisionProfile::Get(), TEXT("Invalid CollisionProfile"));
	AddFluidSurfaceCollisionProfile();
}

void UFluidSurfaceSubsystem::AddFluidSurfaceCollisionProfile()
{
	// Make sure FluidSurfaceCollisionProfileName is added to Engine's collision profiles
	FCollisionResponseTemplate FluidSurfaceCollisionProfile;
	const FName CollisionProfileName = GetCollisionProfileName();
	if (!UCollisionProfile::Get()->GetProfileTemplate(CollisionProfileName, FluidSurfaceCollisionProfile))
	{
		FluidSurfaceCollisionProfile.Name = CollisionProfileName;
		FluidSurfaceCollisionProfile.CollisionEnabled = ECollisionEnabled::QueryOnly;
		FluidSurfaceCollisionProfile.ObjectType = ECollisionChannel::ECC_WorldStatic;
		FluidSurfaceCollisionProfile.bCanModify = false;
		FluidSurfaceCollisionProfile.ResponseToChannels = FCollisionResponseContainer::GetDefaultResponseContainer();
		FluidSurfaceCollisionProfile.ResponseToChannels.Camera = ECR_Ignore;
		FluidSurfaceCollisionProfile.ResponseToChannels.Visibility = ECR_Ignore;
		FluidSurfaceCollisionProfile.ResponseToChannels.WorldDynamic = ECR_Overlap;
		FluidSurfaceCollisionProfile.ResponseToChannels.Pawn = ECR_Overlap;
		FluidSurfaceCollisionProfile.ResponseToChannels.PhysicsBody = ECR_Overlap;
		FluidSurfaceCollisionProfile.ResponseToChannels.Destructible = ECR_Overlap;
		FluidSurfaceCollisionProfile.ResponseToChannels.Vehicle = ECR_Overlap;
#if WITH_EDITORONLY_DATA
		FluidSurfaceCollisionProfile.HelpMessage =
			TEXT("Default FluidSurface Collision Profile (Created by FluidSurface Plugin)");
#endif
		FCollisionProfilePrivateAccessor::AddProfileTemplate(FluidSurfaceCollisionProfile);
	}
}

FTraceHandle UFluidSurfaceSubsystem::RequestTrace()
{
	UWorld* World = GetWorld();
	if (World == nullptr)
		return FTraceHandle();

	const TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes {
		UEngineTypes::ConvertToObjectType(ECC_PhysicsBody),
		UEngineTypes::ConvertToObjectType(ECC_Pawn)
	};

	if (const ACharacter* Character = UGameplayStatics::GetPlayerCharacter(GetWorld(), 0))
	{
		const FCollisionObjectQueryParams ObjectQueryParams =
			FFluidSurfaceUtils::ConfigureCollisionObjectParams(ObjectTypes);
		const FCollisionQueryParams Params = FFluidSurfaceUtils::ConfigureCollisionParams(
			TEXT("FluidSimulation_AsyncSweep"),
			false,
			TArray<AActor*> {},
			true,
			this);
		const FVector Start = Character->GetActorLocation();
		const FVector End = Start + FVector::ZAxisVector;
		const FRotator Rotation = FRotator::ZeroRotator;
		const FCollisionShape CollisionShape = FCollisionShape::MakeSphere(FluidSimulationWorldSize / 2.f);
#if WITH_EDITOR
		if (CVarFluidSimulationDebug.GetValueOnAnyThread())
		{
			DrawDebugSphere(World, Start, FluidSimulationWorldSize / 2.f, 32, FColor::Red, false, 0.f, 0, 12);
		}
#endif
		return World->AsyncSweepByObjectType(
			EAsyncTraceType::Multi,
			Start,
			End,
			Rotation.Quaternion(),
			ObjectQueryParams,
			CollisionShape,
			Params,
			&TraceDelegate);
	}
	return FTraceHandle {};
}
