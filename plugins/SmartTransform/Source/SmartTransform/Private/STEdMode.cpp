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
#include "STEdMode.h"
#include "STVertexIterators.h"

#include "ImageUtils.h"
#include "Interfaces/IPluginManager.h"
#include "UnrealEd.h"

#define LOCTEXT_NAMESPACE "FSTEdMode"


namespace FSTEdModeLocal
{
	float DPIScale = 1.f;
	const int32 MaxActorsInViewport = 4000;
	const int32 MaxVerticesForDraw = 3000;
	const FString SmartTransformName = "SmartTransform";
	const FString VertexIconPath = "/VertexIcon.png";
	float RadiusDisplayVertices = 200.f;
	float HitProxyGenerationRadius = 5.f;
}

const FEditorModeID FSTEdMode::EM_STEdModeId = TEXT("EM_STEdModeId");

FLinearColor& FSTSettings::GetColorReferenceByName(FName InName)
{
	if (InName == FSTColorNames::VertexColorName)
		return VertexColor;

	if (InName == FSTColorNames::LineColorForSnappingName)
		return LineColorForSnapping;

	return VertexColor;
}

IMPLEMENT_HIT_PROXY(HSHVertexHitProxy, HHitProxy)

void FSTEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	// TODO: If Add on screen debug message, then everything is displayed correctly. Without this, it sometimes starts to flicker. And also it is not displayed in the absence of light.
	GEditor->AddOnScreenDebugMessage(13, 0, FColor::Red, "Refresh", true, FVector2D(0.f));

	if (PreviousActorsWithVertices.Num())
	{
		for (auto Element : PreviousActorsWithVertices)
		{
			for (const auto CurrentVertexForHitProxy : Element.VerticesForHitProxy)
			{
				PDI->SetHitProxy(new HSHVertexHitProxy(CurrentVertexForHitProxy, Element.Actor));
			}
			for (auto CurrentVertexForDraw : Element.VerticesForDraw)
			{
				const float Scale = FindScaleFromDistanceToSprite(View, CurrentVertexForDraw);

				PDI->DrawSprite(
					CurrentVertexForDraw,
					SmartTransformSettings.VertexSize * Scale,
					SmartTransformSettings.VertexSize * Scale,
					VertexIcon->Resource,
					SmartTransformSettings.VertexColor,
					SDPG_MAX,
					0.0,
					0.0,
					0.0,
					0.0);
			}
		}
	}

	if (bIsMouseMove)
		Viewport->DeferInvalidateHitProxy();

	EdModeView = View;
	FEdMode::Render(View, Viewport, PDI);
}

void FSTEdMode::DrawHUD(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	const FSceneView* View,
	FCanvas* Canvas)
{
	// Draw snapping line
	if (IsKeysForSnappingPressed() && CurrentSelection.IsSomeoneSelected()
		&& CurrentSelection.SelectedVertex != FVector::ZeroVector)
	{
		FVector2D SelectedVertexOnScreen;
		View->WorldToPixel(CurrentSelection.SelectedVertex, SelectedVertexOnScreen);

		FCanvasLineItem LineItem;
		LineItem.LineThickness = SmartTransformSettings.LineForSnappingThickness;
		LineItem.SetColor(SmartTransformSettings.LineColorForSnapping);
		LineItem.Draw(Canvas, MouseOnScreenPosition, SelectedVertexOnScreen / FSTEdModeLocal::DPIScale);
	}

	FEdMode::DrawHUD(ViewportClient, Viewport, View, Canvas);
}

