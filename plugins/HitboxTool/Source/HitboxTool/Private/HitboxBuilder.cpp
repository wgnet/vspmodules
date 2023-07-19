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
#include "HitboxBuilder.h"

#include "Generators/CapsuleGenerator.h"
#include "Generators/MinimalBoxMeshGenerator.h"
#include "Generators/SphereGenerator.h"
#include "KismetProceduralMeshLibrary.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"

FHitboxBuilder::FHitboxBuilder(
	USkeletalMeshComponent* InSkeletalMeshComponent,
	UMaterialInstanceDynamic* InMaterialInstanceDynamic,
	bool InConvex = true,
	bool InCapsule = true,
	bool InBox = true,
	bool InSphere = true)
	: SkeletalMeshComponent(InSkeletalMeshComponent)
	, MaterialInstanceDynamic(InMaterialInstanceDynamic)
	, bConvex(InConvex)
	, bCapsule(InCapsule)
	, bBox(InBox)
	, bSphere(InSphere)
{
}

void FHitboxBuilder::Build(TArray<UProceduralMeshComponent*>& OutProceduralMeshComponents, FName Mark)
{
	if (!IsValid(SkeletalMeshComponent))
		return;

	if (UPhysicsAsset* PhysicsAsset = SkeletalMeshComponent->GetPhysicsAsset())
	{
		TArray<FHitboxData> HitboxData;

		if (bConvex)
		{
			HitboxData.Append(GetConvexData(PhysicsAsset));
		}
		if (bCapsule)
		{
			HitboxData.Append(GetCapsuleData(PhysicsAsset));
		}
		if (bBox)
		{
			HitboxData.Append(GetBoxData(PhysicsAsset));
		}
		if (bSphere)
		{
			HitboxData.Append(GetSphereData(PhysicsAsset));
		}

		for (FHitboxData& Data : HitboxData)
		{
			if (UProceduralMeshComponent* ProceduralMeshComponent = NewObject<UProceduralMeshComponent>(
					SkeletalMeshComponent->GetOwner(),
					NAME_None,
					RF_DuplicateTransient | RF_Transient | RF_TextExportTransient))
			{
				ProceduralMeshComponent->RegisterComponent();

				FAttachmentTransformRules Rules {
					EAttachmentRule::KeepRelative,
					EAttachmentRule::KeepRelative,
					EAttachmentRule::KeepRelative,
					true
				};

				ProceduralMeshComponent->AttachToComponent(SkeletalMeshComponent, Rules, Data.BoneName);

				UKismetProceduralMeshLibrary::CalculateTangentsForMesh(
					Data.Vertices,
					Data.Indexes,
					TArray<FVector2D> {},
					Data.Normals,
					Data.Tangents);
				TArray<FVector2D> EmptyArray;
				TArray<FLinearColor> EmptyColor;
				ProceduralMeshComponent->CreateMeshSection_LinearColor(
					0,
					Data.Vertices,
					Data.Indexes,
					Data.Normals,
					EmptyArray,
					EmptyColor,
					Data.Tangents,
					false);

				ProceduralMeshComponent->SetRenderCustomDepth(true);
				if (Data.PrimName == Mark)
				{
					ProceduralMeshComponent->SetCustomDepthStencilValue(StencilABackValue);
				}
				else
				{
					ProceduralMeshComponent->SetCustomDepthStencilValue(StencilFrontValue);
				}

				if (MaterialInstanceDynamic)
				{
					ProceduralMeshComponent->SetMaterial(0, MaterialInstanceDynamic);
				}

				OutProceduralMeshComponents.Add(ProceduralMeshComponent);
			}
		}
	}
}

TArray<FHitboxData> FHitboxBuilder::GetConvexData(UPhysicsAsset* InPhysicsAsset)
{
	if (InPhysicsAsset)
	{
		TArray<FHitboxData> HitboxData;
		for (USkeletalBodySetup* BodySetup : InPhysicsAsset->SkeletalBodySetups)
		{
			for (FKConvexElem& Elem : BodySetup->AggGeom.ConvexElems)
			{
				FHitboxData Data;
				if (Elem.IndexData.Num() > 0 && Elem.IndexData.Num() % 3 == 0)
				{
					TArray<FVector> Positions;
					for (FVector& Iter : Elem.VertexData)
					{
						Positions.Add(Elem.GetTransform().TransformPosition(Iter));
					}
					Data = { BodySetup->BoneName, Elem.GetName(), Elem.GetTransform(), Positions, Elem.IndexData };
				}
				else
				{
					TArray<FDynamicMeshVertex> VertexBuffer;
					TArray<uint32> IndexBuffer;
					FColor VertexColor;
					Elem.AddCachedSolidConvexGeom(VertexBuffer, IndexBuffer, VertexColor);

					TArray<int32> Indexes;
					for (uint32& i : IndexBuffer)
					{
						Indexes.Add(i);
					}

					TArray<FVector> Positions;
					for (FDynamicMeshVertex& v : VertexBuffer)
					{
						Positions.Add(v.Position);
					}
					Data = { BodySetup->BoneName, Elem.GetName(), Elem.GetTransform(), Positions, Indexes };
				}
				HitboxData.Add(Data);
			}
		}
		return HitboxData;
	}
	return TArray<FHitboxData> {};
}

