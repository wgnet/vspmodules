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

#include "FluidSurfaceShape.h"

#include "Components/SplineComponent.h"

AFluidSurfaceShape::AFluidSurfaceShape()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	Shape = CreateDefaultSubobject<USplineComponent>(TEXT("WaterShape"));
	Shape->SetupAttachment(RootComponent);
	Shape->SetClosedLoop(true);

	// Add default spline points
	constexpr float DefaultWidth = 2048.f;
	constexpr float DefaultDepth = 150.f;

	Shape->SplineCurves.Position.Points.Empty(3);
	Shape->SplineCurves.Rotation.Points.Empty(3);
	Shape->SplineCurves.Scale.Points.Empty(3);

	Shape->SplineCurves.Position.Points.Emplace(
		0.0f,
		FVector(0.f, 0.f, 0.f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_Constant);
	Shape->SplineCurves.Rotation.Points.Emplace(0.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Constant);
	Shape->SplineCurves.Scale.Points.Emplace(
		0.0f,
		FVector(DefaultWidth, DefaultDepth, 1.0f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_Constant);

	Shape->SplineCurves.Position.Points.Emplace(
		1.0f,
		FVector(7000.f, -3000.f, 0.f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_Constant);
	Shape->SplineCurves.Rotation.Points.Emplace(1.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Constant);
	Shape->SplineCurves.Scale.Points.Emplace(
		1.0f,
		FVector(DefaultWidth, DefaultDepth, 1.0f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_Constant);

	Shape->SplineCurves.Position.Points.Emplace(
		2.0f,
		FVector(6500.f, 6500.f, 0.f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_Constant);
	Shape->SplineCurves.Rotation.Points.Emplace(2.0f, FQuat::Identity, FQuat::Identity, FQuat::Identity, CIM_Constant);
	Shape->SplineCurves.Scale.Points.Emplace(
		2.0f,
		FVector(DefaultWidth, DefaultDepth, 1.0f),
		FVector::ZeroVector,
		FVector::ZeroVector,
		CIM_Constant);
}

void AFluidSurfaceShape::GetPoints(TArray<FVector>& Positions, TArray<int32>& Indices) const
{
	const float SplineLength = FMath::Floor(Shape->GetSplineLength());
	const int32 TruncSegments = FMath::DivideAndRoundUp(SplineLength, SegmentSize);
	const int32 NumberOfSplinePoints = Shape->GetNumberOfSplinePoints();

	if ((TruncSegments < 3) && (Shape->GetNumberOfSplinePoints() >= 3))
	{
		Positions.SetNumUninitialized(NumberOfSplinePoints);
		Indices.SetNumUninitialized(NumberOfSplinePoints);
		for (int32 Index = 0; Index < NumberOfSplinePoints; Index++)
		{
			constexpr ESplineCoordinateSpace::Type CoordSpace = ESplineCoordinateSpace::World;
			const FVector LocationAtSpline = Shape->GetLocationAtSplinePoint(Index, CoordSpace);
			Positions[Index] = LocationAtSpline;
			Indices[Index] = Index;
		}
	}
	else
	{
		Positions.SetNumUninitialized(TruncSegments);
		Indices.SetNumUninitialized(TruncSegments);
		for (int32 Index = 0; Index < TruncSegments; Index++)
		{
			constexpr ESplineCoordinateSpace::Type CoordSpace = ESplineCoordinateSpace::World;
			const float Distance = FMath::Floor(Index * SegmentSize);
			const FVector LocationAtSpline = Shape->GetLocationAtDistanceAlongSpline(Distance, CoordSpace);
			Positions[Index] = LocationAtSpline;
			Indices[Index] = Index;
		}
	}
}
