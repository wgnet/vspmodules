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
#include "TFSplineHelper.h"
#include "Components/SplineComponent.h"
#include "Components/SplineMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UTFSplineHelper::CalcRailLength(
	const USplineComponent* Spline,
	int32& Number,
	float& Length,
	const float IdealLength)
{
	if (!Spline)
		return;

	const float ClampedLength = FMath::Clamp(IdealLength, 1.f, 100000.f);
	Number = (Spline->GetSplineLength() / ClampedLength);
	Length = (Spline->GetSplineLength() / Number);
}

void UTFSplineHelper::CalcStartEnd(
	const USplineComponent* Spline,
	FVector& LocStart,
	FVector& TanStart,
	FVector& LocEnd,
	FVector& TanEnd,
	const int32 Index,
	const float Length)
{
	if (!Spline)
		return;

	constexpr ESplineCoordinateSpace::Type L = ESplineCoordinateSpace::Local;
	LocStart = Spline->GetLocationAtDistanceAlongSpline(Index * Length, L);
	LocEnd = Spline->GetLocationAtDistanceAlongSpline((Index + 1) * Length, L);
	const FVector Tan1 = Spline->GetTangentAtDistanceAlongSpline(Index * Length, L);
	TanStart = Tan1.GetSafeNormal() * Length;
	const FVector Tan2 = Spline->GetTangentAtDistanceAlongSpline((Index + 1) * Length, L);
	TanEnd = Tan2.GetSafeNormal() * Length;
}

void UTFSplineHelper::CalcRotFromUp(
	float& Rotation,
	const USplineComponent* Spline,
	const int32 Index,
	const float Length)
{
	if (!Spline)
		return;

	constexpr ESplineCoordinateSpace::Type W = ESplineCoordinateSpace::World;
	const FVector Tan = Spline->GetTangentAtDistanceAlongSpline((Index + 1) * Length, W);
	const FVector Crossed1 =
		FVector::CrossProduct(Tan.GetSafeNormal(), Spline->GetUpVectorAtDistanceAlongSpline(Index * Length, W));
	const FVector Crossed2 =
		FVector::CrossProduct(Tan.GetSafeNormal(), Spline->GetUpVectorAtDistanceAlongSpline((Index + 1) * Length, W));
	const FVector Crossed3 = FVector::CrossProduct(Crossed1, Crossed2).GetSafeNormal();
	const float Dot1 = FVector::DotProduct(Crossed1.GetSafeNormal(), Crossed2.GetSafeNormal());
	const float Dot2 = FVector::DotProduct(Crossed3.GetSafeNormal(), Tan);
	Rotation = ((UKismetMathLibrary::SignOfFloat(Dot2)) * (-1) * (FGenericPlatformMath::Acos(Dot1)));
}

void UTFSplineHelper::ConfigSplineMesh(
	const int32& Index,
	const float& Length,
	const USplineComponent* SplineFinal,
	USplineMeshComponent* SplineMesh,
	const AActor* Actor,
	UStaticMesh* StaticMesh,
	const FStartEndScale StartEndScale,
	const float Roll)
{
	if (!SplineFinal || !SplineMesh || !Actor)
		return;

	FVector LocStart = FVector(0, 0, 0);
	FVector LocEnd = FVector(100, 0, 0);
	FVector TanStart = FVector(100, 0, 0);
	FVector TanEnd = FVector(100, 0, 0);

	CalcStartEnd(SplineFinal, LocStart, TanStart, LocEnd, TanEnd, Index, Length);
	SplineMesh->SetStartAndEnd(LocStart, TanStart, LocEnd, TanEnd, false);

	constexpr ESplineCoordinateSpace::Type W = ESplineCoordinateSpace::World;
	const FVector UpDir = SplineFinal->GetUpVectorAtDistanceAlongSpline(Index * Length, W);
	const FTransform TransformDir = Actor->GetActorTransform();
	const FVector Transformed = TransformDir.InverseTransformVectorNoScale(UpDir);
	SplineMesh->SetSplineUpDir(Transformed, true);

	float Rotation = 0;
	CalcRotFromUp(Rotation, SplineFinal, Index, Length);

	Rotation = Rotation + FMath::DegreesToRadians(Roll);
	SplineMesh->SetStartRoll(FMath::DegreesToRadians(Roll), false);
	SplineMesh->SetEndRoll(Rotation, false);
	SplineMesh->SetStartScale(StartEndScale.Start, false);
	SplineMesh->SetEndScale(StartEndScale.End, false);
	SplineMesh->SetStaticMesh(StaticMesh);
}

