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
#include "DelaunayTrianglation.h"

#define BLENDSPACE_MINSAMPLE 3

struct FPointWithIndex
{
	FPoint Point;
	int32 OriginalIndex;

	FPointWithIndex(const FPoint& P, int32 InOriginalIndex) : Point(P), OriginalIndex(InOriginalIndex)
	{
	}
};

void FDelaunayTrianglation::Reset()
{
	EmptyTriangles();
	EmptySamplePoints();
	IndiceMappingTable.Empty();
}

void FDelaunayTrianglation::EmptyTriangles()
{
	for (int32 TriangleIndex = 0; TriangleIndex < TriangleList.Num(); ++TriangleIndex)
	{
		delete TriangleList[TriangleIndex];
	}

	TriangleList.Empty();
}

void FDelaunayTrianglation::EmptySamplePoints()
{
	SamplePointList.Empty();
}

void FDelaunayTrianglation::Triangulate()
{
	if (SamplePointList.Num() == 0)
	{
		return;
	}
	else if (SamplePointList.Num() == 1)
	{
		// degenerate case 1
		FTriangle Triangle(&SamplePointList[0]);
		AddTriangle(Triangle);
	}
	else if (SamplePointList.Num() == 2)
	{
		// degenerate case 2
		FTriangle Triangle(&SamplePointList[0], &SamplePointList[1]);
		AddTriangle(Triangle);
	}
	else
	{
		SortSamples();

		// first choose first 3 points
		for (int32 I = 2; I < SamplePointList.Num(); ++I)
		{
			GenerateTriangles(SamplePointList, I + 1);
		}

		// degenerate case 3: many points all collinear or coincident
		if (TriangleList.Num() == 0)
		{
			if (AllCoincident(SamplePointList))
			{
				// coincident case - just create one triangle
				FTriangle Triangle(&SamplePointList[0]);
				AddTriangle(Triangle);
			}
			else
			{
				// collinear case: create degenerate triangles between pairs of points
				for (int32 PointIndex = 0; PointIndex < SamplePointList.Num() - 1; ++PointIndex)
				{
					FTriangle Triangle(&SamplePointList[PointIndex], &SamplePointList[PointIndex + 1]);
					AddTriangle(Triangle);
				}
			}
		}
	}
}

void FDelaunayTrianglation::AddSamplePoint(const FVector& NewPoint, const int32 SampleIndex)
{
	checkf(!SamplePointList.Contains(NewPoint), TEXT("Found duplicate points in blendspace"));
	SamplePointList.Add(FPoint(NewPoint));
	IndiceMappingTable.Add(SampleIndex);
}

void FDelaunayTrianglation::SortSamples()
{
	// Populate sorting array with sample points and their original (blend space -> sample data) indices
	TArray<FPointWithIndex> SortedPoints;
	SortedPoints.Reserve(SamplePointList.Num());
	for (int32 SampleIndex = 0; SampleIndex < SamplePointList.Num(); ++SampleIndex)
	{
		SortedPoints.Add(FPointWithIndex(SamplePointList[SampleIndex], IndiceMappingTable[SampleIndex]));
	}

	// return A-B
	struct FComparePoints
	{
		FORCEINLINE bool operator()(const FPointWithIndex& A, const FPointWithIndex& B) const
		{
			// the sorting happens from -> +X, -> +Y,  -> for now ignore Z ->+Z
			if (A.Point.Position.X == B.Point.Position.X) // same, then compare Y
			{
				if (A.Point.Position.Y == B.Point.Position.Y)
				{
					return A.Point.Position.Z < B.Point.Position.Z;
				}
				else
				{
					return A.Point.Position.Y < B.Point.Position.Y;
				}
			}

			return A.Point.Position.X < B.Point.Position.X;
		}
	};
	// sort all points
	SortedPoints.Sort(FComparePoints());

	// now copy back to SamplePointList
	IndiceMappingTable.Empty(SamplePointList.Num());
	IndiceMappingTable.AddUninitialized(SamplePointList.Num());
	for (int32 SampleIndex = 0; SampleIndex < SamplePointList.Num(); ++SampleIndex)
	{
		SamplePointList[SampleIndex] = SortedPoints[SampleIndex].Point;
		IndiceMappingTable[SampleIndex] = SortedPoints[SampleIndex].OriginalIndex;
	}
}

