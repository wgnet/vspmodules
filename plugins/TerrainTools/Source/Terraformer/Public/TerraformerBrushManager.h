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

#include "LandscapeBlueprintBrush.h"
#include "TerraformerBrushManager.generated.h"

class UJumpFlood;
class ALandscape;
class USplineComponent;
class ATerraformerSplineBrush;

UCLASS(hidecategories = (Collision))
class TERRAFORMER_API ATerraformerBrushManager : public ALandscapeBlueprintBrush
{
	GENERATED_BODY()
public:
	ATerraformerBrushManager();

	// Action buttons
	UFUNCTION(CallInEditor, meta = (Category = "Actions"))
	void ForceUpdate();

	UFUNCTION(CallInEditor, meta = (Category = "Actions"))
	void RecalculateNormals() const;

	UFUNCTION(CallInEditor, meta = (Category = "Actions"))
	void UpdateTargetLandscape();

	// Optimization
	UPROPERTY(EditAnywhere, meta = (Category = "Optimization"))
	bool bAutoUpdate = true;

	UPROPERTY(EditAnywhere, meta = (Category = "Optimization"))
	bool bUseSmoothShape = true;

	UPROPERTY(EditAnywhere, meta = (Category = "Optimization"))
	bool bUseSmoothEdges = true;

	// Settings
	UPROPERTY(EditInstanceOnly, NoClear, meta = (Category = "Settings"))
	TLazyObjectPtr<ALandscape> Landscape;

	UPROPERTY(EditInstanceOnly, NoClear, meta = (Category = "Settings"))
	bool bPadToPowerOfTwo = false;

	UPROPERTY(EditInstanceOnly, NoClear, meta = (Category = "Settings"))
	bool bRenderRegions = false;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	float MaxWorldHeight = 65536;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	FTransform LandscapeTransform;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	FIntPoint LandscapeRTSize;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	FIntPoint LandscapeQuads;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	FVector LandscapeOrigin;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	FVector LandscapeExtent;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	int32 LandscapeSubsections;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	int32 LandscapeQuadsSize;

	UPROPERTY(VisibleAnywhere, meta = (Category = "Settings"))
	FVector LandscapeSize;

	void RequestUpdate();

	static void UpdateBrushManager(UWorld* InWorld);

protected:
	UPROPERTY(VisibleAnywhere, Transient, meta = (Category = "Debug"))
	UTextureRenderTarget2D* SourceHeightRT = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, meta = (Category = "Debug"))
	UTextureRenderTarget2D* TargetHeightRT = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, meta = (Category = "Debug"))
	UTextureRenderTarget2D* OutputHeightRT = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, meta = (Category = "Debug"))
	UTextureRenderTarget2D* DepthAndShapeRT = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, meta = (Category = "Debug"))
	UTextureRenderTarget2D* JumpFloodRTA = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, meta = (Category = "Debug"))
	UTextureRenderTarget2D* JumpFloodRTB = nullptr;

	UPROPERTY(VisibleAnywhere, Transient, meta = (Category = "Debug"))
	UTextureRenderTarget2D* DistanceFieldCacheRT = nullptr;

protected:
	virtual void Initialize_Native(
		const FTransform& InLandscapeTransform,
		const FIntPoint& InLandscapeSize,
		const FIntPoint& InLandscapeRenderTargetSize) override;
	virtual UTextureRenderTarget2D* Render_Native(
		bool InIsHeightmap,
		UTextureRenderTarget2D* InCombinedResult,
		const FName& InWeightmapLayerName) override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	void AllocateRTs(const FIntPoint Size);
	void DrawShape(const ATerraformerSplineBrush* BrushActorRenderContext) const;
	void GenerateJumpFlood(const ATerraformerSplineBrush* BrushActorRenderContext) const;
	void CacheBrushDistanceField(const ATerraformerSplineBrush* BrushActorRenderContext) const;
	void BlendAngleFalloff(
		const ATerraformerSplineBrush* BrushActorRenderContext,
		UTextureRenderTarget2D* InSourceHeight,
		UTextureRenderTarget2D* InTargetHeight) const;
	void CopyRenderTarget(UTextureRenderTarget2D* SourceRT, UTextureRenderTarget2D* DestRT) const;
	void SetTargetLandscape(ALandscape* InTargetLandscape);
	FIntRect CalculateRenderRegion(const ATerraformerSplineBrush* BrushActorRenderContext) const;

	UTextureRenderTarget2D* GetDistanceFieldCache() const;
	UTextureRenderTarget2D* GetDepthAndShapeRT() const
	{
		return DepthAndShapeRT;
	}
	UTextureRenderTarget2D* GetJumpFloodRT(const ATerraformerSplineBrush* BrushActorRenderContext) const;

private:
	bool bNeedUpdate = false;
	static bool IsClockwise(const USplineComponent* InSpline);
	static FLinearColor PackFloatToRG8(float InValue);
	static FLinearColor PackFloatToRGB8(float InValue);
	static void GetLandscapeCenterAndExtent(
		const ALandscape* InLandscape,
		FVector& OutCenter,
		FVector& OutExtent,
		int32& OutSubsections,
		int32& OutQuadsSize);

	UTextureRenderTarget2D* PingPongSource(int32 Offset, int32 CompletedPasses) const;
	UTextureRenderTarget2D* PingPongTarget(int32 Offset, int32 CompletedPasses) const;

	FCriticalSection CriticalSection;
	FRenderCommandFence Fence;
};