bool FSTEdMode::HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
{
	bool bIsHandled = false;

	//	Move to vertex
	if (HitProxy)
	{
		const FModifierKeysState ModifierKeysState = FSlateApplication::Get().GetModifierKeys();
		const bool bIsControlOrShiftDown = ModifierKeysState.IsControlDown() || ModifierKeysState.IsShiftDown();

		if (HitProxy && Click.GetKey() == EKeys::LeftMouseButton && IsKeysForSnappingPressed()
			&& CurrentSelection.IsSomeoneSelected())
		{
			bIsHandled = true;
			if (HitProxy->IsA(HSHVertexHitProxy::StaticGetType()))
			{
				GEditor->BeginTransaction(LOCTEXT("SnapToVertex", "Snap to vertex"));

				const auto VertexHitProxy = static_cast<HSHVertexHitProxy*>(HitProxy);

				for (auto Element : CurrentSelection.SelectedActors)
				{
					Element->Modify();
					FVector Offset = Element->GetActorLocation() - CurrentSelection.SelectedVertex;
					const FVector DesiredLocation = VertexHitProxy->RefVector + Offset;
					FVector DesiredLocation2DSnap =
						GetLocationWithoutDepthOffset(InViewportClient, Element->GetActorLocation(), DesiredLocation);

					const FVector& Location = bIs2DSnapping ? DesiredLocation2DSnap : DesiredLocation;
					Element->SetActorLocation(Location);
				}

				const FVector Location2DSnapSelectedVertex = GetLocationWithoutDepthOffset(
					InViewportClient,
					CurrentSelection.SelectedVertex,
					VertexHitProxy->RefVector);

				CurrentSelection.SelectedVertex =
					bIs2DSnapping ? Location2DSnapSelectedVertex : VertexHitProxy->RefVector;
				GUnrealEd->UpdatePivotLocationForSelection(true);

				// We're done moving actors so close transaction
				GEditor->EndTransaction();
			}
		}
		// Select vertex, actor and update pivot for selected actors
		else if (HitProxy->IsA(HSHVertexHitProxy::StaticGetType()) && !bIsControlOrShiftDown)
		{
			bIsHandled = true;
			const auto VertexHitProxy = static_cast<HSHVertexHitProxy*>(HitProxy);
			CurrentSelection.SelectedVertex = VertexHitProxy->RefVector;

			if (!CurrentSelection.IsSomeoneSelected() && VertexHitProxy->RefActor)
			{
				GEditor->SelectActor(VertexHitProxy->RefActor, true, true, false, true);
				CurrentSelection.SetTemporaryPivotPoint(VertexHitProxy->RefActor, GetWidgetLocation());
			}
			else
			{
				for (auto Element : CurrentSelection.SelectedActors)
				{
					GEditor->SelectActor(Element, true, true, false, true);
					CurrentSelection.SetTemporaryPivotPoint(Element, GetWidgetLocation());
				}
			}
		}
	}

	return bIsHandled;
}

FVector FCurrentSelection::GetDefaultPivotLocation() const
{
	return FSTEdMode::GetActorArrayAverageLocation(SelectedActors);
}

bool FCurrentSelection::IsSomeoneSelected() const
{
	return SelectedActors.Num() != 0;
}

void FCurrentSelection::UpdateSelection()
{
	USelection* CurrentSelectionFormEditor = GEditor->GetSelectedActors();
	TArray<AActor*> NewSelectedActors;
	CurrentSelectionFormEditor->GetSelectedObjects<AActor>(NewSelectedActors);

	if (!IsSomeoneSelected())
	{
		SelectedActors = NewSelectedActors;

		for (auto Element : SelectedActors)
		{
			FVector NewPivot;
			SelectedVertex == FVector::ZeroVector ? NewPivot = GetDefaultPivotLocation() : NewPivot = SelectedVertex;
			SetTemporaryPivotPoint(Element, NewPivot);
		}
		return;
	}

	// Select none
	if (!NewSelectedActors.Num())
	{
		ResetPivotPointForAllSelectedActors();

		SelectedActors.Empty();
		ResetSelectedVertex();
		return;
	}

	//	The same number is selected
	if (NewSelectedActors.Num() == SelectedActors.Num())
	{
		SelectedActors = NewSelectedActors;
		ResetSelectedVertex();

		return;
	}

	// More than was selected
	if (NewSelectedActors.Num() > SelectedActors.Num())
	{
		SelectedActors = NewSelectedActors;

		for (auto Element : SelectedActors)
		{
			FVector NewPivot;
			SelectedVertex == FVector::ZeroVector ? NewPivot = GetDefaultPivotLocation() : NewPivot = SelectedVertex;
			SetTemporaryPivotPoint(Element, NewPivot);
		}
		return;
	}

	//	Less than was selected
	if (NewSelectedActors.Num() < SelectedActors.Num())
	{
		for (auto Element : SelectedActors)
		{
			if (!NewSelectedActors.Contains(Element))
			{
				ResetPivotPoint(Element);
			}
		}
		SelectedActors = NewSelectedActors;

		for (auto Element : SelectedActors)
		{
			FVector NewPivot;
			SelectedVertex == FVector::ZeroVector ? NewPivot = GetDefaultPivotLocation() : NewPivot = SelectedVertex;
			SetTemporaryPivotPoint(Element, NewPivot);
		}
	}
}

void FCurrentSelection::ResetPivotPointForAllSelectedActors()
{
	for (auto Element : SelectedActors)
	{
		if (IsValid(Element))
			Element->SetPivotOffset(FVector::ZeroVector);
	}
	GUnrealEd->UpdatePivotLocationForSelection(true);
}

