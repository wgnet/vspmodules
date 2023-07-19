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
#include "TerraformerBrushBase.h"

#include "TerraformerSplineBrush.generated.h"


class USplineComponent;
class UProceduralMeshComponent;
class ATerraformerBrushManager;
class UTerraformerSplineComponent;
class UTextureRenderTarget2D;
class UBoxComponent;

UENUM()
enum class ETerraformerBrushType : uint8
{
	Area = 0  UMETA(DisplayName = "Area"),
	Path = 1  UMETA(DisplayName = "Path"),
	Debug = 2 UMETA(DisplayName = "Debug")
};

UENUM()
enum class ETerraformerBrushBlendMode : uint8
{
	/** Alpha Blend will affect the heightmap both upwards and downwards. */
	Alpha = 0 UMETA(DisplayName = "Alpha"),
	/** Limits the brush to only lowering the terrain. */
	Min = 1	  UMETA(DisplayName = "Min"),
	/** Limits the brush to only raising the terrain. */
	Max = 2	  UMETA(DisplayName = "Max"),
	/** Performs an additive blend, using a flat Z=0 terrain as the input. Useful when you want to preserve underlying detail or ramps. */
	Add = 3	  UMETA(DisplayName = "Additive"),
	/** Debug */
	Deb = 4	  UMETA(DisplayName = "Debug")
};

UCLASS(ConversionRoot, ComponentWrapperClass)
class TERRAFORMER_API ATerraformerSplineBrush : public ATerraformerBrushBase
{
	GENERATED_BODY()
public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Blend"))
	ETerraformerBrushBlendMode BlendMode = ETerraformerBrushBlendMode::Alpha;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Category = "Blend"))
	ETerraformerBrushType BrushType = ETerraformerBrushType::Area;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Distance Field", ClampMin = "100"))
	float SegmentSize = 256.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Distance Field", ClampMin = "0", ClampMax = "8"))
	int32 SmoothShape = 0;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Distance Field", ClampMin = "0", ClampMax = "8"))
	int32 SmoothEdges = 0;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Distance Field"))
	float HeightOffset = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, meta = (Category = "Distance Field"))
	bool InvertShape = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Falloff Settings"))
	bool bCapShape = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Falloff Settings"))
	bool bClipShape = false;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Falloff Settings", ClampMax = "89"))
	float FalloffAngle = 45.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Falloff Settings"))
	int32 FalloffWidth = 0;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Falloff Settings"))
	float EdgeWidthOffset = 0.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	float InnerSmoothThreshold = 0.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	float OuterSmoothThreshold = 0.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	float TerraceAlpha = 0.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	float TerraceSmoothness = 0.1f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	float TerraceSpacing = 4096.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	float TerraceMaskLength = 4096.f;

	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	float MaskStartOffset = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	float CurveChannelDepth = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, meta = (Category = "Smoothing and Terracing"))
	bool UseCurveChannel = false;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, meta = (Category = "Displacement Settings"))
	float DisplacementHeight = 0.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, meta = (Category = "Displacement Settings"))
	float DisplacementMidpoint = 0.5f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, meta = (Category = "Displacement Settings"))
	float DisplacementTiling = 2.f;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, meta = (Category = "Displacement Settings"))
	FLinearColor DisplacementChannel = { 1, 0, 0, 0 };

	UPROPERTY(VisibleInstanceOnly, BlueprintReadWrite, meta = (Category = "Displacement Settings"))
	TSoftObjectPtr<UTexture2D> DisplacementTexture;

	ATerraformerSplineBrush();
	USplineComponent* GetSplineComponent() const;
	virtual void Render(FTerraformerBrushRenderData& Data) override;
	virtual void GetTriangulatedShape(
		FTransform InTransform,
		FIntPoint InRTSize,
		TArray<FVector>& OutVertices,
		TArray<int32>& OutIndexes) const override;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, meta = (AllowPrivateAccess = "true"))
	UTerraformerSplineComponent* SplineComp;

	UPROPERTY()
	UBoxComponent* Box;

	void TriangulateAreaShape(TArray<FVector>& OutVertices, TArray<int32>& OutIndexes) const;
	void TriangulatePathShape(TArray<FVector>& OutVertices, TArray<int32>& OutIndexes) const;
	bool DetectDegenerateTriangles(
		const TArray<FVector>& Vertices,
		const TArray<int32>& Triangles,
		TArray<int32>& OutNonDegenerateTriangles) const;

	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;
	virtual void OnConstruction(const FTransform& Transform) override;
};