FDelaunayTrianglation::ECircumCircleState FDelaunayTrianglation::GetCircumcircleState(
	const FTriangle* T,
	const FPoint& TestPoint)
{
	const int32 NumPointsPerTriangle = 3;

	// First off, normalize all the points
	FVector NormalizedPositions[NumPointsPerTriangle];

	// Unrolled loop
	NormalizedPositions[0] = (T->Vertices[0]->Position - GridMin) * RecipGridSize;
	NormalizedPositions[1] = (T->Vertices[1]->Position - GridMin) * RecipGridSize;
	NormalizedPositions[2] = (T->Vertices[2]->Position - GridMin) * RecipGridSize;

	const FVector NormalizedTestPoint = (TestPoint.Position - GridMin) * RecipGridSize;

	// ignore Z, eventually this has to be on plane
	// http://en.wikipedia.org/wiki/Delaunay_triangulation - determinant
	const float M00 = NormalizedPositions[0].X - NormalizedTestPoint.X;
	const float M01 = NormalizedPositions[0].Y - NormalizedTestPoint.Y;
	const float M02 = NormalizedPositions[0].X * NormalizedPositions[0].X
		- NormalizedTestPoint.X * NormalizedTestPoint.X + NormalizedPositions[0].Y * NormalizedPositions[0].Y
		- NormalizedTestPoint.Y * NormalizedTestPoint.Y;

	const float M10 = NormalizedPositions[1].X - NormalizedTestPoint.X;
	const float M11 = NormalizedPositions[1].Y - NormalizedTestPoint.Y;
	const float M12 = NormalizedPositions[1].X * NormalizedPositions[1].X
		- NormalizedTestPoint.X * NormalizedTestPoint.X + NormalizedPositions[1].Y * NormalizedPositions[1].Y
		- NormalizedTestPoint.Y * NormalizedTestPoint.Y;

	const float M20 = NormalizedPositions[2].X - NormalizedTestPoint.X;
	const float M21 = NormalizedPositions[2].Y - NormalizedTestPoint.Y;
	const float M22 = NormalizedPositions[2].X * NormalizedPositions[2].X
		- NormalizedTestPoint.X * NormalizedTestPoint.X + NormalizedPositions[2].Y * NormalizedPositions[2].Y
		- NormalizedTestPoint.Y * NormalizedTestPoint.Y;

	const float Det =
		M00 * M11 * M22 + M01 * M12 * M20 + M02 * M10 * M21 - (M02 * M11 * M20 + M01 * M10 * M22 + M00 * M12 * M21);

	// When the vertices are sorted in a counterclockwise order, the determinant is positive if and only if Testpoint lies inside the circumcircle of T.
	if (FMath::IsNegativeFloat(Det))
	{
		return ECCS_Outside;
	}
	else
	{
		// On top of the triangle edge
		if (FMath::IsNearlyZero(Det, KINDA_SMALL_NUMBER))
			return ECCS_On;
		else
			return ECCS_Inside;
	}
}

bool FDelaunayTrianglation::IsCollinear(const FPoint* A, const FPoint* B, const FPoint* C)
{
	const FVector Diff1 = B->Position - A->Position;
	const FVector Diff2 = C->Position - A->Position;
	const float Result = Diff1.X * Diff2.Y - Diff1.Y * Diff2.X;
	return (Result == 0.f);
}

bool FDelaunayTrianglation::AllCoincident(const TArray<FPoint>& InPoints)
{
	if (InPoints.Num() > 0)
	{
		const FPoint& FirstPoint = InPoints[0];
		for (int32 PointIndex = 0; PointIndex < InPoints.Num(); ++PointIndex)
		{
			const FPoint& Point = InPoints[PointIndex];
			if (Point.Position != FirstPoint.Position)
			{
				return false;
			}
		}
		return true;
	}
	return false;
}

bool FDelaunayTrianglation::FlipTriangles(const int32 TriangleIndexOne, const int32 TriangleIndexTwo)
{
	const FTriangle* A = TriangleList[TriangleIndexOne];
	const FTriangle* B = TriangleList[TriangleIndexTwo];

	// if already optimized, don't have to do any
	FPoint* TestPt = A->FindNonSharingPoint(B);

	// If it's not inside, we don't have to do any
	if (GetCircumcircleState(A, *TestPt) != ECCS_Inside)
	{
		return false;
	}

	FTriangle NewTriangles[2];
	int32 TrianglesMade = 0;

	for (int32 VertexIndexOne = 0; VertexIndexOne < 2; ++VertexIndexOne)
	{
		for (int32 VertexIndexTwo = VertexIndexOne + 1; VertexIndexTwo < 3; ++VertexIndexTwo)
		{
			// Check if these vertices form a valid triangle (should be non-colinear)
			if (IsEligibleForTriangulation(A->Vertices[VertexIndexOne], A->Vertices[VertexIndexTwo], TestPt))
			{
				// Create the new triangle and check if the final (original) vertex falls inside or outside of it's circumcircle
				const FTriangle NewTriangle(A->Vertices[VertexIndexOne], A->Vertices[VertexIndexTwo], TestPt);
				const int32 VertexIndexThree = 3 - (VertexIndexTwo + VertexIndexOne);
				if (GetCircumcircleState(&NewTriangle, *A->Vertices[VertexIndexThree]) == ECCS_Outside)
				{
					// If so store the triangle and increment the number of triangles
					checkf(TrianglesMade < 2, TEXT("Incorrect number of triangles created"));
					NewTriangles[TrianglesMade] = NewTriangle;
					++TrianglesMade;
				}
			}
		}
	}

	// In case two triangles were generated the flip was successful so we can add them to the list
	if (TrianglesMade == 2)
	{
		AddTriangle(NewTriangles[0], false);
		AddTriangle(NewTriangles[1], false);
	}

	return TrianglesMade == 2;
}