void FCurrentSelection::ResetPivotPoint(AActor* InActorForReset)
{
	if (InActorForReset)
		InActorForReset->SetPivotOffset(FVector::ZeroVector);
}

void FCurrentSelection::ResetSelectedVertex()
{
	SelectedVertex = FVector::ZeroVector;
}

void FSTEdMode::Enter()
{
	FEdMode::Enter();

	if (GCurrentLevelEditingViewportClient)
		FSTEdModeLocal::DPIScale = GCurrentLevelEditingViewportClient->GetDPIScale();

	if (!AsyncCalculationTaskTimer.IsValid())
		GetWorld()->GetTimerManager().SetTimer(
			AsyncCalculationTaskTimer,
			FTimerDelegate::CreateRaw(this, &FSTEdMode::RunAsyncTask),
			0.1f,
			true);

	CurrentSelection.UpdateSelection();
}

void FSTEdMode::Exit()
{
	if (AsyncCalculationTaskTimer.IsValid())
		GetWorld()->GetTimerManager().ClearTimer(AsyncCalculationTaskTimer);

	CurrentSelection.ResetPivotPointForAllSelectedActors();
	CurrentSelection.ResetSelectedVertex();

	FEdMode::Exit();
}

void FSTEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	bIsMouseMove = GetMouseVector2D() != MouseOnScreenPosition;
	MouseOnScreenPosition = GetMouseVector2D();

	FEdMode::Tick(ViewportClient, DeltaTime);
}

void FSTEdMode::ActorSelectionChangeNotify()
{
	CurrentSelection.UpdateSelection();
}

bool FSTEdMode::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale)
{
	if (InViewportClient->GetCurrentWidgetAxis() == EAxisList::None)
	{
		return false;
	}

	if (!InDrag.IsZero())
	{
		CurrentSelection.SelectedVertex = GetWidgetLocation() + InDrag;

		if (!IsKeysForChangePivotPressed())
		{
			// Do not interfere when trying to copy
			if (FSlateApplication::Get().GetModifierKeys().IsAltDown())
				return false;

			for (auto Element : CurrentSelection.SelectedActors)
			{
				Element->AddActorWorldOffset(InDrag);
			}
		}

		//TODO: Remove flickering of the pivot point when moving
		else
		{
			CurrentSelection.SelectedVertex = GetWidgetLocation() + InDrag;

			if (!bIsMouseMove)
			{
				for (auto Element : CurrentSelection.SelectedActors)
				{
					CurrentSelection.SetTemporaryPivotPoint(Element, CurrentSelection.SelectedVertex);
				}
			}
		}
		return true;
	}
	return false;
}

bool FSTEdMode::ShouldDrawWidget() const
{
	return GEditor->GetSelectedActors()->Num() && !IsKeysForSnappingPressed();
}

FVector FSTEdMode::GetWidgetLocation() const
{
	if (CurrentSelection.IsSomeoneSelected() && CurrentSelection.SelectedVertex == FVector::ZeroVector)
		return CurrentSelection.GetDefaultPivotLocation();

	return CurrentSelection.SelectedVertex;
}

FVector FSTEdMode::GetActorArrayAverageLocation(const TArray<AActor*>& Actors)
{
	FVector AverageLocation;

	if (Actors.Num() > 1)
	{
		FVector LocationSum(0, 0, 0);
		int32 ActorCount = 0;

		for (auto Element : Actors)
		{
			if (IsValid(Element) && Element->GetRootComponent())
			{
				FVector ActorBoundsOrigin;
				FVector ActorBoundsExtend;
				Element->GetActorBounds(true, ActorBoundsOrigin, ActorBoundsExtend);

				LocationSum += ActorBoundsOrigin;
				ActorCount++;
			}
		}

		// Find average
		if (ActorCount > 0)
		{
			AverageLocation = LocationSum / ActorCount;
		}
	}
	else
	{
		AverageLocation = Actors[0]->GetActorLocation();
	}
	return AverageLocation;
}

float FSTEdMode::FindScaleFromDistanceToSprite(const FSceneView* InView, FVector InCurrentVertexForDraw)
{
	return InView->WorldToScreen(InCurrentVertexForDraw).W
		* (1.0f / InView->UnscaledViewRect.Width() / InView->ViewMatrices.GetProjectionMatrix().M[0][0]);
}

