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
#include "FXSpawnLibrary.h"
#include "FXConfigTypes.h"
#include "FXContainerTypes.h"
#include "FXControlComponent.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Particles/ParticleSystem.h"
#include "VSPEmitter.h"
#include "VSPNiagaraEmitter.h"

namespace FFXSpawnLibraryLocal
{
	struct FFXSpawnContext
	{
		UFXSystemAsset* FXSystem = nullptr;
		UParticleSystem* ParticleSystem = nullptr;
		UNiagaraSystem* NiagaraSystem = nullptr;
		UWorld* World = nullptr;
		AActor* AttachToActor = nullptr;
		bool bUseEmitterActorForSpawn = false;
		uint8 EffectPriority = 0;
		const FTransform* SpawnTransform = nullptr;
		const FFXEffectData* EffectData = nullptr;

		bool IsValid() const
		{
			return World && AttachToActor && SpawnTransform;
		};

		FFXSpawnContext() = default;
		FFXSpawnContext(
			UFXSystemAsset* InFXSystem,
			UWorld* InWorld,
			AActor* InAttachToActor,
			const FFXEffectDataAttached& InConfig);
		FFXSpawnContext(
			UFXSystemAsset* InFXSystem,
			UWorld* InWorld,
			AActor* InAttachToActor,
			const FTransform* InSpawnTransform);
	};

	FFXSpawnContext::FFXSpawnContext(
		UFXSystemAsset* InFXSystem,
		UWorld* InWorld,
		AActor* InAttachToActor,
		const FFXEffectDataAttached& InConfig)
	{
		FXSystem = InFXSystem;
		ParticleSystem = Cast<UParticleSystem>(InFXSystem);
		NiagaraSystem = Cast<UNiagaraSystem>(InFXSystem);
		World = InWorld;
		EffectData = &InConfig.FXData;

		AttachToActor = InAttachToActor;
		bUseEmitterActorForSpawn = InConfig.bUseEmitterActorForSpawn;
		EffectPriority = InConfig.EffectPriority;
		SpawnTransform = &InConfig.FXData.FXOffset;
	}

	FFXSpawnContext::FFXSpawnContext(
		UFXSystemAsset* InFXSystem,
		UWorld* InWorld,
		AActor* InAttachToActor,
		const FTransform* InSpawnTransform)
	{
		FXSystem = InFXSystem;
		ParticleSystem = Cast<UParticleSystem>(InFXSystem);
		NiagaraSystem = Cast<UNiagaraSystem>(InFXSystem);
		World = InWorld;

		AttachToActor = InAttachToActor;
		SpawnTransform = InSpawnTransform;
	}

	ENCPoolMethod GetNCPoolMethod(EPSCPoolMethod PoolMethod)
	{
		switch (PoolMethod)
		{
		case EPSCPoolMethod::None:
			return ENCPoolMethod::None;
		case EPSCPoolMethod::AutoRelease:
			return ENCPoolMethod::AutoRelease;
		case EPSCPoolMethod::ManualRelease:
			return ENCPoolMethod::ManualRelease;
		case EPSCPoolMethod::ManualRelease_OnComplete:
			return ENCPoolMethod::ManualRelease_OnComplete;
		case EPSCPoolMethod::FreeInPool:
			return ENCPoolMethod::FreeInPool;
		default:
			checkNoEntry();
		}
		return ENCPoolMethod::None;
	}

	FAttachmentTransformRules GetAttachmentTransformRules(EAttachLocation::Type AttachLocation)
	{
		switch (AttachLocation)
		{
		case EAttachLocation::KeepRelativeOffset:
			return FAttachmentTransformRules::KeepRelativeTransform;
		case EAttachLocation::KeepWorldPosition:
			return FAttachmentTransformRules::KeepWorldTransform;
		case EAttachLocation::SnapToTarget:
			return FAttachmentTransformRules::SnapToTargetNotIncludingScale;
		case EAttachLocation::SnapToTargetIncludingScale:
			return FAttachmentTransformRules::SnapToTargetIncludingScale;
		default:
			checkNoEntry();
		}
		return FAttachmentTransformRules::KeepRelativeTransform;
	}

