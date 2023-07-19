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

#include "CoreMinimal.h"
#include "Particles/Emitter.h"
#include "VSPEmitter.generated.h"

// In this class, we have replaced the native particle system component with VSPParticleSystemComponent
UCLASS()
class AVSPEmitter : public AEmitter
{
	GENERATED_BODY()

public:
	AVSPEmitter();

	// Actor will be attached to InAttachToComponent and will be destroyed, when ParentActor gets destroyed
	void AttachToComponentFollowDestroy(
		AActor* ParentActor,
		USceneComponent* InAttachToComponent,
		const FAttachmentTransformRules& InAttachmentTransformRules,
		const FName& InSocketName);

#if WITH_EDITORONLY_DATA

	UPROPERTY()
	UBillboardComponent* BillboardComponent;
#endif

private:
	UFUNCTION()
	void ParentActorDestroyed(AActor* DestroyedActor);
};
