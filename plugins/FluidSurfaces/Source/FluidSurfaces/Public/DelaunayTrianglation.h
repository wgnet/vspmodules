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

struct FTriangle;

struct FPoint
{
	// position of Point
	FVector Position;
	// Triangles this point belongs to
	TArray<FTriangle*> Triangles;

	FPoint(FVector Pos) : Position(Pos)
	{
	}

	bool operator==(const FPoint& Other) const
	{
		// if same position, it's same point
		return (Other.Position == Position);
	}

	void AddTriangle(FTriangle* NewTriangle)
	{
		Triangles.AddUnique(NewTriangle);
	}

	void RemoveTriangle(FTriangle* TriangleToRemove)
	{
		Triangles.Remove(TriangleToRemove);
	}

	float GetDistance(const FPoint& Other)
	{
		return (Other.Position - Position).Size();
	}
};

struct FHalfEdge
{
	// 3 vertices in CCW order
	FPoint* Vertices[2];

	FHalfEdge() {};

	FHalfEdge(FPoint* A, FPoint* B)
	{
		Vertices[0] = A;
		Vertices[1] = B;
	}

	bool DoesShare(const FHalfEdge& A) const
	{
		return (Vertices[0] == A.Vertices[1] && Vertices[1] == A.Vertices[0]);
	}

	bool operator==(const FHalfEdge& Other) const
	{
		// if same position, it's same point
		return FMemory::Memcmp(Other.Vertices, Vertices, sizeof(Vertices)) == 0;
	}
};

struct FTriangle
{
	// 3 vertices in CCW order
	FPoint* Vertices[3];
	// average points for Vertices
	FVector Center;
	// FEdges
	FHalfEdge Edges[3];

	bool operator==(const FTriangle& Other) const
	{
		// if same position, it's same point
		return FMemory::Memcmp(Other.Vertices, Vertices, sizeof(Vertices)) == 0;
	}

	FTriangle(FTriangle& Copy)
	{
		FMemory::Memcpy(Vertices, Copy.Vertices);
		FMemory::Memcpy(Edges, Copy.Edges);
		Center = Copy.Center;

		Vertices[0]->AddTriangle(this);
		Vertices[1]->AddTriangle(this);
		Vertices[2]->AddTriangle(this);
	}

	FTriangle(FPoint* A, FPoint* B, FPoint* C)
	{
		Vertices[0] = A;
		Vertices[1] = B;
		Vertices[2] = C;
		Center = (A->Position + B->Position + C->Position) / 3.0f;

		Vertices[0]->AddTriangle(this);
		Vertices[1]->AddTriangle(this);
		Vertices[2]->AddTriangle(this);
		// when you make triangle first time, make sure it stays in CCW
		MakeCCW();

		// now create edges, this should be in the CCW order
		Edges[0] = FHalfEdge(Vertices[0], Vertices[1]);
		Edges[1] = FHalfEdge(Vertices[1], Vertices[2]);
		Edges[2] = FHalfEdge(Vertices[2], Vertices[0]);
	}

	FTriangle(FPoint* A)
	{
		Vertices[0] = A;
		Vertices[1] = A;
		Vertices[2] = A;
		Center = A->Position;

		Vertices[0]->AddTriangle(this);
		Vertices[1]->AddTriangle(this);
		Vertices[2]->AddTriangle(this);

		// now create edges, this should be in the CCW order
		Edges[0] = FHalfEdge(Vertices[0], Vertices[1]);
		Edges[1] = FHalfEdge(Vertices[1], Vertices[2]);
		Edges[2] = FHalfEdge(Vertices[2], Vertices[0]);
	}

	FTriangle(FPoint* A, FPoint* B)
	{
		Vertices[0] = A;
		Vertices[1] = B;
		Vertices[2] = B;
		Center = (A->Position + B->Position) / 2.0f;

		Vertices[0]->AddTriangle(this);
		Vertices[1]->AddTriangle(this);
		Vertices[2]->AddTriangle(this);

		// now create edges, this should be in the CCW order
		Edges[0] = FHalfEdge(Vertices[0], Vertices[1]);
		Edges[1] = FHalfEdge(Vertices[1], Vertices[2]);
		Edges[2] = FHalfEdge(Vertices[2], Vertices[0]);
	}

	FTriangle()
	{
		Vertices[0] = nullptr;
		Vertices[1] = nullptr;
		Vertices[2] = nullptr;
	}

	~FTriangle()
	{
		for (int32 VertexIndex = 0; VertexIndex < 3; ++VertexIndex)
		{
			if (Vertices[VertexIndex])
			{
				Vertices[VertexIndex]->RemoveTriangle(this);
			}
		}
	}

	bool Contains(const FPoint& Other) const
	{
		return (Other == *Vertices[0] || Other == *Vertices[1] || Other == *Vertices[2]);
	}

