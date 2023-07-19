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


#include "TerraformerGroup.h"

#include "TerraformerBrushManager.h"

ATerraformerGroup::ATerraformerGroup()
{
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	RootComponent->SetAbsolute(false, true, true);
}

int32 ATerraformerGroup::GetGroupPriority() const
{
	return GroupPriority;
}

void ATerraformerGroup::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		ATerraformerBrushManager::UpdateBrushManager(GetWorld());
	}
}

void ATerraformerGroup::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	ATerraformerBrushManager::UpdateBrushManager(GetWorld());
}

void ATerraformerGroup::EditorApplyScale(
	const FVector& DeltaScale,
	const FVector* PivotLocation,
	bool bAltDown,
	bool bShiftDown,
	bool bCtrlDown)
{
	Super::EditorApplyScale(FVector::ZeroVector, PivotLocation, bAltDown, bShiftDown, bCtrlDown);
}

void ATerraformerGroup::EditorApplyRotation(
	const FRotator& DeltaRotation,
	bool bAltDown,
	bool bShiftDown,
	bool bCtrlDown)
{
	Super::EditorApplyRotation(FRotator::ZeroRotator, bAltDown, bShiftDown, bCtrlDown);
}