	UFXSystemComponent* SpawnFXAttachedAsComponent(
		const FFXSpawnContext& Context,
		USceneComponent* AttachToComponent,
		const FName& SocketName,
		const FTransform& SpawnTransform,
		EAttachLocation::Type AttachmentRule,
		EPSCPoolMethod PoolMethod,
		bool bAutoActivate,
		bool bAutoDestroy)
	{
		UFXSystemComponent* FXSystemComponent = nullptr;
		if (Context.ParticleSystem)
		{
			UParticleSystemComponent* ParticleSystem = UGameplayStatics::SpawnEmitterAttached(
				Context.ParticleSystem,
				AttachToComponent,
				SocketName,
				SpawnTransform.GetLocation(),
				SpawnTransform.GetRotation().Rotator(),
				SpawnTransform.GetScale3D(),
				AttachmentRule,
				bAutoDestroy,
				PoolMethod,
				bAutoActivate);

			if (ParticleSystem)
			{
				//we should use GetVisibleFlag instead of IsVisible,
				//because of AttachComponent can be hidden collision component so IsVisible will return false in this case
				ParticleSystem->SetVisibility(AttachToComponent->GetVisibleFlag());
				if (Context.EffectData)
				{
					ParticleSystem->CustomTimeDilation = Context.EffectData->FXPlayRate;
				}
			}
			FXSystemComponent = ParticleSystem;
		}
		else if (Context.NiagaraSystem)
		{
			UNiagaraComponent* NiagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttached(
				Context.NiagaraSystem,
				AttachToComponent,
				SocketName,
				SpawnTransform.GetLocation(),
				SpawnTransform.GetRotation().Rotator(),
				AttachmentRule,
				bAutoDestroy,
				bAutoActivate,
				GetNCPoolMethod(PoolMethod));

			if (NiagaraComponent)
			{
				NiagaraComponent->SetVisibility(AttachToComponent->GetVisibleFlag());
				NiagaraComponent->SetWorldScale3D(SpawnTransform.GetScale3D());
			}
			FXSystemComponent = NiagaraComponent;
		}

		return FXSystemComponent;
	}

