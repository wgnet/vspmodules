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
#include "StaticMeshHitboxVisualizer.h"

#include "Components/SceneCaptureComponent2D.h"
#include "DrawDebugHelpers.h"
#include "GameFramework/Character.h"
#include "Generators/CapsuleGenerator.h"
#include "Generators/MinimalBoxMeshGenerator.h"
#include "Generators/SphereGenerator.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "KismetProceduralMeshLibrary.h"
#include "LevelEditorViewport.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "ProceduralMeshComponent.h"


namespace SMHitboxVisualizer
{
	const FSoftObjectPath GeometryMaterialPath { TEXT(
		"Material'/SMCollisionVisualizer/MiniMapCaptureTool/Shaders/M_HitboxPreview.M_HitboxPreview'") };
}

UStaticMeshHitboxVisualizer::UStaticMeshHitboxVisualizer()
{
	GetMaterial();
	UpdateMaterials();
}

void UStaticMeshHitboxVisualizer::BeginDestroy()
{
	Super::BeginDestroy();
	Clear();
}

void UStaticMeshHitboxVisualizer::UpdateMaterials()
{
	if (UMaterialInstanceDynamic* Mid = GetMaterial())
	{
		Mid->SetVectorParameterValue(TEXT("FrontColor"), FrontColor);
		Mid->SetVectorParameterValue(TEXT("BackColor"), BackColor);
		Mid->SetScalarParameterValue(TEXT("Glow"), Glow);
		Mid->SetScalarParameterValue(TEXT("Opacity"), Opacity);
		Mid->SetScalarParameterValue(TEXT("Exponent"), Exponent);
		Mid->SetScalarParameterValue(TEXT("BaseReflect"), BaseReflect);
		Mid->SetScalarParameterValue(TEXT("FadeLenght"), FadeLenght);
		Mid->SetScalarParameterValue(TEXT("FadeOffset"), FadeOffset);
	}
}

UMaterialInstanceDynamic* UStaticMeshHitboxVisualizer::GetMaterial()
{
	if (MaterialInstanceDynamic)
	{
		return MaterialInstanceDynamic;
	}

	if (!GeometryMaterial)
	{
		GeometryMaterial = CastChecked<UMaterialInterface>(SMHitboxVisualizer::GeometryMaterialPath.TryLoad());
		MaterialInstanceDynamic =
			UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, GeometryMaterial, TEXT("DMI"));
		BackMaterialInstanceDynamic =
			UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, GeometryMaterial, TEXT("DMI"));
	}

	return MaterialInstanceDynamic;
}

void UStaticMeshHitboxVisualizer::BuildStaticMeshHitboxes(UStaticMeshComponent* InStaticMeshComponent)
{
	if (IsValid(InStaticMeshComponent))
	{
		StaticMeshComponent = InStaticMeshComponent;
		Clear();

		if (!IsValid(StaticMeshComponent))
			return;

		TArray<FHitboxStaticMeshData> HitboxData;

		if (bConvex)
		{
			HitboxData.Append(GetConvexData(StaticMeshComponent->GetStaticMesh()));
		}
		if (bCapsule)
		{
			HitboxData.Append(GetCapsuleData(StaticMeshComponent->GetStaticMesh()));
		}
		if (bBox)
		{
			HitboxData.Append(GetBoxData(StaticMeshComponent->GetStaticMesh()));
		}
		if (bSphere)
		{
			HitboxData.Append(GetSphereData(StaticMeshComponent->GetStaticMesh()));
		}

		for (FHitboxStaticMeshData& Data : HitboxData)
		{
			if (UProceduralMeshComponent* ProceduralMeshComponent = NewObject<UProceduralMeshComponent>(
					StaticMeshComponent->GetOwner(),
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

				ProceduralMeshComponent->AttachToComponent(StaticMeshComponent, Rules);

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
				if (Data.PrimName == TEXT("Back"))
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

				ProceduralMeshComponents.Add(ProceduralMeshComponent);
			}
		}
	}
}



void UStaticMeshHitboxVisualizer::Clear()
{
	for (UProceduralMeshComponent* ProceduralMesh : ProceduralMeshComponents)
	{
		if (IsValid(ProceduralMesh))
			ProceduralMesh->ConditionalBeginDestroy();
	}

	ProceduralMeshComponents.Empty();
}

void UStaticMeshHitboxVisualizer::ClearAndMarkPendingKill()
{
	Clear();
	MarkPendingKill();
}


TArray<FHitboxStaticMeshData> UStaticMeshHitboxVisualizer::GetConvexData(UStaticMesh* InStaticMesh)
{
	if (InStaticMesh)
	{
		TArray<FHitboxStaticMeshData> HitboxData;

		for (FKConvexElem& Elem : StaticMeshComponent->GetStaticMesh()->GetBodySetup()->AggGeom.ConvexElems)
		{
			FHitboxStaticMeshData Data;
			if (Elem.IndexData.Num() > 0 && Elem.IndexData.Num() % 3 == 0)
			{
				TArray<FVector> Positions;
				for (FVector& Iter : Elem.VertexData)
				{
					Positions.Add(Elem.GetTransform().TransformPosition(Iter));
				}
				Data = { Elem.GetName(), Elem.GetTransform(), Positions, Elem.IndexData };
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
				Data = { Elem.GetName(), Elem.GetTransform(), Positions, Indexes };
			}
			HitboxData.Add(Data);
		}

		return HitboxData;
	}
	return TArray<FHitboxStaticMeshData> {};
}

TArray<FHitboxStaticMeshData> UStaticMeshHitboxVisualizer::GetCapsuleData(UStaticMesh* InStaticMesh)
{
	if (InStaticMesh)
	{
		TArray<FHitboxStaticMeshData> HitboxData;

		for (auto& Elem : InStaticMesh->GetBodySetup()->AggGeom.SphylElems)
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

			FHitboxStaticMeshData Data { Elem.GetName(), Elem.GetTransform(), Positions, Indexes, Normals };
			HitboxData.Add(Data);
		}

		return HitboxData;
	}
	return TArray<FHitboxStaticMeshData> {};
}

TArray<FHitboxStaticMeshData> UStaticMeshHitboxVisualizer::GetBoxData(UStaticMesh* InStaticMesh)
{
	if (InStaticMesh)
	{
		TArray<FHitboxStaticMeshData> HitboxData;

		for (auto& Elem : InStaticMesh->GetBodySetup()->AggGeom.BoxElems)
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

			FHitboxStaticMeshData Data { Elem.GetName(), Elem.GetTransform(), Positions, Indexes, Normals };

			HitboxData.Add(Data);
		}

		return HitboxData;
	}
	return TArray<FHitboxStaticMeshData> {};
}

TArray<FHitboxStaticMeshData> UStaticMeshHitboxVisualizer::GetSphereData(UStaticMesh* InStaticMesh)
{
	if (InStaticMesh)
	{
		TArray<FHitboxStaticMeshData> HitboxData;

		for (auto& Elem : InStaticMesh->GetBodySetup()->AggGeom.SphereElems)
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

			FHitboxStaticMeshData Data { Elem.GetName(), Elem.GetTransform(), Positions, Indexes };

			HitboxData.Add(Data);
		}

		return HitboxData;
	}
	return TArray<FHitboxStaticMeshData> {};
}
