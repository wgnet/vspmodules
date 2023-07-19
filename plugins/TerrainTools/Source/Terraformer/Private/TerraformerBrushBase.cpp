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

#include "TerraformerBrushBase.h"

#include "EngineUtils.h"
#include "TerraformerBrushManager.h"
#include "TerraformerGroup.h"

DEFINE_LOG_CATEGORY(LogTerraformerBrush);

ATerraformerBrushBase::ATerraformerBrushBase()
{
}

void ATerraformerBrushBase::Render(FTerraformerBrushRenderData& Data)
{
}

void ATerraformerBrushBase::GetTriangulatedShape(
	FTransform InTransform,
	FIntPoint InRTSize,
	TArray<FVector>& OutVertices,
	TArray<int32>& OutIndexes) const
{
	OutVertices = TArray<FVector> {};
	OutIndexes = TArray<int32> {};
}

void ATerraformerBrushBase::PostEditMove(bool bFinished)
{
	Super::PostEditMove(bFinished);

	if (bFinished)
	{
		UpdateBrushManager();
	}
}

bool ATerraformerBrushBase::HasGroup() const
{
	if (const AActor* ParentActor = GetAttachParentActor())
	{
		return ParentActor->IsA(ATerraformerGroup::StaticClass());
	}
	return false;
}

int32 ATerraformerBrushBase::GetGroupPriority() const
{
	if (const AActor* ParentActor = GetAttachParentActor())
	{
		if (const ATerraformerGroup* TerraformerGroup = Cast<ATerraformerGroup>(ParentActor))
		{
			return TerraformerGroup->GetGroupPriority();
		}
	}
	return 0;
}

void ATerraformerBrushBase::UpdateBrushManager() const
{
	ATerraformerBrushManager::UpdateBrushManager(GetWorld());
}
