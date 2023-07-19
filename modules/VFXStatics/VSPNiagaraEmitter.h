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
#include "GameFramework/Actor.h"
#include "VSPNiagaraEmitter.generated.h"

class UNiagaraSystem;
class UNiagaraComponent;

UCLASS()
class AVSPNiagaraEmitter : public AActor
{
	GENERATED_BODY()

public:
	bool bDestroyOnSystemFinish = true;

	AVSPNiagaraEmitter();

	void AttachToComponentFollowDestroy(
		AActor* ParentActor,
		USceneComponent* InAttachToComponent,
		const FAttachmentTransformRules& InAttachmentTransformRules,
		const FName& InSocketName);

	void Activate();
	void Deactivate();
	void SetAsset(UNiagaraSystem* InAsset);

	virtual void PostInitializeComponents() override;

	UPROPERTY(EditDefaultsOnly)
	UNiagaraComponent* NiagaraComponent;

private:
	UFUNCTION()
	void ParentActorDestroyed(AActor* DestroyedActor);

	UFUNCTION()
	void SystemFinished(UNiagaraComponent* NiagaraSystem);
};
