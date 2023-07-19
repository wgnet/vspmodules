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


#include "VSPNiagaraEmitter.h"
#include "NiagaraComponent.h"


// Sets default values
AVSPNiagaraEmitter::AVSPNiagaraEmitter()
{
	static FName const NiagaraComponentName = FName(TEXT("NiagaraComponent"));
	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(NiagaraComponentName);
	RootComponent = NiagaraComponent;
}

void AVSPNiagaraEmitter::AttachToComponentFollowDestroy(
	AActor* ParentActor,
	USceneComponent* InAttachToComponent,
	const FAttachmentTransformRules& InAttachmentTransformRules,
	const FName& InSocketName)
{
	AttachToComponent(InAttachToComponent, InAttachmentTransformRules, InSocketName);
	ParentActor->OnDestroyed.AddUniqueDynamic(this, &AVSPNiagaraEmitter::ParentActorDestroyed);
	Activate();
}

void AVSPNiagaraEmitter::Activate()
{
	if (NiagaraComponent)
	{
		NiagaraComponent->ActivateSystem(false);
	}
}

void AVSPNiagaraEmitter::Deactivate()
{
	if (NiagaraComponent)
	{
		NiagaraComponent->Deactivate();
	}
}

void AVSPNiagaraEmitter::SetAsset(UNiagaraSystem* InAsset)
{
	if (NiagaraComponent)
	{
		NiagaraComponent->SetAsset(InAsset);
	}
}

void AVSPNiagaraEmitter::PostInitializeComponents()
{
	Super::PostInitializeComponents();

	if (NiagaraComponent)
	{
		NiagaraComponent->OnSystemFinished.AddUniqueDynamic(this, &AVSPNiagaraEmitter::SystemFinished);
	}
}

void AVSPNiagaraEmitter::ParentActorDestroyed(AActor* DestroyedActor)
{
	Deactivate();
}

void AVSPNiagaraEmitter::SystemFinished(UNiagaraComponent* NiagaraSystem)
{
	if (bDestroyOnSystemFinish)
	{
		Destroy();
	}
}