	UFXSystemComponent* SpawnFXAttachedAsActor(
		const FFXSpawnContext& Context,
		USceneComponent* AttachToComponent,
		const FName& SocketName,
		const FTransform& SpawnTransform,
		EAttachLocation::Type AttachmentRule)
	{
		if (Context.ParticleSystem)
		{
			AVSPEmitter* Emitter = Context.World->SpawnActorDeferred<AVSPEmitter>(
				AVSPEmitter::StaticClass(),
				SpawnTransform,
				Context.AttachToActor,
				Context.AttachToActor ? Context.AttachToActor->GetInstigator() : nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			if (Emitter)
			{
				const FAttachmentTransformRules AttachmentTransformRules =
					FFXSpawnLibraryLocal::GetAttachmentTransformRules(AttachmentRule);
				Emitter->SetTemplate(Context.ParticleSystem);
				Emitter->AttachToComponentFollowDestroy(
					Context.AttachToActor,
					AttachToComponent,
					AttachmentTransformRules,
					SocketName);
				Emitter->bDestroyOnSystemFinish = true;
				if (UParticleSystemComponent* PSC = Emitter->GetParticleSystemComponent())
				{
					PSC->SetVisibility(AttachToComponent->GetVisibleFlag());
				}
				if (Context.EffectData)
				{
					Emitter->CustomTimeDilation = Context.EffectData->FXPlayRate;
				}
				Emitter->FinishSpawning(SpawnTransform);
			}

			return Emitter->GetParticleSystemComponent();
		}
		if (Context.NiagaraSystem)
		{
			AVSPNiagaraEmitter* Emitter = Context.World->SpawnActorDeferred<AVSPNiagaraEmitter>(
				AVSPNiagaraEmitter::StaticClass(),
				SpawnTransform,
				Context.AttachToActor,
				Context.AttachToActor ? Context.AttachToActor->GetInstigator() : nullptr,
				ESpawnActorCollisionHandlingMethod::AlwaysSpawn);

			if (Emitter)
			{
				const FAttachmentTransformRules AttachmentTransformRules(EAttachmentRule::KeepRelative, false);
				Emitter->SetAsset(Context.NiagaraSystem);
				Emitter->AttachToComponentFollowDestroy(
					Context.AttachToActor,
					AttachToComponent,
					AttachmentTransformRules,
					SocketName);
				Emitter->bDestroyOnSystemFinish = true;
				if (Emitter->NiagaraComponent)
				{
					Emitter->NiagaraComponent->SetVisibility(AttachToComponent->GetVisibleFlag());
				}
				if (Context.EffectData)
				{
					Emitter->CustomTimeDilation = Context.EffectData->FXPlayRate;
				}
				Emitter->FinishSpawning(SpawnTransform);
			}

			return Emitter->NiagaraComponent;
		}

		checkNoEntry();
		return nullptr;
	}

	void UpdateAttachedEffectsAfterSpawn(
		UFXSystemComponent* FXComponent,
		const FFXParametersParams& FXParams,
		UFXControlComponent* ControlComponent,
		USceneComponent* AttachToComponent,
		const FName& SocketName,
		uint8 EffectPriority)
	{
		if (FXComponent)
		{
			FFXSpawnLibrary::UpdateFXParameters(FXComponent, FXParams);
			if (ControlComponent)
			{
				ControlComponent->AddEffectInSocket(FXComponent, AttachToComponent, SocketName, EffectPriority);
			}
		}
	}

	UFXSystemComponent* CreateFXComponentAtLocation(
		UFXSystemAsset* FXSystem,
		UWorld* World,
		EPSCPoolMethod PoolMethod,
		const FVector& SpawnLocation,
		const FRotator& Rotation,
		const FVector& Scale,
		const FTransform& FXOffset,
		const FFXEffectData* EffectData,
		bool bAutoActivate,
		bool bAutoDestroy)
	{
		if (UParticleSystem* ParticleSystem = Cast<UParticleSystem>(FXSystem))
		{
			UParticleSystemComponent* ParticleSystemComponent = UGameplayStatics::SpawnEmitterAtLocation(
				World,
				ParticleSystem,
				SpawnLocation + FXOffset.GetLocation(),
				FXOffset.Rotator() + Rotation,
				FXOffset.GetScale3D() * Scale,
				bAutoDestroy,
				PoolMethod,
				bAutoActivate);

			if (ParticleSystemComponent && EffectData)
			{
				ParticleSystemComponent->CustomTimeDilation = EffectData->FXPlayRate;
			}
			return ParticleSystemComponent;
		}

		if (UNiagaraSystem* NiagaraSystem = Cast<UNiagaraSystem>(FXSystem))
		{
			return UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				World,
				NiagaraSystem,
				SpawnLocation + FXOffset.GetLocation(),
				FXOffset.Rotator() + Rotation,
				FXOffset.GetScale3D() * Scale,
				bAutoDestroy,
				bAutoActivate,
				FFXSpawnLibraryLocal::GetNCPoolMethod(PoolMethod));
		}

		return nullptr;
	}

	UFXSystemComponent* SpawnFXAtLocationInternal(
		const FFXEffectData& Config,
		UWorld* World,
		AActor* Instigator,
		EPSCPoolMethod PoolMethod,
		bool bAutoActivate,
		bool bAutoDestroy,
		const FVector& SpawnLocation,
		const FRotator& Rotation = FRotator::ZeroRotator,
		const FVector& Scale = FVector::OneVector)
	{
		check(World);

		//Here you can use your relathionship methods
		const bool bAlly = false;
		UFXSystemAsset* FXSystem = bAlly ? Config.FXAlly : Config.FXEnemy;

		UFXSystemComponent* FXComponent = CreateFXComponentAtLocation(
			FXSystem,
			World,
			PoolMethod,
			SpawnLocation,
			Rotation,
			Scale,
			Config.FXOffset,
			&Config,
			bAutoActivate,
			bAutoDestroy);

		if (FXComponent)
		{
			FFXSpawnLibrary::UpdateFXParameters(FXComponent, Config.FXParametersData);
		}

		return FXComponent;
	}

