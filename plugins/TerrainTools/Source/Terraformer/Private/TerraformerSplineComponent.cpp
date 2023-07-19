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


#include "TerraformerSplineComponent.h"

#include "TerraformerEditorSettings.h"
#include "TerraformerSplineBrush.h"

namespace FTerraformerSplineSceneProxyLocal
{
	static void DrawTerraformerSpline(
		FPrimitiveDrawInterface* PDI,
		const FSceneView* View,
		const FInterpCurveVector& SplineInfo,
		const FMatrix& LocalToWorld,
		const FLinearColor& LineColor,
		uint8 DepthPriorityGroup)
	{
		const int32 GrabHandleSize = 6;
		const int32 NumSteps = 20;
		const int32 SegmentLineThickness =
			GetDefault<UTerraformerEditorSettings>()->GetUnselectedSegmentLineThickness();

		FVector OldKeyPos(0);
		const int32 NumPoints = SplineInfo.Points.Num();
		const int32 NumSegments = SplineInfo.bIsLooped ? NumPoints : NumPoints - 1;

		for (int32 KeyIdx = 0; KeyIdx < NumSegments + 1; KeyIdx++)
		{
			const FVector NewKeyPos = LocalToWorld.TransformPosition(SplineInfo.Eval(KeyIdx, FVector(0)));
			// Draw the keypoint
			if (KeyIdx < NumPoints)
			{
				PDI->DrawPoint(NewKeyPos, LineColor, GrabHandleSize, DepthPriorityGroup);
			}
			// If not the first keypoint, draw a line to the previous keypoint.
			if (KeyIdx > 0)
			{
				// For constant interpolation - don't draw ticks - just draw dotted line.
				if (SplineInfo.Points[KeyIdx - 1].InterpMode == CIM_Constant)
				{
					// Calculate dash length according to size on screen
					const float StartW = View->WorldToScreen(OldKeyPos).W;
					const float EndW = View->WorldToScreen(NewKeyPos).W;

					constexpr float WLimit = 10.0f;
					if (StartW > WLimit || EndW > WLimit)
					{
						const float Scale = 0.03f;
						DrawDashedLine(
							PDI,
							OldKeyPos,
							NewKeyPos,
							LineColor,
							FMath::Max(StartW, EndW) * Scale,
							DepthPriorityGroup);
					}
				}
				else
				{
					// Find position on first keyframe.
					FVector OldPos = OldKeyPos;
					// Then draw a line for each substep.
					for (int32 StepIdx = 1; StepIdx <= NumSteps; StepIdx++)
					{
						const float Key = (KeyIdx - 1) + (StepIdx / static_cast<float>(NumSteps));
						const FVector NewPos = LocalToWorld.TransformPosition(SplineInfo.Eval(Key, FVector(0)));
						PDI->DrawLine(OldPos, NewPos, LineColor, DepthPriorityGroup, SegmentLineThickness);

						OldPos = NewPos;
					}
				}
			}

			OldKeyPos = NewKeyPos;
		}
	}

	class FSplineSceneProxy final : public FPrimitiveSceneProxy
	{
	public:
		SIZE_T GetTypeHash() const override
		{
			static size_t UniquePointer;
			return reinterpret_cast<size_t>(&UniquePointer);
		}

		FSplineSceneProxy(const USplineComponent* InComponent)
			: FPrimitiveSceneProxy(InComponent)
			, bDrawDebug(InComponent->bDrawDebug)
			, SplineInfo(InComponent->SplineCurves.Position)
			, LineColor(InComponent->EditorUnselectedSplineSegmentColor)
		{
		}