	float GetDistance(const FPoint& Other) const
	{
		return (Other.Position - Center).Size();
	}

	float GetDistance(const FVector& Other) const
	{
		return (Other - Center).Size();
	}

	bool HasSameHalfEdge(const FTriangle* Other) const
	{
		for (int32 I = 0; I < 3; ++I)
		{
			for (int32 J = 0; J < 3; ++J)
			{
				if (Other->Edges[I] == Edges[J])
				{
					return true;
				}
			}
		}

		return false;
	}

	bool DoesShareSameEdge(const FTriangle* Other) const
	{
		for (int32 I = 0; I < 3; ++I)
		{
			for (int32 J = 0; J < 3; ++J)
			{
				if (Other->Edges[I].DoesShare(Edges[J]))
				{
					return true;
				}
			}
		}

		return false;
	}

	// find point that doesn't share with this
	// this should only get called if it shares same edge
	FPoint* FindNonSharingPoint(const FTriangle* Other) const
	{
		if (!Contains(*Other->Vertices[0]))
		{
			return Other->Vertices[0];
		}

		if (!Contains(*Other->Vertices[1]))
		{
			return Other->Vertices[1];
		}

		if (!Contains(*Other->Vertices[2]))
		{
			return Other->Vertices[2];
		}

		return NULL;
	}

private:
	void MakeCCW()
	{
		// this eventually has to happen on the plane that contains this 3 points
		// for now we ignore Z
		FVector Diff1 = Vertices[1]->Position - Vertices[0]->Position;
		FVector Diff2 = Vertices[2]->Position - Vertices[0]->Position;

		float Result = Diff1.X * Diff2.Y - Diff1.Y * Diff2.X;

		check(Result != 0.f);

		// it's in left side, we need this to be right side
		if (Result < 0.f)
		{
			// swap 1&2
			FPoint* TempPt = Vertices[2];
			Vertices[2] = Vertices[1];
			Vertices[1] = TempPt;
		}
	}
};

class FDelaunayTrianglation
{
public:
	enum ECircumCircleState
	{
		ECCS_Outside = -1,
		ECCS_On = 0,
		ECCS_Inside = 1,
	};

	void Reset();
	void EmptyTriangles();
	void EmptySamplePoints();
	void Triangulate();
	void AddSamplePoint(const FVector& NewPoint, const int32 SampleIndex);
	void Step(int32 StartIndex);
	void SortSamples();

	~FDelaunayTrianglation()
	{
		Reset();
	}

	const TArray<FTriangle*>& GetTriangleList() const
	{
		return TriangleList;
	};
	const TArray<FPoint>& GetSamplePointList() const
	{
		return SamplePointList;
	};

	void EditPointValue(int32 SamplePointIndex, FVector NewValue)
	{
		SamplePointList[SamplePointIndex] = NewValue;
	}

	int32 GetOriginalIndex(int32 NewSortedSamplePointList) const
	{
		return IndiceMappingTable[NewSortedSamplePointList];
	}

	const TArray<int32>& GetIndiceMapping()
	{
		return IndiceMappingTable;
	}

	void SetGridBox(const FBlendParameter& BlendParamX, const FBlendParameter& BlendParamY)
	{
		FBox GridBox;
		GridBox.Min.X = BlendParamX.Min;
		GridBox.Max.X = BlendParamX.Max;
		GridBox.Min.Y = BlendParamY.Min;
		GridBox.Max.Y = BlendParamY.Max;

		FVector Size = GridBox.GetSize();

		Size.X = FMath::Max(Size.X, DELTA);
		Size.Y = FMath::Max(Size.Y, DELTA);
		Size.Z = FMath::Max(Size.Z, DELTA);

		GridMin = GridBox.Min;
		RecipGridSize = FVector(1.0f, 1.0f, 1.0f) / Size;
	}

private:
	ECircumCircleState GetCircumcircleState(const FTriangle* T, const FPoint& TestPoint);

	bool IsEligibleForTriangulation(const FPoint* A, const FPoint* B, const FPoint* C)
	{
		return (IsCollinear(A, B, C) == false);
	}

	bool IsCollinear(const FPoint* A, const FPoint* B, const FPoint* C);
	bool AllCoincident(const TArray<FPoint>& InPoints);
	bool FlipTriangles(const int32 TriangleIndexOne, const int32 TriangleIndexTwo);
	void AddTriangle(FTriangle& newTriangle, bool bCheckHalfEdge = true);
	int32 GenerateTriangles(TArray<FPoint>& PointList, const int32 TotalNum);

private:
	TArray<FPoint> SamplePointList;
	TArray<int32> IndiceMappingTable;
	TArray<FTriangle*> TriangleList;
	FVector GridMin;
	FVector RecipGridSize;
};
