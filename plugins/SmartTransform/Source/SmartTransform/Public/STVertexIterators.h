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
#include "Components/BrushComponent.h"
#include "Engine/Polys.h"
#include "Rendering/SkeletalMeshRenderData.h"

//	The code was copied from the engine from the file Engine\Source\Editor\UnrealEd\Private\VertexSnapping.cpp

/**
 * Base class for an interator that iterates through the vertices on a component
 */
class FVertexIterator
{
public:
	virtual ~FVertexIterator() {};

	/** Advances to the next vertex */
	void operator++()
	{
		Advance();
	}

	/** @return True if there are more vertices on the component */
	explicit operator bool() const
	{
		return HasMoreVertices();
	}

	/**
	 * @return The position in world space of the current vertex
	 */
	virtual FVector Position() const = 0;

	/**
	 * @return The position in world space of the current vertex normal
	 */
	virtual FVector Normal() const = 0;

protected:
	/**
	 * @return True if there are more vertices on the component
	 */
	virtual bool HasMoreVertices() const = 0;

	/**
	 * Advances to the next vertex
	 */
	virtual void Advance() = 0;
};

/**
 * Iterates through the vertices of a static mesh
 */
class FStaticMeshVertexIterator : public FVertexIterator
{
public:
	FStaticMeshVertexIterator(class UStaticMeshComponent* SMC)
		: ComponentToWorldIT(SMC->GetComponentTransform().ToInverseMatrixWithScale().GetTransposed())
		, StaticMeshComponent(SMC)
		, PositionBuffer(SMC->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.PositionVertexBuffer)
		, VertexBuffer(SMC->GetStaticMesh()->GetRenderData()->LODResources[0].VertexBuffers.StaticMeshVertexBuffer)
		, CurrentVertexIndex(0)
	{
	}

	/** FVertexIterator interface */
	virtual FVector Position() const override
	{
		return StaticMeshComponent->GetComponentTransform().TransformPosition(
			PositionBuffer.VertexPosition(CurrentVertexIndex));
	}

	virtual FVector Normal() const override
	{
		return ComponentToWorldIT.TransformVector(VertexBuffer.VertexTangentZ(CurrentVertexIndex));
	}

protected:
	virtual void Advance() override
	{
		++CurrentVertexIndex;
	}

	virtual bool HasMoreVertices() const override
	{
		return CurrentVertexIndex < PositionBuffer.GetNumVertices();
	}

private:
	/** Component To World Inverse Transpose matrix */
	FMatrix ComponentToWorldIT;
	/** Component containing the mesh that we are getting vertices from */
	UStaticMeshComponent* StaticMeshComponent;
	/** The static meshes position vertex buffer */
	FPositionVertexBuffer& PositionBuffer;
	/** The static meshes vertex buffer for normals */
	FStaticMeshVertexBuffer& VertexBuffer;
	/** Current vertex index */
	uint32 CurrentVertexIndex;
};

/**
 * Vertex iterator for brush components
 */
class FBrushVertexIterator : public FVertexIterator
{
public:
	FBrushVertexIterator(UBrushComponent* InBrushComponent) : BrushComponent(InBrushComponent), CurrentVertexIndex(0)
	{
		// Build up a list of vertices
		UModel* Model = BrushComponent->Brush;
		for (int32 PolyIndex = 0; PolyIndex < Model->Polys->Element.Num(); ++PolyIndex)
		{
			FPoly& Poly = Model->Polys->Element[PolyIndex];
			for (int32 VertexIndex = 0; VertexIndex < Poly.Vertices.Num(); ++VertexIndex)
			{
				Vertices.Add(Poly.Vertices[VertexIndex]);
			}
		}
	}

	/** FVertexIterator interface */
	virtual FVector Position() const override
	{
		return BrushComponent->GetComponentTransform().TransformPosition(Vertices[CurrentVertexIndex]);
	}

	/** FVertexIterator interface */
	virtual FVector Normal() const override
	{
		return FVector::ZeroVector;
	}

protected:
	virtual void Advance() override
	{
		++CurrentVertexIndex;
	}

	virtual bool HasMoreVertices() const override
	{
		return Vertices.IsValidIndex(CurrentVertexIndex);
	}

private:
	/** The brush component getting vertices from */
	UBrushComponent* BrushComponent;
	/** All brush component vertices */
	TArray<FVector> Vertices;
	/** Current vertex index the iterator is on */
	uint32 CurrentVertexIndex;
	/** The number of vertices to iterate through */
	uint32 NumVertices;
};

/**
 * Iterates through the vertices on a component
 */
class FSkeletalMeshVertexIterator : public FVertexIterator
{
public:
	FSkeletalMeshVertexIterator(USkinnedMeshComponent* InSkinnedMeshComp)
		: ComponentToWorldIT(InSkinnedMeshComp->GetComponentTransform().ToInverseMatrixWithScale().GetTransposed())
		, SkinnedMeshComponent(InSkinnedMeshComp)
		, LODData(InSkinnedMeshComp->GetSkeletalMeshRenderData()->LODRenderData[0])
		, VertexIndex(0)
	{
	}

	/** FVertexIterator interface */
	virtual FVector Position() const override
	{
		const FVector VertPos = LODData.StaticVertexBuffers.PositionVertexBuffer.VertexPosition(VertexIndex);
		return SkinnedMeshComponent->GetComponentTransform().TransformPosition(VertPos);
	}

	virtual FVector Normal() const override
	{
		FPackedNormal TangentZ = LODData.StaticVertexBuffers.StaticMeshVertexBuffer.VertexTangentZ(VertexIndex);
		FVector VertNormal = TangentZ.ToFVector();
		return ComponentToWorldIT.TransformVector(VertNormal);
	}

protected:
	virtual void Advance() override
	{
		VertexIndex++;
	}

	virtual bool HasMoreVertices() const override
	{
		return VertexIndex < LODData.StaticVertexBuffers.PositionVertexBuffer.GetNumVertices();
	}

private:
	/** Component To World inverse transpose matrix */
	FMatrix ComponentToWorldIT;
	/** The component getting vertices from */
	USkinnedMeshComponent* SkinnedMeshComponent;
	/** Skeletal mesh render data */
	FSkeletalMeshLODRenderData& LODData;
	/** Current Soft vertex index the iterator is on */
	uint32 VertexIndex;
};

/**
 * Makes a vertex iterator from the specified component
 */
static TSharedPtr<FVertexIterator> MakeVertexIterator(UPrimitiveComponent* Component)
{
	UStaticMeshComponent* SMC = Cast<UStaticMeshComponent>(Component);
	if (SMC && SMC->GetStaticMesh())
	{
		return MakeShareable(new FStaticMeshVertexIterator(SMC));
	}

	UBrushComponent* BrushComponent = Cast<UBrushComponent>(Component);
	if (BrushComponent && BrushComponent->Brush)
	{
		return MakeShareable(new FBrushVertexIterator(BrushComponent));
	}

	USkinnedMeshComponent* SkinnedComponent = Cast<USkinnedMeshComponent>(Component);
	if (SkinnedComponent && SkinnedComponent->SkeletalMesh && SkinnedComponent->MeshObject)
	{
		return MakeShareable(new FSkeletalMeshVertexIterator(SkinnedComponent));
	}

	return nullptr;
}