void FCurrentSelection::SetTemporaryPivotPoint(AActor* InActor, FVector InPivotLocation)
{
	const FTransform NewPivotTransform = FTransform(InPivotLocation);

	InActor->SetPivotOffset(NewPivotTransform.GetRelativeTransform(InActor->GetActorTransform()).GetLocation());
	GUnrealEd->UpdatePivotLocationForSelection(true);
}

bool FSTEdMode::IsNeedTraceCheck()
{
	const auto EditorViewportClient = static_cast<FEditorViewportClient*>(GEditor->GetActiveViewport()->GetClient());

	if (!EditorViewportClient->IsPerspective())
		return false;

	switch (EditorViewportClient->GetViewMode())
	{
	case VMI_BrushWireframe:
	case VMI_Wireframe:
		return false;
	case VMI_Unlit:
	case VMI_Lit:
	case VMI_Lit_DetailLighting:
	case VMI_LightingOnly:
	case VMI_LightComplexity:
	case VMI_ShaderComplexity:
	case VMI_LightmapDensity:
		return true;

	default:
		return false;
	}
}

bool FSTEdMode::IsKeysForSnappingPressed()
{
	FSlateApplication& Slate = FSlateApplication::Get();
	return Slate.GetModifierKeys().IsLeftControlDown() && Slate.GetModifierKeys().IsLeftAltDown();
}

bool FSTEdMode::IsKeysForChangePivotPressed()
{
	FSlateApplication& Slate = FSlateApplication::Get();

	if (!Slate.GetModifierKeys().IsAltDown())
		return false;
	return Slate.GetPressedMouseButtons().Contains(EKeys::MiddleMouseButton);
}

void FSTEdMode::GetActorsWithComponentsInView(
	TMap<AActor*, TSet<UActorComponent*>>& OutActorsInView,
	TSet<TWeakObjectPtr<UPrimitiveComponent>>& OutHiddenComponents) const
{
	int32 ActorsInView = 0;

	for (TActorIterator<AActor> It(GetWorld(), AActor::StaticClass()); It; ++It)
	{
		if (ActorsInView < FSTEdModeLocal::MaxActorsInViewport)
		{
			AActor* Actor = *It;
			TSet<UActorComponent*> VisibleComponents;

			if (Actor->WasRecentlyRendered())
			{
				ActorsInView++;
				OutActorsInView.Add(Actor, Actor->GetComponents());
			}
			else
			{
				TArray<UPrimitiveComponent*> PrimitiveComponents;
				Actor->GetComponents(PrimitiveComponents);
				for (auto Element : PrimitiveComponents)
				{
					OutHiddenComponents.Add(Element);
				}
			}
		}
	}
}

FVector2D FSTEdMode::GetMouseVector2D()
{
	FViewport* ActiveViewport = GEditor ? GEditor->GetActiveViewport() : nullptr;
	return ActiveViewport
		? (FVector2D { static_cast<float>(ActiveViewport->GetMouseX()),
					   static_cast<float>(ActiveViewport->GetMouseY()) }
		   / FSTEdModeLocal::DPIScale)
		: FVector2D::ZeroVector;
}

void FSTEdMode::LoadVertexIconTexture()
{
	// Get full path to vertex icon
	const FString RelativePath =
		IPluginManager::Get().FindPlugin(FSTEdModeLocal::SmartTransformName)->GetBaseDir() / TEXT("Resources");
	FString FullPath = FPaths::ConvertRelativePathToFull(RelativePath);
	FullPath += FSTEdModeLocal::VertexIconPath;

	UTexture2D* LoadedTexture = FImageUtils::ImportFileAsTexture2D(FullPath);

	check(LoadedTexture);

	LoadedTexture->AddToRoot();
	VertexIcon = LoadedTexture;
}

void FSTEdMode::RunAsyncTask()
{
	if (!bIsAsyncTaskInWork)
	{
		PreviousActorsWithVertices = FreshActorsWithVertices;
		FreshActorsWithVertices.Empty();

		TSet<TWeakObjectPtr<UPrimitiveComponent>> HiddenComponents;
		FInfoForCalculate InfoForAsyncTask;
		InfoForAsyncTask.EdMode = this;
		InfoForAsyncTask.MouseOnScreen = MouseOnScreenPosition;
		InfoForAsyncTask.bIsNeedTraceCheck = IsNeedTraceCheck();
		GetActorsWithComponentsInView(InfoForAsyncTask.ActorWithComponents, InfoForAsyncTask.HiddenComponents);

		bIsAsyncTaskInWork = true;
		(new FAutoDeleteAsyncTask<CalculatingVerticesTask>(InfoForAsyncTask))->StartBackgroundTask();
	}
}