TArray<FHitboxData> FHitboxBuilder::GetCapsuleData(UPhysicsAsset* InPhysicsAsset)
{
	if (InPhysicsAsset)
	{
		TArray<FHitboxData> HitboxData;
		for (USkeletalBodySetup* BodySetup : InPhysicsAsset->SkeletalBodySetups)
		{
			for (auto& Elem : BodySetup->AggGeom.SphylElems)
			{
				FCapsuleGenerator CapsuleGen;
				CapsuleGen.Radius = Elem.Radius;
				CapsuleGen.SegmentLength = Elem.Length;
				CapsuleGen.NumHemisphereArcSteps = SphereResolution / 4 + 1;
				CapsuleGen.NumCircleSteps = SphereResolution;
				CapsuleGen.bPolygroupPerQuad = false;
				CapsuleGen.Generate();

				TArray<FVector> Positions;
				for (auto& Iter : CapsuleGen.Vertices)
				{
					Positions.Add(Elem.GetTransform().TransformPosition(FVector(Iter.X, Iter.Y, Iter.Z)));
				}

				TArray<int32> Indexes;
				for (auto& Iter : CapsuleGen.Triangles)
				{
					Indexes.Add(Iter.A);
					Indexes.Add(Iter.B);
					Indexes.Add(Iter.C);
				}

				TArray<FVector> Normals;
				for (auto& Iter : CapsuleGen.Normals)
				{
					Normals.Add(FVector(Iter.X, Iter.Y, Iter.Z));
				}

				FHitboxData Data {
					BodySetup->BoneName, Elem.GetName(), Elem.GetTransform(), Positions, Indexes, Normals
				};
				HitboxData.Add(Data);
			}
		}
		return HitboxData;
	}
	return TArray<FHitboxData> {};
}

TArray<FHitboxData> FHitboxBuilder::GetBoxData(UPhysicsAsset* InPhysicsAsset)
{
	if (InPhysicsAsset)
	{
		TArray<FHitboxData> HitboxData;
		for (USkeletalBodySetup* BodySetup : InPhysicsAsset->SkeletalBodySetups)
		{
			for (auto& Elem : BodySetup->AggGeom.BoxElems)
			{
				FMinimalBoxMeshGenerator BoxGen;
				BoxGen.Box = FOrientedBox3d(
					FFrame3d(Elem.Center, Elem.Rotation.Quaternion()),
					FVector(Elem.X, Elem.Y, Elem.Z) * 0.5f);
				BoxGen.Generate();

				TArray<FVector> Positions;
				for (auto& Iter : BoxGen.Vertices)
				{
					Positions.Add(Elem.GetTransform().TransformPosition(FVector(Iter.X, Iter.Y, Iter.Z)));
				}

				TArray<int32> Indexes;
				for (auto& Iter : BoxGen.Triangles)
				{
					Indexes.Add(Iter.A);
					Indexes.Add(Iter.B);
					Indexes.Add(Iter.C);
				}

				TArray<FVector> Normals;
				for (auto& Iter : BoxGen.Normals)
				{
					Normals.Add(FVector(Iter.X, Iter.Y, Iter.Z));
				}

				FHitboxData Data {
					BodySetup->BoneName, Elem.GetName(), Elem.GetTransform(), Positions, Indexes, Normals
				};

				HitboxData.Add(Data);
			}
		}
		return HitboxData;
	}
	return TArray<FHitboxData> {};
}

TArray<FHitboxData> FHitboxBuilder::GetSphereData(UPhysicsAsset* InPhysicsAsset)
{
	if (InPhysicsAsset)
	{
		TArray<FHitboxData> HitboxData;
		for (USkeletalBodySetup* BodySetup : InPhysicsAsset->SkeletalBodySetups)
		{
			for (auto& Elem : BodySetup->AggGeom.SphereElems)
			{
				FSphereGenerator SphereGen;
				SphereGen.Radius = Elem.Radius;
				SphereGen.NumPhi = SphereGen.NumTheta = SphereResolution;
				SphereGen.bPolygroupPerQuad = false;
				SphereGen.Generate();

				TArray<FVector> Positions;
				for (auto& Iter : SphereGen.Vertices)
				{
					Positions.Add(Elem.GetTransform().TransformPosition(FVector(Iter.X, Iter.Y, Iter.Z)));
				}

				TArray<int32> Indexes;
				for (auto& Iter : SphereGen.Triangles)
				{
					Indexes.Add(Iter.A);
					Indexes.Add(Iter.B);
					Indexes.Add(Iter.C);
				}

				TArray<FVector> Normals;
				for (auto& Iter : SphereGen.Normals)
				{
					Normals.Add(FVector(Iter.X, Iter.Y, Iter.Z));
				}

				FHitboxData Data { BodySetup->BoneName, Elem.GetName(), Elem.GetTransform(), Positions, Indexes };

				HitboxData.Add(Data);
			}
		}
		return HitboxData;
	}
	return TArray<FHitboxData> {};
}