void UTFSplineHelper::BuildOffsetSpline(
	const USplineComponent* SplineUser,
	USplineComponent* SplineOffset,
	const float RotFromUp,
	const float OffsetDist)
{
	if (!SplineUser || !SplineOffset)
		return;

	constexpr ESplineCoordinateSpace::Type W = ESplineCoordinateSpace::World;
	SplineOffset->ClearSplinePoints(true);
	const int32 LastIndex = SplineUser->GetNumberOfSplinePoints() - 1;

	FVector UuVectorScaled = FVector(0, 0, 0);
	FVector TanAtPoint = FVector(0, 0, 0);
	FVector OffsetVector = FVector(0, 0, 0);
	FVector PointPos = FVector(0, 0, 0);

	for (int32 i = 0; i <= LastIndex; i++)
	{
		UuVectorScaled = OffsetDist * (SplineUser->GetUpVectorAtSplinePoint(i, W));
		TanAtPoint = (SplineUser->GetTangentAtSplinePoint(i, W));
		OffsetVector = UuVectorScaled.RotateAngleAxis(RotFromUp, TanAtPoint.GetSafeNormal());
		PointPos = OffsetVector + (SplineUser->GetLocationAtSplinePoint(i, W));
		SplineOffset->AddSplinePointAtIndex(PointPos, i, W, false);
		SplineOffset->SetTangentAtSplinePoint(i, TanAtPoint, W, false);
		SplineOffset->SetSplinePointType(i, SplineUser->GetSplinePointType(i), false);
	}
	SplineOffset->UpdateSpline();
	FixTangents(SplineUser, SplineOffset);
}

void UTFSplineHelper::FixTangents(const USplineComponent* SplineUser, USplineComponent* SplineOffset)
{
	if (!SplineUser || !SplineOffset)
		return;

	constexpr ESplineCoordinateSpace::Type W = ESplineCoordinateSpace::World;
	FVector ArriveTan = FVector(10, 0, 0);
	FVector LeaveTan = FVector(10, 0, 0);
	FVector UserTan = FVector(10, 0, 0);
	const int32 LastIndex = SplineUser->GetNumberOfSplinePoints() - 1;

	for (int32 a = 0; a <= 2; a++)
	{
		for (int32 i = 0; i <= LastIndex; i++)
		{
			UserTan = SplineUser->GetTangentAtSplinePoint(i, W);

			const int32 ArriveIndex = FMath::Clamp((i - 1), 0, (LastIndex + 1));
			const int32 ArriveLoopingIndex = LastIndex * ((i - 1) < 0);
			ArriveTan = UserTan
				* ((SplineOffset->GetDistanceAlongSplineAtSplinePoint(i)
					- (SplineOffset->GetDistanceAlongSplineAtSplinePoint(ArriveIndex)
					   + SplineOffset->GetDistanceAlongSplineAtSplinePoint(ArriveLoopingIndex)))
				   / (SplineUser->GetDistanceAlongSplineAtSplinePoint(i)
					  - (SplineUser->GetDistanceAlongSplineAtSplinePoint(ArriveIndex)
						 + SplineUser->GetDistanceAlongSplineAtSplinePoint(ArriveLoopingIndex))));

			const int32 LeaveLoopingIndex = (LastIndex) == (i);
			LeaveTan = UserTan
				* ((SplineOffset->GetDistanceAlongSplineAtSplinePoint(i)
					- (SplineOffset->GetDistanceAlongSplineAtSplinePoint(i + 1)
					   + (SplineOffset->GetSplineLength() * LeaveLoopingIndex)))
				   / (SplineUser->GetDistanceAlongSplineAtSplinePoint(i)
					  - (SplineUser->GetDistanceAlongSplineAtSplinePoint(i + 1)
						 + (SplineUser->GetSplineLength() * LeaveLoopingIndex))));

			SplineOffset->SetTangentsAtSplinePoint(i, ArriveTan, LeaveTan, W, false);
		}
		SplineOffset->UpdateSpline();
	}
}