FVector FSTEdMode::GetLocationWithoutDepthOffset(
	FEditorViewportClient* InViewportClient,
	FVector InCurrentLocation,
	FVector InDesiredLocation)
{
	// Top and bottom. Ignore Z
	if (InViewportClient->GetViewportType() == LVT_OrthoXY
		|| InViewportClient->GetViewportType() == LVT_OrthoNegativeXY)
	{
		return FVector(InDesiredLocation.X, InDesiredLocation.Y, InCurrentLocation.Z);
	}

	// Front and back. Ignore Y
	if (InViewportClient->GetViewportType() == LVT_OrthoXZ
		|| InViewportClient->GetViewportType() == LVT_OrthoNegativeXZ)
	{
		return FVector(InDesiredLocation.X, InCurrentLocation.Y, InDesiredLocation.Z);
	}

	// Left and right. Ignore X
	if (InViewportClient->GetViewportType() == LVT_OrthoYZ
		|| InViewportClient->GetViewportType() == LVT_OrthoNegativeYZ)
	{
		return FVector(InCurrentLocation.X, InDesiredLocation.Y, InDesiredLocation.Z);
	}

	return InDesiredLocation;
}

CalculatingVerticesTask::~CalculatingVerticesTask()
{
	InfoForCalculate.EdMode->bIsAsyncTaskInWork = false;
}

void CalculatingVerticesTask::DoWork()
{
	if (!InfoForCalculate.EdMode->EdModeView || !InfoForCalculate.EdMode->EdModeView->bIsViewInfo)
		return;

	int32 TotalVertices = 0;
	TArray<FActorWithVertices> ActorsWithVerticesResult;

	FCollisionQueryParams TraceQueryParams;
	TraceQueryParams.bTraceComplex = true;

	if (InfoForCalculate.HiddenComponents.Num())
		TraceQueryParams.AddIgnoredComponents(InfoForCalculate.HiddenComponents.Array());

	for (TTuple<AActor*, TSet<UActorComponent*>>& Element : InfoForCalculate.ActorWithComponents)
	{
		FActorWithVertices ActorWithVertices;
		ActorWithVertices.Actor = Element.Key;

		for (auto CurrentActorComponent : Element.Value)
		{
			TSharedPtr<FVertexIterator> VertexGetter =
				MakeVertexIterator(Cast<UPrimitiveComponent>(CurrentActorComponent));
			if (VertexGetter.IsValid())
			{
				FVertexIterator& VertexGetterRef = *VertexGetter;

				for (; VertexGetterRef && TotalVertices < FSTEdModeLocal::MaxVerticesForDraw; ++VertexGetterRef)
				{
					FVector2D VertexOnScreen;
					InfoForCalculate.EdMode->EdModeView->WorldToPixel(VertexGetterRef.Position(), VertexOnScreen);
					VertexOnScreen /= FSTEdModeLocal::DPIScale;

					float DistanceToCursor = FVector2D::Distance(InfoForCalculate.MouseOnScreen, VertexOnScreen);

					// Get the closest to cursor vertex on each component
					if (DistanceToCursor < FSTEdModeLocal::RadiusDisplayVertices)
					{
						bool bIsLineTraceHit = false;
						if (InfoForCalculate.bIsNeedTraceCheck)
						{
							FVector VectorForTrace = VertexGetterRef.Position()
								+ InfoForCalculate.EdMode->EdModeView->GetViewDirection() * -5;
							FHitResult HitResult;

							// Line trace for each vertex to check visibility
							if (InfoForCalculate.EdMode->GetWorld()->LineTraceSingleByChannel(
									HitResult,
									InfoForCalculate.EdMode->EdModeView->ViewLocation,
									VectorForTrace,
									ECollisionChannel::ECC_Visibility,
									TraceQueryParams))
							{
								bIsLineTraceHit = true;
							}
						}
						else
							bIsLineTraceHit = false;

						if (!bIsLineTraceHit)
						{
							ActorWithVertices.VerticesForDraw.Add(VertexGetterRef.Position());
							TotalVertices++;

							// Find the vertices closest to the cursor to make a hit proxy for them
							if (DistanceToCursor < FSTEdModeLocal::HitProxyGenerationRadius)
							{
								ActorWithVertices.VerticesForHitProxy.Add(VertexGetterRef.Position());
							}
						}
					}
				}
			}
		}

		ActorsWithVerticesResult.Add(ActorWithVertices);
	}

	InfoForCalculate.EdMode->FreshActorsWithVertices = ActorsWithVerticesResult;
}
