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

#include "VSPEmitter.h"

#if WITH_EDITOR
	#include "Components/ArrowComponent.h"
	#include "Components/BillboardComponent.h"
	#include "Misc/MapErrors.h"
#endif

#include "Particles/ParticleSystemComponent.h"

AVSPEmitter::AVSPEmitter()
{
	bDestroyOnSystemFinish = true;

#if WITH_EDITOR
	BillboardComponent = CreateDefaultSubobject<UBillboardComponent>(TEXT("BillboardComponent"));
	BillboardComponent->SetupAttachment(RootComponent);

	// Remove native components

	if (GetArrowComponent())
	{
		GetArrowComponent()->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		GetArrowComponent()->DestroyComponent();
	}

	if (GetSpriteComponent())
	{
		GetSpriteComponent()->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
		GetSpriteComponent()->DestroyComponent();
	}
#endif
}

void AVSPEmitter::AttachToComponentFollowDestroy(
	AActor* ParentActor,
	USceneComponent* InAttachToComponent,
	const FAttachmentTransformRules& InAttachmentTransformRules,
	const FName& InSocketName)
{
	AttachToComponent(InAttachToComponent, InAttachmentTransformRules, InSocketName);
	ParentActor->OnDestroyed.AddUniqueDynamic(this, &AVSPEmitter::ParentActorDestroyed);
	Activate();
}

void AVSPEmitter::ParentActorDestroyed(AActor* DestroyedActor)
{
	Deactivate();
}