		virtual void GetDynamicMeshElements(
			const TArray<const FSceneView*>& Views,
			const FSceneViewFamily& ViewFamily,
			uint32 VisibilityMap,
			FMeshElementCollector& Collector) const override
		{
			QUICK_SCOPE_CYCLE_COUNTER(STAT_SplineSceneProxy_GetDynamicMeshElements);

			if (IsSelected())
			{
				return;
			}

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ViewIndex++)
			{
				if (VisibilityMap & (1 << ViewIndex))
				{
					const FSceneView* View = Views[ViewIndex];
					FPrimitiveDrawInterface* PDI = Collector.GetPDI(ViewIndex);

					const FMatrix& MatrixLocalToWorld = GetLocalToWorld();

					// Taking into account the min and maximum drawing distance
					const float DistanceSqr =
						(View->ViewMatrices.GetViewOrigin() - MatrixLocalToWorld.GetOrigin()).SizeSquared();
					if (DistanceSqr < FMath::Square(GetMinDrawDistance())
						|| DistanceSqr > FMath::Square(GetMaxDrawDistance()))
					{
						continue;
					}

					DrawTerraformerSpline(PDI, View, SplineInfo, MatrixLocalToWorld, LineColor, SDPG_World);
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Result;
			Result.bDrawRelevance =
				bDrawDebug && !IsSelected() && IsShown(View) && View->Family->EngineShowFlags.Splines;
			Result.bDynamicRelevance = true;
			Result.bShadowRelevance = IsShadowCast(View);
			Result.bEditorPrimitiveRelevance = UseEditorCompositing(View);

			return Result;
		}

		virtual uint32 GetMemoryFootprint(void) const override
		{
			return sizeof *this + GetAllocatedSize();
		}
		uint32 GetAllocatedSize(void) const
		{
			return FPrimitiveSceneProxy::GetAllocatedSize();
		}

	private:
		bool bDrawDebug;
		FInterpCurveVector SplineInfo;
		FLinearColor LineColor;
	};
}

UTerraformerSplineComponent::UTerraformerSplineComponent()
{
	UPrimitiveComponent::SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// Add default spline points
	constexpr float DefaultWidth = 2048.f;
	constexpr float DefaultDepth = 150.f;

	SplineCurves.Position.Points.Empty(3);
	SplineCurves.Rotation.Points.Empty(3);
	SplineCurves.Scale.Points.Empty(3);

	SplineCurves.Position.Points.Emplace(
		0.0f,
		FVector(0.f, 0.f, 0.f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_CurveAuto);
	SplineCurves.Rotation.Points.Emplace(0.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_CurveAuto);
	SplineCurves.Scale.Points.Emplace(
		0.0f,
		FVector(DefaultWidth, DefaultDepth, 1.0f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_CurveAuto);

	SplineCurves.Position.Points.Emplace(
		1.0f,
		FVector(7000.f, -3000.f, 0.f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_CurveAuto);
	SplineCurves.Rotation.Points.Emplace(1.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_CurveAuto);
	SplineCurves.Scale.Points.Emplace(
		1.0f,
		FVector(DefaultWidth, DefaultDepth, 1.0f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_CurveAuto);

	SplineCurves.Position.Points.Emplace(
		2.0f,
		FVector(6500.f, 6500.f, 0.f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_CurveAuto);
	SplineCurves.Rotation.Points.Emplace(2.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_CurveAuto);
	SplineCurves.Scale.Points.Emplace(
		2.0f,
		FVector(DefaultWidth, DefaultDepth, 1.0f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_CurveAuto);

	ScaleVisualizationWidth = 1.f;
	SetUnselectedSplineSegmentColor(FColor::Cyan.ReinterpretAsLinear());
	SetSelectedSplineSegmentColor(FColor::Red.ReinterpretAsLinear());
	SetTangentColor(FLinearColor::Red);
}

bool UTerraformerSplineComponent::CanEditChange(const FProperty* InProperty) const
{
	if (InProperty && InProperty->GetFName() == TEXT("ScaleVisualizationWidth"))
		return false;

	return Super::CanEditChange(InProperty);
}

TArray<ESplinePointType::Type> UTerraformerSplineComponent::GetEnabledSplinePointTypes() const
{
	return {
		ESplinePointType::Linear,
		ESplinePointType::Curve,
		ESplinePointType::CurveClamped,
		ESplinePointType::CurveCustomTangent
	};
}

bool UTerraformerSplineComponent::AllowsSplinePointScaleEditing() const
{
	if (const ATerraformerSplineBrush* Owner = Cast<ATerraformerSplineBrush>(GetOwner()))
	{
		return Owner->BrushType == ETerraformerBrushType::Path;
	}
	return false;
}

FPrimitiveSceneProxy* UTerraformerSplineComponent::CreateSceneProxy()
{
	if (!bDrawDebug)
	{
		return Super::CreateSceneProxy();
	}

	return new FTerraformerSplineSceneProxyLocal::FSplineSceneProxy(this);
}
