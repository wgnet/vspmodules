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
#include "LandscapeProxy.h"
#include "TerrainMaterialProxyData.h"
#include "TerrainMaterialProxy.generated.h"


UCLASS(NotBlueprintable)
class ATerrainMaterialProxy : public AActor
{
	GENERATED_BODY()

public:
	ATerrainMaterialProxy();

	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void FillData(ULandscapeComponent* InLandscapeComponent);
	void ApplyData();

	UPROPERTY(VisibleAnywhere, Category = Options)
	TMap<FName, FTerrainMaterialInfo> MaterialsInfo;

	UPROPERTY(EditInstanceOnly)
	TSoftObjectPtr<ULandscapeComponent> LandscapeComponent;

#if WITH_EDITORONLY_DATA

	UFUNCTION(CallInEditor, Category = Layers)
	void Refresh();

	UFUNCTION(CallInEditor, Category = Layers)
	void ResetIDs();

	UFUNCTION(CallInEditor, Category = Layers)
	static TArray<ULandscapeComponent*> GetLandscapeComponents();

	UPROPERTY(EditDefaultsOnly)
	UBillboardComponent* Sprite;

#endif
};