void FDelaunayTrianglation::AddTriangle(FTriangle& newTriangle, bool bCheckHalfEdge)
{
	// see if it's same vertices
	for (int32 I = 0; I < TriangleList.Num(); ++I)
	{
		if (newTriangle == *TriangleList[I])
			return;
		if (bCheckHalfEdge && newTriangle.HasSameHalfEdge(TriangleList[I]))
			return;
	}

	TriangleList.Add(new FTriangle(newTriangle));
}

int32 FDelaunayTrianglation::GenerateTriangles(TArray<FPoint>& PointList, const int32 TotalNum)
{
	if (TotalNum == BLENDSPACE_MINSAMPLE)
	{
		if (IsEligibleForTriangulation(&PointList[0], &PointList[1], &PointList[2]))
		{
			FTriangle Triangle(&PointList[0], &PointList[1], &PointList[2]);
			AddTriangle(Triangle);
		}
	}
	else if (TriangleList.Num() == 0)
	{
		FPoint* TestPoint = &PointList[TotalNum - 1];

		// so far no triangle is made, try to make it with new points that are just entered
		for (int32 I = 0; I < TotalNum - 2; ++I)
		{
			if (IsEligibleForTriangulation(&PointList[I], &PointList[I + 1], TestPoint))
			{
				FTriangle NewTriangle(&PointList[I], &PointList[I + 1], TestPoint);
				AddTriangle(NewTriangle);
			}
		}
	}
	else
	{
		// get the last addition
		FPoint* TestPoint = &PointList[TotalNum - 1];
		int32 TriangleNum = TriangleList.Num();

		for (int32 I = 0; I < TriangleList.Num(); ++I)
		{
			FTriangle* Triangle = TriangleList[I];
			if (IsEligibleForTriangulation(Triangle->Vertices[0], Triangle->Vertices[1], TestPoint))
			{
				FTriangle NewTriangle(Triangle->Vertices[0], Triangle->Vertices[1], TestPoint);
				AddTriangle(NewTriangle);
			}

			if (IsEligibleForTriangulation(Triangle->Vertices[0], Triangle->Vertices[2], TestPoint))
			{
				FTriangle NewTriangle(Triangle->Vertices[0], Triangle->Vertices[2], TestPoint);
				AddTriangle(NewTriangle);
			}

			if (IsEligibleForTriangulation(Triangle->Vertices[1], Triangle->Vertices[2], TestPoint))
			{
				FTriangle NewTriangle(Triangle->Vertices[1], Triangle->Vertices[2], TestPoint);
				AddTriangle(NewTriangle);
			}
		}

		// this is locally optimization part
		// we need to make sure all triangles are locally optimized. If not optimize it.
		for (int32 I = 0; I < TriangleList.Num(); ++I)
		{
			FTriangle* A = TriangleList[I];
			for (int32 J = I + 1; J < TriangleList.Num(); ++J)
			{
				FTriangle* B = TriangleList[J];

				// does share same edge
				if (A->DoesShareSameEdge(B))
				{
					// then test to see if locally optimized
					if (FlipTriangles(I, J))
					{
						// if this flips, remove current triangle
						delete TriangleList[I];
						delete TriangleList[J];
						//I need to remove J first because other wise,
						//  index J isn't valid anymore
						TriangleList.RemoveAt(J);
						TriangleList.RemoveAt(I);
						// start over since we modified triangle
						// once we don't have any more to flip, we're good to go!
						I = -1;
						break;
					}
				}
			}
		}
	}

	return TriangleList.Num();
}

#undef BLENDSPACE_MINSAMPLE