	UFXSystemComponent* CreateFXComponentAttached(
		const FFXSpawnContext& Context,
		const FFXParametersParams& FXParams,
		UFXControlComponent* ControlComponent,
		USceneComponent* AttachToComponent,
		const FName& SocketName,
		const FVector& Location,
		const FRotator& Rotation,
		const FVector& Scale,
		EAttachLocation::Type AttachType,
		EPSCPoolMethod PoolMethod,
		bool bAutoActivate,
		bool bAutoDestroy)
	{
		check(Context.IsValid());
		check(AttachToComponent);

		if (ControlComponent)
		{
			UFXSystemComponent* FXSystemComponent =
				ControlComponent->TryIncreaseEffectsNum(Context.FXSystem, AttachToComponent, SocketName);
			if (FXSystemComponent)
			{
				return FXSystemComponent;
			}
		}

		UFXSystemComponent* FXSystemComponent = nullptr;

		FTransform SpawnTransform = *Context.SpawnTransform;
		SpawnTransform.AddToTranslation(Location);
		SpawnTransform.ConcatenateRotation(Rotation.Quaternion());
		SpawnTransform.MultiplyScale3D(Scale);

		if (Context.bUseEmitterActorForSpawn)
		{
			FXSystemComponent =
				SpawnFXAttachedAsActor(Context, AttachToComponent, SocketName, SpawnTransform, AttachType);
		}
		else
		{
			FXSystemComponent = SpawnFXAttachedAsComponent(
				Context,
				AttachToComponent,
				SocketName,
				SpawnTransform,
				AttachType,
				PoolMethod,
				bAutoActivate,
				bAutoDestroy);
		}

		FFXSpawnLibraryLocal::UpdateAttachedEffectsAfterSpawn(
			FXSystemComponent,
			FXParams,
			ControlComponent,
			AttachToComponent,
			SocketName,
			Context.EffectPriority);

		return FXSystemComponent;
	}

	FFXBeamContext SpawnFXBeam(
		const FFXBeamData& Config,
		UWorld* World,
		AActor* Instigator,
		const FVector& SourceLocation,
		const FVector& TargetLocation,
		EPSCPoolMethod PoolMethod,
		bool bAutoActivate,
		bool bAutoDestroy)
	{
		check(Instigator);

		UFXSystemComponent* FXBeam = FFXSpawnLibraryLocal::SpawnFXAtLocationInternal(
			Config.FXData,
			World,
			Instigator,
			PoolMethod,
			bAutoActivate,
			bAutoDestroy,
			SourceLocation);

		FFXBeamContext FXBeamContext {};
		FXBeamContext.FXComponent = FXBeam;
		FXBeamContext.SourcePoint.PointBeamParameterName = Config.BeamSourceParameterName;
		FXBeamContext.SourcePoint.PointLocation = SourceLocation;
		FXBeamContext.TargetPoint.PointBeamParameterName = Config.BeamTargetParameterName;
		FXBeamContext.TargetPoint.PointLocation = TargetLocation;

		return FXBeamContext;
	}
}

FFXComponentContainer FFXSpawnLibrary::SpawnFXAttachedToActor(
	const FFXEffectDataAttached& Config,
	UWorld* World,
	const AActor* Instigator,
	AActor* AttachToActor,
	EPSCPoolMethod PoolMethod,
	EAttachLocation::Type AttachType,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	bool bAutoActivate,
	bool bAutoDestroy,
	UFXControlComponent* VFXControlComponent)
{
	TArray<USceneComponent*> AttachToComponents = FFXSpawnLibrary::CollectSceneComponentsForAttach(AttachToActor);

	check(AttachToComponents.Num() != 0);

	FFXComponentContainer Result(AttachToComponents.Num());

	for (const auto& AttachToComponent : AttachToComponents)
	{
		if (IsValid(AttachToComponent))
		{
			FFXComponentContainer TempFXSystemComponents = FFXSpawnLibrary::SpawnFXAttachedToComponent(
				Config,
				World,
				Instigator,
				AttachToComponent,
				PoolMethod,
				AttachType,
				Location,
				Rotation,
				Scale,
				bAutoActivate,
				bAutoDestroy,
				VFXControlComponent);
			Result.Append(TempFXSystemComponents);
		}
	}

	return Result;
}

