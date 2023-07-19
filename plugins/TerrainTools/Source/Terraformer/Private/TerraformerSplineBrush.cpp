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

#include "TerraformerSplineBrush.h"

#include "Algo/ForEach.h"
#include "Async/ParallelFor.h"
#include "Components/BoxComponent.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Terraformer.h"
#include "TerraformerSplineComponent.h"
#include "TerrainGeneratorUtils.h"


ATerraformerSplineBrush::ATerraformerSplineBrush()
{
	SplineComp = CreateDefaultSubobject<UTerraformerSplineComponent>(TEXT("TerraformerSpline"));
	SplineComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	SplineComp->SetClosedLoop(true);

	RootComponent = SplineComp;

	Box = CreateEditorOnlyDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetupAttachment(RootComponent);
	Box->SetAbsolute(false, true, false);
	Box->bDrawOnlyIfSelected = true;
}

USplineComponent* ATerraformerSplineBrush::GetSplineComponent() const
{
	return Cast<UTerraformerSplineComponent>(SplineComp);
}

void ATerraformerSplineBrush::Render(FTerraformerBrushRenderData& Data)
{
	Super::Render(Data);
}

void ATerraformerSplineBrush::GetTriangulatedShape(
	FTransform InTransform,
	FIntPoint InRTSize,
	TArray<FVector>& OutVertices,
	TArray<int32>& OutIndexes) const
{
	TArray<int32> OutNonDegenerateTriangles;
	const bool bDegenerate = DetectDegenerateTriangles(OutVertices, OutIndexes, OutNonDegenerateTriangles);
	if (BrushType == ETerraformerBrushType::Area)
		TriangulateAreaShape(OutVertices, bDegenerate ? OutNonDegenerateTriangles : OutIndexes);
	if (BrushType == ETerraformerBrushType::Path)
		TriangulatePathShape(OutVertices, bDegenerate ? OutNonDegenerateTriangles : OutIndexes);

	ParallelFor(
		OutVertices.Num(),
		[&](int32 Idx)
		{
			OutVertices[Idx] = InTransform.InverseTransformPosition(OutVertices[Idx]);
			OutVertices[Idx] /= FVector(InRTSize.X, InRTSize.Y, 1);
		},
		!FApp::ShouldUseThreadingForPerformance());
}

void ATerraformerSplineBrush::TriangulateAreaShape(TArray<FVector>& OutVertices, TArray<int32>& OutIndexes) const
{
	const float SplineLength = FMath::Floor(GetSplineComponent()->GetSplineLength());
	const int32 TruncSegments = FMath::DivideAndRoundUp(SplineLength, SegmentSize);

	TArray<FVector> Vertices;
	Vertices.SetNumUninitialized(TruncSegments);
	for (int32 Index = 0; Index < TruncSegments; Index++)
	{
		constexpr ESplineCoordinateSpace::Type CoordSpace = ESplineCoordinateSpace::World;
		const float Distance = FMath::Floor(Index * SegmentSize);
		const FVector LocationAtSpline = GetSplineComponent()->GetLocationAtDistanceAlongSpline(Distance, CoordSpace);
		Vertices[Index] = LocationAtSpline;
	}

	TArray<int32> Triangles;
	FPolygonTriangulationUtils::TriangulatePolygon(Vertices, Triangles);

	// Topology that defines a triangle N with 3 vertex extremities: 3*N+0, 3*N+1, 3*N+2
	TArray<FVector> TriangleList;
	TriangleList.SetNumUninitialized(Triangles.Num());
	for (int32 Index = 0; Index < Triangles.Num(); Index++)
	{
		TriangleList[Index] = Vertices[Triangles[Index]];
	}

	UE_LOG(LogTerraformerEditor, Log, TEXT("Brush %s Vertices : %d"), *GetNameSafe(this), TriangleList.Num());
	UE_LOG(LogTerraformerEditor, Log, TEXT("Brush %s Indexes  : %d"), *GetNameSafe(this), Triangles.Num());
	UE_LOG(LogTerraformerEditor, Log, TEXT("..."));

	// Output
	OutVertices = TriangleList;
	OutIndexes = Triangles;
}

