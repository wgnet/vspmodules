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

#include "TerraformerSplineVisualizer.h"

#include "TerraformerEditorSettings.h"
#include "TerraformerSplineComponent.h"

namespace FTerraformerSplineVisualizerLocal
{
	static float GetDashSize(const FSceneView* View, const FVector& Start, const FVector& End, float Scale)
	{
		const float StartW = View->WorldToScreen(Start).W;
		const float EndW = View->WorldToScreen(End).W;

		const float WLimit = 10.0f;
		if (StartW > WLimit || EndW > WLimit)
		{
			return FMath::Max(StartW, EndW) * Scale;
		}

		return 0.0f;
	}
}

IMPLEMENT_HIT_PROXY(HTerraformerSplineVisProxy, HComponentVisProxy);
IMPLEMENT_HIT_PROXY(HTerraformerSplineKeyProxy, HTerraformerSplineVisProxy);
IMPLEMENT_HIT_PROXY(HTerraformerSplineSegmentProxy, HTerraformerSplineVisProxy);
IMPLEMENT_HIT_PROXY(HTerraformerSplineTangentHandleProxy, HTerraformerSplineVisProxy);

#define LOCTEXT_NAMESPACE "TerraformerSplineVisualizer"

void FTerraformerSplineVisualizer::DrawVisualization(
	const UActorComponent* Component,
	const FSceneView* View,
	FPrimitiveDrawInterface* PDI)
{
	const USplineComponent* SplineComp = Cast<const USplineComponent>(Component);

	if (!SplineComp)
		return;

	const FInterpCurveVector& SplineInfo = SplineComp->GetSplinePointsPosition();
	const USplineComponent* EditedSplineComp = GetEditedSplineComponent();

	const USplineComponent* Archetype = CastChecked<USplineComponent>(SplineComp->GetArchetype());
	const bool bIsSplineEditable = !SplineComp->bModifiedByConstructionScript;

	const FColor NormalColor = FColor(SplineComp->EditorSelectedSplineSegmentColor.ToFColor(true));
	const FColor SelectedColor = FColor(SplineComp->EditorSelectedSplineSegmentColor.ToFColor(true));
	const FColor TangentColor = FColor(SplineComp->EditorTangentColor.ToFColor(true));
	const float GrabHandleSize = GetDefault<UTerraformerEditorSettings>()->GetSplineHandleSize();
	const int32 NumSteps = 20; // Then draw a line for each substep.
	const float SegmentLineThickness = GetDefault<UTerraformerEditorSettings>()->GetSelectedSegmentLineThickness();
	const float TangentHandleSize = 8.0f
		+ (bIsSplineEditable ? GetDefault<ULevelEditorViewportSettings>()->SplineTangentHandleSizeAdjustment : 0.0f);
	const float TangentScale = GetDefault<ULevelEditorViewportSettings>()->SplineTangentScale;

	const FMaterialRenderProxy* SphereMaterialProxy = GEngine->ArrowMaterial->GetRenderProxy();
	// Draw the tangent handles before anything else so they will not overdraw the rest of the spline
	if (SplineComp == EditedSplineComp)
	{
		check(SelectionState);

		if (SplineComp->GetNumberOfSplinePoints() == 0 && SelectionState->GetSelectedKeys().Num() > 0)
		{
			ChangeSelectionState(INDEX_NONE, false);
		}
		else
		{
			const TSet<int32> SelectedKeysCopy = SelectionState->GetSelectedKeys();
			for (int32 SelectedKey : SelectedKeysCopy)
			{
				check(SelectedKey >= 0);
				if (SelectedKey >= SplineComp->GetNumberOfSplinePoints())
				{
					// Catch any keys that might not exist anymore due to the underlying component changing.
					ChangeSelectionState(SelectedKey, true);
					continue;
				}

				if (SplineInfo.Points[SelectedKey].IsCurveKey())
				{
					const FVector Location =
						SplineComp->GetLocationAtSplinePoint(SelectedKey, ESplineCoordinateSpace::World);

					const FVector LeaveTangent =
						SplineComp->GetLeaveTangentAtSplinePoint(SelectedKey, ESplineCoordinateSpace::World)
						* TangentScale;

					const FVector ArriveTangent = SplineComp->bAllowDiscontinuousSpline
						? SplineComp->GetArriveTangentAtSplinePoint(SelectedKey, ESplineCoordinateSpace::World)
							* TangentScale
						: LeaveTangent;

					PDI->SetHitProxy(nullptr);
					PDI->DrawLine(Location, Location + LeaveTangent, TangentColor, SDPG_Foreground);
					PDI->DrawLine(Location, Location - ArriveTangent, TangentColor, SDPG_Foreground);

					if (bIsSplineEditable)
					{
						PDI->SetHitProxy(new HTerraformerSplineTangentHandleProxy(Component, SelectedKey, false));
					}
					PDI->DrawPoint(Location + LeaveTangent, TangentColor, TangentHandleSize, SDPG_Foreground);

					if (bIsSplineEditable)
					{
						PDI->SetHitProxy(new HTerraformerSplineTangentHandleProxy(Component, SelectedKey, true));
					}
					PDI->DrawPoint(Location - ArriveTangent, TangentColor, TangentHandleSize, SDPG_Foreground);

					PDI->SetHitProxy(nullptr);
				}
			}
		}
	}

	const bool bShouldVisualizeScale = SplineComp->bShouldVisualizeScale;
	const float DefaultScale = SplineComp->ScaleVisualizationWidth;

	FVector OldKeyPos(0);
	FVector OldKeyRightVector(0);
	FVector OldKeyScale(0);

	const TSet<int32>& SelectedKeys = SelectionState->GetSelectedKeys();

	const int32 NumPoints = SplineInfo.Points.Num();
	const int32 NumSegments = SplineInfo.bIsLooped ? NumPoints : NumPoints - 1;

	for (int32 KeyIdx = 0; KeyIdx < NumSegments + 1; KeyIdx++)
	{
		const FVector NewKeyPos = SplineComp->GetLocationAtSplinePoint(KeyIdx, ESplineCoordinateSpace::World);
		const FVector NewKeyRightVector =
			SplineComp->GetRightVectorAtSplinePoint(KeyIdx, ESplineCoordinateSpace::World);
		const FVector NewKeyUpVector = SplineComp->GetUpVectorAtSplinePoint(KeyIdx, ESplineCoordinateSpace::World);
		const FVector NewKeyScale = SplineComp->GetScaleAtSplinePoint(KeyIdx) * DefaultScale;

		const FColor KeyColor =
			(SplineComp == EditedSplineComp && SelectedKeys.Contains(KeyIdx)) ? SelectedColor : NormalColor;

		// Draw the keypoint and up/right vectors
		if (KeyIdx < NumPoints)
		{
			if (bShouldVisualizeScale)
			{
				PDI->SetHitProxy(nullptr);
				PDI->DrawLine(NewKeyPos, NewKeyPos - NewKeyRightVector * NewKeyScale.Y, KeyColor, SDPG_Foreground);
				PDI->DrawLine(NewKeyPos, NewKeyPos + NewKeyRightVector * NewKeyScale.Y, KeyColor, SDPG_Foreground);
			}

			if (bIsSplineEditable)
			{
				PDI->SetHitProxy(new HTerraformerSplineKeyProxy(Component, KeyIdx));
			}

			DrawSphere(
				PDI,
				NewKeyPos,
				FRotator::ZeroRotator,
				FVector(4.0f) * GrabHandleSize,
				12,
				12,
				SphereMaterialProxy,
				SDPG_Foreground);
			PDI->SetHitProxy(nullptr);
		}

		// If not the first keypoint, draw a line to the previous keypoint.
		if (KeyIdx > 0)
		{
			const FColor LineColor = NormalColor;
			if (bIsSplineEditable)
			{
				PDI->SetHitProxy(new HTerraformerSplineSegmentProxy(Component, KeyIdx - 1));
			}

			// For constant interpolation - don't draw ticks - just draw dotted line.
			if (SplineInfo.Points[KeyIdx - 1].InterpMode == CIM_Constant)
			{
				const float DashSize =
					FTerraformerSplineVisualizerLocal::GetDashSize(View, OldKeyPos, NewKeyPos, 0.03f);
				if (DashSize > 0.0f)
				{
					DrawDashedLine(PDI, OldKeyPos, NewKeyPos, LineColor, DashSize, SDPG_World);
				}
			}
			else
			{
				// Find position on first keyframe.
				FVector OldPos = OldKeyPos;
				FVector OldRightVector = OldKeyRightVector;
				FVector OldScale = OldKeyScale;

				for (int32 StepIdx = 1; StepIdx <= NumSteps; StepIdx++)
				{
					const float Key = (KeyIdx - 1) + (StepIdx / static_cast<float>(NumSteps));
					const FVector NewPos = SplineComp->GetLocationAtSplineInputKey(Key, ESplineCoordinateSpace::World);
					const FVector NewRightVector =
						SplineComp->GetRightVectorAtSplineInputKey(Key, ESplineCoordinateSpace::World);
					const FVector NewScale = SplineComp->GetScaleAtSplineInputKey(Key) * DefaultScale;

					PDI->DrawLine(OldPos, NewPos, LineColor, SDPG_Foreground, SegmentLineThickness);
					if (bShouldVisualizeScale)
					{
						PDI->DrawLine(
							OldPos - OldRightVector * OldScale.Y,
							NewPos - NewRightVector * NewScale.Y,
							LineColor,
							SDPG_Foreground);
						PDI->DrawLine(
							OldPos + OldRightVector * OldScale.Y,
							NewPos + NewRightVector * NewScale.Y,
							LineColor,
							SDPG_Foreground);
					}

					OldPos = NewPos;
					OldRightVector = NewRightVector;
					OldScale = NewScale;
				}
			}

			PDI->SetHitProxy(nullptr);
		}

		OldKeyPos = NewKeyPos;
		OldKeyRightVector = NewKeyRightVector;
		OldKeyScale = NewKeyScale;
	}
}