FFXComponentContainer FFXSpawnLibrary::SpawnFXAttachedToComponent(
	const FFXEffectDataAttached& Config,
	UWorld* World,
	const AActor* Instigator,
	USceneComponent* AttachToComponent,
	EPSCPoolMethod PoolMethod,
	EAttachLocation::Type AttachType,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	bool bAutoActivate,
	bool bAutoDestroy,
	UFXControlComponent* FXControlComponent)
{
	check(AttachToComponent);
	check(World);

	//Here you can use your relathionship methods
	const bool bAlly = false;
	UFXSystemAsset* FXSystem = bAlly ? Config.FXData.FXAlly : Config.FXData.FXEnemy;

	const FFXSpawnLibraryLocal::FFXSpawnContext SpawnContext { FXSystem, World, AttachToComponent->GetOwner(), Config };

	FFXComponentContainer Result;

	UFXSystemComponent* FXSystemComponent = FFXSpawnLibraryLocal::CreateFXComponentAttached(
		SpawnContext,
		Config.FXData.FXParametersData,
		FXControlComponent,
		AttachToComponent,
		NAME_None,
		Location,
		Rotation,
		Scale,
		AttachType,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy);
	if (FXSystemComponent)
	{
		Result.Add(FXSystemComponent);
	}

	return Result;
}

FFXComponentContainer FFXSpawnLibrary::SpawnFXAttachedToNoneSocket(
	const FFXEffectDataAttached& Config,
	UWorld* World,
	const AActor* Instigator,
	USceneComponent* AttachToComponent,
	EPSCPoolMethod PoolMethod,
	EAttachLocation::Type AttachType,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	bool bAutoActivate,
	bool bAutoDestroy,
	UFXControlComponent* FXControlComponent)
{
	check(AttachToComponent);
	check(World);

	//Here you can use your relathionship methods
	const bool bAlly = false;
	UFXSystemAsset* FXSystem = bAlly ? Config.FXData.FXAlly : Config.FXData.FXEnemy;

	const FFXSpawnLibraryLocal::FFXSpawnContext SpawnContext { FXSystem, World, AttachToComponent->GetOwner(), Config };

	UFXSystemComponent* FXSystemComponent = FFXSpawnLibraryLocal::CreateFXComponentAttached(
		SpawnContext,
		Config.FXData.FXParametersData,
		FXControlComponent,
		AttachToComponent,
		NAME_None,
		Location,
		Rotation,
		Scale,
		AttachType,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy);

	FFXComponentContainer Result(FXSystemComponent);
	return Result;
}

UFXSystemComponent* FFXSpawnLibrary::SpawnFXAttachedToNoneSocket(
	const FFXEffectDataSimple& Config,
	UWorld* World,
	USceneComponent* AttachToComponent,
	EPSCPoolMethod PoolMethod,
	EAttachLocation::Type AttachType,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	bool bAutoActivate,
	bool bAutoDestroy,
	UFXControlComponent* FXControlComponent)
{
	const FFXSpawnLibraryLocal::FFXSpawnContext SpawnContext {
		Config.FXSystem,
		World,
		AttachToComponent->GetOwner(),
		&Config.FXOffset
	};
	return FFXSpawnLibraryLocal::CreateFXComponentAttached(
		SpawnContext,
		{},
		FXControlComponent,
		AttachToComponent,
		NAME_None,
		Location,
		Rotation,
		Scale,
		AttachType,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy);
}

UFXSystemComponent* FFXSpawnLibrary::SpawnFXAttachedToNoneSocket(
	UFXSystemAsset* FXSystemAsset,
	UWorld* World,
	USceneComponent* AttachToComponent,
	EPSCPoolMethod PoolMethod,
	EAttachLocation::Type AttachType,
	const FVector& Location,
	const FRotator& Rotation,
	const FVector& Scale,
	bool bAutoActivate,
	bool bAutoDestroy,
	UFXControlComponent* FXControlComponent)
{
	const FFXSpawnLibraryLocal::FFXSpawnContext SpawnContext {
		FXSystemAsset,
		World,
		AttachToComponent->GetOwner(),
		&FTransform::Identity
	};
	return FFXSpawnLibraryLocal::CreateFXComponentAttached(
		SpawnContext,
		{},
		FXControlComponent,
		AttachToComponent,
		NAME_None,
		Location,
		Rotation,
		Scale,
		AttachType,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy);
}