void ATerraformerSplineBrush::TriangulatePathShape(TArray<FVector>& OutVertices, TArray<int32>& OutIndexes) const
{
	const float DefaultScale = SplineComp->ScaleVisualizationWidth;
	const int32 NumPoints = SplineComp->GetSplinePointsPosition().Points.Num();
	const int32 NumSegments = SplineComp->GetSplinePointsPosition().bIsLooped ? NumPoints : NumPoints - 1;
	const int32 TruncSegments = FMath::TruncToInt(SplineComp->GetSplineLength() / SegmentSize) + 3;
	constexpr ESplineCoordinateSpace::Type CoordSpace = ESplineCoordinateSpace::World;

	TArray<FVector> LftPoints;
	LftPoints.SetNumUninitialized(TruncSegments);
	TArray<FVector> RgtPoints;
	RgtPoints.SetNumUninitialized(TruncSegments);

	for (int32 Iter = 0; Iter < TruncSegments; Iter++)
	{
		FVector Location = SplineComp->GetLocationAtDistanceAlongSpline(Iter * floor(SegmentSize), CoordSpace);
		FVector RightVector =
			SplineComp->GetRightVectorAtDistanceAlongSpline(Iter * floor(SegmentSize), CoordSpace).GetSafeNormal();
		FVector ScaleAlong = SplineComp->GetScaleAtDistanceAlongSpline(Iter * floor(SegmentSize));
		LftPoints[Iter] = (Location - RightVector * ScaleAlong);
		RgtPoints[Iter] = (Location + RightVector * ScaleAlong);
	}

	if ((LftPoints.Num() > 0) && (RgtPoints.Num() > 0) && (LftPoints.Num() == RgtPoints.Num()))
	{
		TArray<FVector> Vertices;
		Vertices.Reserve(LftPoints.Num() + RgtPoints.Num());
		for (int32 i = 0; i < LftPoints.Num(); ++i)
		{
			Vertices.Add(LftPoints[i]);
			Vertices.Add(RgtPoints[i]);
		}

		TArray<int32> Indexes;
		Indexes.Reserve((Vertices.Num() - 3) * 3);
		for (int32 Index = 0; Index < Vertices.Num() - 3; Index++)
		{
			Indexes.Add(Index + Index % 2 ? 1 : 0);
			Indexes.Add(Index + Index % 2 ? 0 : 1);
			Indexes.Add(Index + Index % 2 ? 2 : 2);
		}

		if (SplineComp->IsClosedLoop())
		{
		}

		OutVertices = Vertices;
		OutIndexes = Indexes;
	}
}

bool ATerraformerSplineBrush::DetectDegenerateTriangles(
	const TArray<FVector>& Vertices,
	const TArray<int32>& Triangles,
	TArray<int32>& OutNonDegenerateTriangles) const
{
	// Get triangle indices, clamping to vertex range
	const int32 MaxIndex = Vertices.Num() - 1;
	const auto GetTriIndices = [&Triangles, MaxIndex](int32 Idx)
	{
		return TTuple<int32, int32, int32>(
			FMath::Min(Triangles[Idx + 0], MaxIndex),
			FMath::Min(Triangles[Idx + 1], MaxIndex),
			FMath::Min(Triangles[Idx + 2], MaxIndex));
	};

	// Ensure number of triangle indices is multiple of three
	const int32 NumTriIndices = (Triangles.Num() / 3) * 3;

	// Detect degenerate triangles, i.e. non-unique vertex indices within the same triangle
	int32 NumDegenerateTriangles = 0;
	for (int32 IndexIdx = 0; IndexIdx < NumTriIndices; IndexIdx += 3)
	{
		int32 a, b, c;
		Tie(a, b, c) = GetTriIndices(IndexIdx);
		NumDegenerateTriangles += a == b || a == c || b == c;
	}
	if (NumDegenerateTriangles > 0)
	{
		UE_LOG(
			LogTerraformerEditor,
			Warning,
			TEXT(
				"Detected %d degenerate triangle%s with non-unique vertex indices for created mesh section in '%s'; degenerate triangles will be dropped."),
			NumDegenerateTriangles,
			NumDegenerateTriangles > 1 ? "s" : "",
			*GetFullName());
	}

	// Copy index buffer for non-degenerate triangles
	TArray<int32> IndexBuffer;
	IndexBuffer.AddUninitialized(NumTriIndices - NumDegenerateTriangles * 3);
	int32 CopyIndexIdx = 0;
	for (int32 IndexIdx = 0; IndexIdx < NumTriIndices; IndexIdx += 3)
	{
		int32 a, b, c;
		Tie(a, b, c) = GetTriIndices(IndexIdx);

		if (a != b && a != c && b != c)
		{
			IndexBuffer[CopyIndexIdx++] = a;
			IndexBuffer[CopyIndexIdx++] = b;
			IndexBuffer[CopyIndexIdx++] = c;
		}
		else
		{
			--NumDegenerateTriangles;
		}
	}

	OutNonDegenerateTriangles = IndexBuffer;

	return (NumDegenerateTriangles > 0);
}

void ATerraformerSplineBrush::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);
	GetSplineComponent()->bShouldVisualizeScale = (BrushType == ETerraformerBrushType::Path);
	UpdateBrushManager();
}

void ATerraformerSplineBrush::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (BrushType == ETerraformerBrushType::Path)
	{
		for (int32 i = 0; i < GetSplineComponent()->GetNumberOfSplinePoints(); i++)
		{
			FVector PointScale = GetSplineComponent()->GetScaleAtSplinePoint(i);
			PointScale.Z = FMath::Max(PointScale.Y, 1.f);
			PointScale.X = FMath::Max(PointScale.Y, 1.f);
			GetSplineComponent()->SetScaleAtSplinePoint(i, PointScale, false);
		}
		GetSplineComponent()->UpdateSpline();
	}

	if (BrushType == ETerraformerBrushType::Area)
	{
		for (int32 i = 0; i < GetSplineComponent()->GetNumberOfSplinePoints(); i++)
		{
			FVector PointCoord = GetSplineComponent()->GetLocationAtSplinePoint(i, ESplineCoordinateSpace::Local);
			PointCoord.Z = 0.f;
			GetSplineComponent()->SetLocationAtSplinePoint(i, PointCoord, ESplineCoordinateSpace::Local, false);
		}
		GetSplineComponent()->UpdateSpline();
	}

	const FBoxSphereBounds Bounds = GetSplineComponent()->Bounds;
	Box->SetBoxExtent(Bounds.BoxExtent + FalloffWidth, false);
	Box->SetWorldLocation(Bounds.Origin);
}
