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

#include "GameFramework/Actor.h"
#include "TerraformerGroup.generated.h"

UCLASS(HideCategories = (Rendering, Replication, Collision, Actor, Cooking, "Tags", "AssetUserData", HLOD, Input, LOD))
class TERRAFORMER_API ATerraformerGroup : public AActor
{
	GENERATED_BODY()

public:
	ATerraformerGroup();

	int32 GetGroupPriority() const;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Group"))
	int32 GroupPriority = 0;

	virtual void PostEditMove(bool bFinished) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void EditorApplyScale(
		const FVector& DeltaScale,
		const FVector* PivotLocation,
		bool bAltDown,
		bool bShiftDown,
		bool bCtrlDown) override;
	virtual void EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
		override;
};
