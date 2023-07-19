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
#include "Components/BoxComponent.h"
#include "TerraformerBrushBase.h"
#include "TerraformerStampBrush.generated.h"

UCLASS()
class TERRAFORMER_API UStampBoxComponent : public UBoxComponent
{
	GENERATED_BODY()

public:
	UStampBoxComponent(const FObjectInitializer& ObjectInitializer);
};

UENUM()
enum class ETFStampBrushBlendMode : uint8
{
	/** Alpha Blend will affect the heightmap both upwards and downwards. */
	Alpha = 0 UMETA(DisplayName = "Alpha"),
	/** Limits the brush to only lowering the terrain. */
	Min = 1	  UMETA(DisplayName = "Min"),
	/** Limits the brush to only raising the terrain. */
	Max = 2	  UMETA(DisplayName = "Max"),
	/** Performs an additive blend, using a flat Z=0 terrain as the input. Useful when you want to preserve underlying detail or ramps. */
	Add = 3	  UMETA(DisplayName = "Additive"),
	/** Overlay Blend will affect the heightmap both upwards and downwards. */
	Ovr = 4	  UMETA(DisplayName = "Overlay")
};

UCLASS()
class TERRAFORMER_API ATerraformerStampBrush : public ATerraformerBrushBase
{
	GENERATED_BODY()

public:
	ATerraformerStampBrush();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Blend"))
	ETFStampBrushBlendMode BlendMode = ETFStampBrushBlendMode::Add;

	UPROPERTY(EditInstanceOnly, Category = "Brush")
	TSoftObjectPtr<UTexture2D> BrushTexture;

	virtual void Render(FTerraformerBrushRenderData& Data) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void OnConstruction(const FTransform& Transform) override;
	virtual void EditorApplyRotation(const FRotator& DeltaRotation, bool bAltDown, bool bShiftDown, bool bCtrlDown)
		override;
	virtual void EditorApplyScale(
		const FVector& DeltaScale,
		const FVector* PivotLocation,
		bool bAltDown,
		bool bShiftDown,
		bool bCtrlDown) override;

private:
	/** Box for visualizing surface extents. */
	UPROPERTY(Transient)
	UStampBoxComponent* Box = nullptr;

	UPROPERTY(Transient)
	TSoftObjectPtr<UTexture2D> DefaultTexture;
};