FFXComponentContainer FFXSpawnLibrary::SpawnFXContainerAtLocation(
	const FFXEffectData& Config,
	UWorld* World,
	AActor* Instigator,
	EPSCPoolMethod PoolMethod,
	const FVector& SpawnLocation,
	const FRotator& Rotation,
	const FVector& Scale,
	bool bAutoActivate,
	bool bAutoDestroy)
{
	UFXSystemComponent* FXComponent = FFXSpawnLibraryLocal::SpawnFXAtLocationInternal(
		Config,
		World,
		Instigator,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy,
		SpawnLocation,
		Rotation,
		Scale);

	FFXComponentContainer Result(FXComponent);

	return Result;
}

UFXSystemComponent* FFXSpawnLibrary::SpawnFXAtLocation(
	const FFXEffectData& Config,
	UWorld* World,
	AActor* Instigator,
	EPSCPoolMethod PoolMethod,
	const FVector& SpawnLocation,
	const FRotator& Rotation,
	const FVector& Scale,
	bool bAutoActivate,
	bool bAutoDestroy)
{
	return FFXSpawnLibraryLocal::SpawnFXAtLocationInternal(
		Config,
		World,
		Instigator,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy,
		SpawnLocation,
		Rotation,
		Scale);
}

UFXSystemComponent* FFXSpawnLibrary::SpawnFXAtLocation(
	const FFXEffectDataSimple& Config,
	UWorld* World,
	EPSCPoolMethod PoolMethod,
	const FVector& SpawnLocation,
	const FRotator& Rotation,
	const FVector& Scale,
	bool bAutoActivate,
	bool bAutoDestroy)
{
	return FFXSpawnLibraryLocal::CreateFXComponentAtLocation(
		Config.FXSystem,
		World,
		PoolMethod,
		SpawnLocation,
		Rotation,
		Scale,
		Config.FXOffset,
		nullptr,
		bAutoActivate,
		bAutoDestroy);
}

FFXBeamContainer FFXSpawnLibrary::SpawnFXBeam(
	const FFXBeamData& Config,
	UWorld* World,
	AActor* Instigator,
	AActor* SourceActor,
	AActor* TargetActor,
	EPSCPoolMethod PoolMethod,
	bool bAutoActivate,
	bool bAutoDestroy)
{
	check(TargetActor);
	check(SourceActor);

	TArray<USceneComponent*> SourceComponents = FFXSpawnLibrary::CollectSceneComponentsForAttach(SourceActor);
	TArray<USceneComponent*> TargetComponents = FFXSpawnLibrary::CollectSceneComponentsForAttach(TargetActor);

	const uint32 SourceCompNum = SourceComponents.Num();
	const uint32 TargetCompNum = TargetComponents.Num();
	check(SourceCompNum != 0);
	check(TargetCompNum != 0);

	const uint32 MaxCompNum = FMath::Max(SourceCompNum, TargetCompNum);


	const uint32 MaxSocketsNum = 1;

	FFXBeamContainer Result {};
	for (uint32 i = 0; i < MaxCompNum; i++)
	{
		for (uint32 j = 0; j < MaxSocketsNum; j++)
		{

			FFXBeamContext FXBeamContext = FFXSpawnLibraryLocal::SpawnFXBeam(
				Config,
				World,
				Instigator,
				SourceActor->GetActorLocation(),
				TargetActor->GetActorLocation(),
				PoolMethod,
				bAutoActivate,
				bAutoDestroy);

			FXBeamContext.SourcePoint.PointComponent = SourceComponents[i % SourceCompNum];
			FXBeamContext.SourcePoint.PointSocketName = Config.BeamSourceSocketName;
			FXBeamContext.TargetPoint.PointComponent = TargetComponents[i % TargetCompNum];
			FXBeamContext.TargetPoint.PointSocketName = Config.BeamTargetSocketName;

			FFXSpawnLibrary::UpdateFXBeam(FXBeamContext);

			Result.Add(FXBeamContext);
		}
	}

	return Result;
}