void UTFSplineHelper::BuildCorrectedSpline(
	const USplineComponent* SplineUser,
	const USplineComponent* SplineOffset,
	USplineComponent* SplineFinal,
	const float IdealLength)
{
	if (!SplineUser || !SplineOffset || !SplineFinal)
		return;

	int32 NumSections;
	float Length;
	CalcRailLength(SplineUser, NumSections, Length, IdealLength);
	SplineFinal->ClearSplinePoints(true);
	const float NumLoops = NumSections + ((SplineUser->IsClosedLoop()) * -1);

	constexpr ESplineCoordinateSpace::Type W = ESplineCoordinateSpace::World;
	for (int32 i = 0; i <= NumLoops; i++)
	{
		FVector Position = SplineUser->GetLocationAtDistanceAlongSpline(i * Length, W);
		SplineFinal->AddSplinePointAtIndex(Position, i, W, false);

		FVector PosOffset = SplineOffset->FindLocationClosestToWorldLocation(Position, W);
		SplineFinal->SetUpVectorAtSplinePoint(i, (PosOffset - Position).GetSafeNormal(), W, false);

		FVector Tangent = Length * ((SplineUser->GetTangentAtDistanceAlongSpline(i * Length, W)).GetSafeNormal());
		SplineFinal->SetTangentAtSplinePoint(i, Tangent, W, false);
	}
	SplineFinal->UpdateSpline();
}

void UTFSplineHelper::GetSplineMeshVertexPositionWorldSpace(
	const USplineMeshComponent* SplineMeshComponent,
	TArray<FVector>& OutVertexPosition)
{
	auto StaticMeshToSplineMeshVertexPosition = [](const FVector& StaticMeshVertexPosition,
												   const USplineMeshComponent* SplineMeshComponent) -> FVector
	{
		const float VertexPositionAlongSpline = StaticMeshVertexPosition[SplineMeshComponent->ForwardAxis];
		const FTransform StaticMeshToSplineMeshTransform =
			SplineMeshComponent->CalcSliceTransform(VertexPositionAlongSpline);
		FVector SlicePos = StaticMeshVertexPosition;
		SlicePos[SplineMeshComponent->ForwardAxis] = 0;
		const FVector SplineMeshSpaceVector = StaticMeshToSplineMeshTransform.TransformPosition(SlicePos);
		FVector ResultPos = SplineMeshSpaceVector;
		return SplineMeshSpaceVector;
	};

	UStaticMesh* StaticMesh = SplineMeshComponent->GetStaticMesh();
	const FStaticMeshLODResources& RenderData = StaticMesh->GetRenderData()->LODResources[0];
	for (uint32 Index = 0; Index < RenderData.VertexBuffers.PositionVertexBuffer.GetNumVertices(); Index++)
	{
		const FVector StaticMeshSpacePosition = RenderData.VertexBuffers.PositionVertexBuffer.VertexPosition(Index);
		const FVector SplineMeshSpacePosition =
			StaticMeshToSplineMeshVertexPosition(StaticMeshSpacePosition, SplineMeshComponent);
		const FVector WorldSpacePosition = SplineMeshComponent->GetComponentLocation()
			+ SplineMeshComponent->GetComponentTransform().TransformVector(SplineMeshSpacePosition);
		OutVertexPosition.Add(WorldSpacePosition);
	}
}
