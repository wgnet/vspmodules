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
#include "TerraformerBrushBase.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTerraformerBrush, Log, All);

class ATerraformerBrushManager;

struct FTerraformerBrushRenderData
{
	UTextureRenderTarget2D*& SourceHeightRT;
	UTextureRenderTarget2D*& TargetHeightRT;
	FTransform TerrainTransform;
	FVector TerrainOrigin;
	FVector TerrainExtent;
	int32 TerrainSubsections;
	int32 TerrainQuadsSize;
};

UCLASS(HideCategories = (Rendering, Replication, Collision, Actor, Cooking, "Tags", "AssetUserData", HLOD, Input, LOD))
class TERRAFORMER_API ATerraformerBrushBase : public AActor
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Blend"))
	int32 Priority = 0;

	ATerraformerBrushBase();
	virtual void Render(FTerraformerBrushRenderData& Data);
	virtual void GetTriangulatedShape(
		FTransform InTransform,
		FIntPoint InRTSize,
		TArray<FVector>& OutVertices,
		TArray<int32>& OutIndexes) const;
	virtual void PostEditMove(bool bFinished) override;

	bool HasGroup() const;
	int32 GetGroupPriority() const;

protected:
	UPROPERTY(Transient)
	ATerraformerBrushManager* BrushManager;

	void UpdateBrushManager() const;
};
