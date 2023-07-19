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
#include "EdMode.h"

class CalculatingVerticesTask;

namespace FSTColorNames
{
	static const FName VertexColorName = "VertexColor";
	static const FName LineColorForSnappingName = "LineColorForSnapping";
}

struct FSTSettings
{
	FLinearColor VertexColor { 0, 0.4, 0 };
	FLinearColor LineColorForSnapping { FLinearColor::Yellow };
	float VertexSize { 13.f };
	float LineForSnappingThickness { 2.0f };

	FLinearColor& GetColorReferenceByName(FName InName);
};

struct HSHVertexHitProxy : public HHitProxy
{
	DECLARE_HIT_PROXY()

	HSHVertexHitProxy(FVector InPosition, AActor* InActor) : HHitProxy(HPP_UI), RefVector(InPosition), RefActor(InActor)
	{
	}

	FVector RefVector;
	AActor* RefActor { nullptr };
};

struct FCurrentSelection
{
	FVector SelectedVertex { FVector::ZeroVector };
	TArray<AActor*> SelectedActors;

	FVector GetDefaultPivotLocation() const;
	bool IsSomeoneSelected() const;
	void UpdateSelection();
	void ResetPivotPointForAllSelectedActors();
	static void ResetPivotPoint(AActor* InActorForReset);
	void ResetSelectedVertex();
	static void SetTemporaryPivotPoint(AActor* InActor, FVector InPivotLocation);
};

struct FActorWithVertices
{
	AActor* Actor { nullptr };
	TArray<FVector> VerticesForDraw;
	TArray<FVector> VerticesForHitProxy;
};

class FSTEdMode : public FEdMode
{
public:
	const static FEditorModeID EM_STEdModeId;

	FSTEdMode()
	{
		LoadVertexIconTexture();
	}

	// FEdMode interface
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual void DrawHUD(
		FEditorViewportClient* ViewportClient,
		FViewport* Viewport,
		const FSceneView* View,
		FCanvas* Canvas) override;
	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
		override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void ActorSelectionChangeNotify() override;
	virtual bool InputDelta(
		FEditorViewportClient* InViewportClient,
		FViewport* InViewport,
		FVector& InDrag,
		FRotator& InRot,
		FVector& InScale) override;
	virtual bool ShouldDrawWidget() const override;
	virtual FVector GetWidgetLocation() const override;
	// End of FEdMode interface

	static FVector GetActorArrayAverageLocation(const TArray<AActor*>& Actors);
	static float FindScaleFromDistanceToSprite(const FSceneView* InView, FVector CurrentVertexForDraw);
	static bool IsNeedTraceCheck();
	static bool IsKeysForSnappingPressed();
	static bool IsKeysForChangePivotPressed();
	void GetActorsWithComponentsInView(
		TMap<AActor*, TSet<UActorComponent*>>& OutActorsInView,
		TSet<TWeakObjectPtr<UPrimitiveComponent>>& OutHiddenComponents) const;
	static FVector2D GetMouseVector2D();
	void LoadVertexIconTexture();
	void RunAsyncTask();

	//	Ignoring the vector along one of the axes, depending on the type of orthogonal projection
	static FVector GetLocationWithoutDepthOffset(
		FEditorViewportClient* InViewportClient,
		FVector InCurrentLocation,
		FVector InDesiredLocation);

	bool bIsAsyncTaskInWork { false };
	bool bIs2DSnapping { true };
	const FSceneView* EdModeView { nullptr };
	UTexture2D* VertexIcon { nullptr };
	FSTSettings SmartTransformSettings;
	TArray<FActorWithVertices> FreshActorsWithVertices;
	TArray<FActorWithVertices> PreviousActorsWithVertices;
	FTimerHandle AsyncCalculationTaskTimer;
	FCurrentSelection CurrentSelection;
	bool bIsMouseMove;
	FVector2D MouseOnScreenPosition;
};

struct FInfoForCalculate
{
	FSTEdMode* EdMode { nullptr };
	FVector2D MouseOnScreen;

	// All actors and components on the screen
	TMap<AActor*, TSet<UActorComponent*>> ActorWithComponents;
	TSet<TWeakObjectPtr<UPrimitiveComponent>> HiddenComponents;
	bool bIsNeedTraceCheck;
};

class CalculatingVerticesTask : public FNonAbandonableTask
{
public:
	CalculatingVerticesTask(FInfoForCalculate InInfo) : InfoForCalculate(InInfo)
	{
	}

	FInfoForCalculate InfoForCalculate;

	~CalculatingVerticesTask();

	// Just need for work )
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(CalculatingVerticesTask, STATGROUP_ThreadPoolAsyncTasks)
	}

	void DoWork();
};