FFXBeamContainer FFXSpawnLibrary::SpawnFXBeam(
	const FFXBeamData& Config,
	UWorld* World,
	AActor* Instigator,
	AActor* SourceActor,
	const FVector& TargetLocation,
	EPSCPoolMethod PoolMethod,
	bool bAutoActivate,
	bool bAutoDestroy)
{
	check(SourceActor);

	TArray<USceneComponent*> SourceComponents = FFXSpawnLibrary::CollectSceneComponentsForAttach(SourceActor);

	const uint32 SourceCompNum = SourceComponents.Num();
	check(SourceCompNum != 0);

	FFXBeamContainer Result {};

	FFXBeamContext FXBeamContext = FFXSpawnLibraryLocal::SpawnFXBeam(
		Config,
		World,
		Instigator,
		SourceActor->GetActorLocation(),
		TargetLocation,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy);

	FXBeamContext.SourcePoint.PointComponent = SourceComponents[0];
	FXBeamContext.SourcePoint.PointSocketName = Config.BeamSourceSocketName;

	FFXSpawnLibrary::UpdateFXBeam(FXBeamContext);

	Result.Add(FXBeamContext);

	return Result;
}

FFXBeamContainer FFXSpawnLibrary::SpawnFXBeam(
	const FFXBeamData& Config,
	UWorld* World,
	AActor* Instigator,
	USceneComponent* SourceComponent,
	const FVector& TargetLocation,
	EPSCPoolMethod PoolMethod,
	bool bAutoActivate,
	bool bAutoDestroy)
{
	check(SourceComponent);

	FFXBeamContainer Result {};

	FFXBeamContext FXBeamContext = FFXSpawnLibraryLocal::SpawnFXBeam(
		Config,
		World,
		Instigator,
		SourceComponent->GetComponentLocation(),
		TargetLocation,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy);

	FXBeamContext.SourcePoint.PointComponent = SourceComponent;
	FXBeamContext.SourcePoint.PointSocketName = Config.BeamSourceSocketName;

	FFXSpawnLibrary::UpdateFXBeam(FXBeamContext);

	Result.Add(FXBeamContext);

	return Result;
}

FFXBeamContainer FFXSpawnLibrary::SpawnFXBeam(
	const FFXBeamData& Config,
	UWorld* World,
	AActor* Instigator,
	const FVector& SourceLocation,
	const FVector& TargetLocation,
	EPSCPoolMethod PoolMethod,
	bool bAutoActivate,
	bool bAutoDestroy)
{
	check(Instigator);

	const FFXBeamContext FXBeamContext = FFXSpawnLibraryLocal::SpawnFXBeam(
		Config,
		World,
		Instigator,
		SourceLocation,
		TargetLocation,
		PoolMethod,
		bAutoActivate,
		bAutoDestroy);

	FFXSpawnLibrary::UpdateFXBeam(FXBeamContext);

	return FFXBeamContainer(FXBeamContext);
}

void FFXSpawnLibrary::UpdateFXLocation(

	TArray<UFXSystemComponent*>& InUpdatingSystems,
	const FVector& InNewLocation,
	bool bRelative)
{
	for (auto& FXSystem : InUpdatingSystems)
	{
		if (IsValid(FXSystem))
		{
			const FVector CurrentLocation =
				bRelative ? FXSystem->GetRelativeLocation() : FXSystem->GetComponentLocation();
			if (InNewLocation != CurrentLocation)
			{
				if (bRelative)
				{
					FXSystem->SetRelativeLocation(InNewLocation, false, nullptr, ETeleportType::TeleportPhysics);
				}
				else
				{
					FXSystem->SetWorldLocation(InNewLocation, false, nullptr, ETeleportType::TeleportPhysics);
				}
			}
		}
	}
}

void FFXSpawnLibrary::UpdateFXBeam(const FFXBeamContext& BeamContext)
{
	if (BeamContext.IsValid())
	{
		const FVector SourceLocation = BeamContext.SourcePoint.GetUpdatedLocation();
		UpdateFXVectorParameter(
			BeamContext.FXComponent,
			{ BeamContext.SourcePoint.PointBeamParameterName, SourceLocation });

		const FVector TargetLocation = BeamContext.TargetPoint.GetUpdatedLocation();
		UpdateFXVectorParameter(
			BeamContext.FXComponent,
			{ BeamContext.TargetPoint.PointBeamParameterName, TargetLocation });
	}
}

TArray<USceneComponent*> FFXSpawnLibrary::CollectSceneComponentsForAttach(AActor* AttachToActor)
{
	TArray<USceneComponent*> SceneComponents;
	check(AttachToActor);

	SceneComponents.Add(AttachToActor->GetRootComponent());

	return SceneComponents;
}