bool FTerraformerSplineVisualizer::VisProxyHandleClick(
	FEditorViewportClient* InViewportClient,
	HComponentVisProxy* VisProxy,
	const FViewportClick& Click)
{
	ResetTempModes();

	if (VisProxy && VisProxy->Component.IsValid())
	{
		check(SelectionState);

		if (VisProxy->IsA(HTerraformerSplineKeyProxy::StaticGetType()))
		{
			// Control point clicked
			const FScopedTransaction Transaction(LOCTEXT("SelectSplinePoint", "Select Spline Point"));

			SelectionState->Modify();

			ResetTempModes();

			if (const USplineComponent* SplineComp = UpdateSelectedSplineComponent(VisProxy))
			{
				HTerraformerSplineKeyProxy* KeyProxy = (HTerraformerSplineKeyProxy*)VisProxy;

				// Modify the selection state, unless right-clicking on an already selected key
				const TSet<int32>& SelectedKeys = SelectionState->GetSelectedKeys();
				if (Click.GetKey() != EKeys::RightMouseButton || !SelectedKeys.Contains(KeyProxy->KeyIndex))
				{
					ChangeSelectionState(KeyProxy->KeyIndex, InViewportClient->IsCtrlPressed());
				}
				SelectionState->ClearSelectedSegmentIndex();
				SelectionState->ClearSelectedTangentHandle();

				if (SelectionState->GetLastKeyIndexSelected() == INDEX_NONE)
				{
					SelectionState->SetSplinePropertyPath(FComponentPropertyPath());
					return false;
				}

				SelectionState->SetCachedRotation(SplineComp->GetQuaternionAtSplinePoint(
					SelectionState->GetLastKeyIndexSelected(),
					ESplineCoordinateSpace::World));

				return true;
			}
		}
		else if (VisProxy->IsA(HTerraformerSplineSegmentProxy::StaticGetType()))
		{
			// Spline segment clicked
			const FScopedTransaction Transaction(LOCTEXT("SelectSplineSegment", "Select Spline Segment"));

			SelectionState->Modify();

			ResetTempModes();

			if (const USplineComponent* SplineComp = UpdateSelectedSplineComponent(VisProxy))
			{
				// Divide segment into subsegments and test each subsegment against ray representing click position and camera direction.
				// Closest encounter with the spline determines the spline position.
				const int32 NumSubdivisions = 16;

				HTerraformerSplineSegmentProxy* SegmentProxy = (HTerraformerSplineSegmentProxy*)VisProxy;

				// Ignore Ctrl key, segments should only be selected one at time
				ChangeSelectionState(SegmentProxy->SegmentIndex, false);
				SelectionState->SetSelectedSegmentIndex(SegmentProxy->SegmentIndex);
				SelectionState->ClearSelectedTangentHandle();

				if (SelectionState->GetLastKeyIndexSelected() == INDEX_NONE)
				{
					SelectionState->SetSplinePropertyPath(FComponentPropertyPath());
					return false;
				}

				SelectionState->SetCachedRotation(SplineComp->GetQuaternionAtSplinePoint(
					SelectionState->GetLastKeyIndexSelected(),
					ESplineCoordinateSpace::World));

				const int32 SelectedSegmentIndex = SelectionState->GetSelectedSegmentIndex();
				const float SubsegmentStartKey = static_cast<float>(SelectedSegmentIndex);
				FVector SubsegmentStart =
					SplineComp->GetLocationAtSplineInputKey(SubsegmentStartKey, ESplineCoordinateSpace::World);

				float ClosestDistance = TNumericLimits<float>::Max();
				FVector BestLocation = SubsegmentStart;

				for (int32 Step = 1; Step < NumSubdivisions; Step++)
				{
					const float SubsegmentEndKey = SelectedSegmentIndex + Step / (float)NumSubdivisions;
					const FVector SubsegmentEnd =
						SplineComp->GetLocationAtSplineInputKey(SubsegmentEndKey, ESplineCoordinateSpace::World);

					FVector SplineClosest;
					FVector RayClosest;
					FMath::SegmentDistToSegmentSafe(
						SubsegmentStart,
						SubsegmentEnd,
						Click.GetOrigin(),
						Click.GetOrigin() + Click.GetDirection() * 50000.0f,
						SplineClosest,
						RayClosest);

					const float Distance = FVector::DistSquared(SplineClosest, RayClosest);
					if (Distance < ClosestDistance)
					{
						ClosestDistance = Distance;
						BestLocation = SplineClosest;
					}
					SubsegmentStart = SubsegmentEnd;
				}

				SelectionState->SetSelectedSplinePosition(BestLocation);

				return true;
			}
		}
		else if (VisProxy->IsA(HTerraformerSplineTangentHandleProxy::StaticGetType()))
		{
			// Spline segment clicked
			const FScopedTransaction Transaction(LOCTEXT("SelectSplineSegment", "Select Spline Segment"));

			SelectionState->Modify();

			ResetTempModes();

			if (const USplineComponent* SplineComp = UpdateSelectedSplineComponent(VisProxy))
			{
				// Tangent handle clicked
				const HTerraformerSplineTangentHandleProxy* KeyProxy =
					static_cast<HTerraformerSplineTangentHandleProxy*>(VisProxy);
				TSet<int32> SelectedKeysCopy(SelectionState->GetSelectedKeys());
				ChangeSelectionState(KeyProxy->KeyIndex, false);
				TSet<int32>& SelectedKeys = SelectionState->ModifySelectedKeys();
				for (int32 KeyIndex : SelectedKeysCopy)
				{
					if (KeyIndex != KeyProxy->KeyIndex)
					{
						SelectedKeys.Add(KeyIndex);
					}
				}

				SelectionState->ClearSelectedSegmentIndex();
				SelectionState->SetSelectedTangentHandle(KeyProxy->KeyIndex);
				SelectionState->SetSelectedTangentHandleType(
					KeyProxy->bArriveTangent ? ESelectedTangentHandle::Arrive : ESelectedTangentHandle::Leave);
				SelectionState->SetCachedRotation(SplineComp->GetQuaternionAtSplinePoint(
					SelectionState->GetSelectedTangentHandle(),
					ESplineCoordinateSpace::World));

				return true;
			}
		}
	}

	return false;
}

UTerraformerSplineComponent* FTerraformerSplineVisualizer::GetEditedTerraformerSplineComponent() const
{
	check(SelectionState != nullptr);
	return Cast<UTerraformerSplineComponent>(SelectionState->GetSplinePropertyPath().GetComponent());
}

#undef LOCTEXT_NAMESPACE
