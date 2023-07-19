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

#include "FlowMapActor.h"

#include "Components/BoxComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "DelaunayTrianglation.h"
#include "Engine/TextureRenderTarget2D.h"
#include "EngineUtils.h"
#include "FluidSurfaceActor.h"
#include "FluidSurfaceShape.h"
#include "FluidSurfaceSubsystem.h"
#include "FluidSurfaceUtils.h"
#include "Kismet/KismetMaterialLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "Materials/MaterialParameterCollection.h"

namespace FFlowMapActorLocal
{
	const TCHAR* WaterMeshPath(TEXT("/FluidSurfaces/FlowMap/S_WaterGrid256.S_WaterGrid256"));
	const TCHAR* PlaneMeshPath(TEXT("/FluidSurfaces/FlowMap/S_1_Unit_Plane.S_1_Unit_Plane"));
	const TCHAR* PreviewMaterialPath(TEXT("/FluidSurfaces/FlowMap/M_FlowMapPreview.M_FlowMapPreview"));
	const TCHAR* AddForceMaterialPath(TEXT("/FluidSurfaces/FlowMap/M_AddForce.M_AddForce"));
	const TCHAR* CopyTextureMaterialPath(TEXT("/FluidSurfaces/FlowMap/M_CopyTexture.M_CopyTexture"));
	const TCHAR* FlowMapRTPath(TEXT("/FluidSurfaces/FlowMap/RT_FlowMap.RT_FlowMap"));
	const TCHAR* ParameterCollectionPath(TEXT("/FluidSurfaces/Data/MPC_FluidSimulation.MPC_FluidSimulation"));
	constexpr float DefaultBoxHeight = 16.f;

	static const TArray<FVector> kOffsetsSquare3x3 {
		{ -1, +1, 0 }, { -1, 0, 0 },  { -1, -1, 0 }, { 0, +1, 0 },	{ 0, 0, 0 },
		{ 0, -1, 0 },  { +1, +1, 0 }, { +1, 0, 0 },	 { +1, -1, 0 },
	};
}

AFlowMapActor::AFlowMapActor()
{
	PrimaryActorTick.bCanEverTick = false;

	struct FConstructorStatics
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> WaterMesh;
		FConstructorStatics() : WaterMesh(FFlowMapActorLocal::WaterMeshPath)
		{
		}
	};
	static FConstructorStatics ConstructorStatics;
	WaterMesh = ConstructorStatics.WaterMesh.Get();

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	RootComponent->SetMobility(EComponentMobility::Movable);
	RootComponent->SetAbsolute(true, true, true);

	WaterHISM = CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Water"));
	WaterHISM->CreationMethod = EComponentCreationMethod::Native;
	WaterHISM->SetupAttachment(RootComponent);
	WaterHISM->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	WaterHISM->SetCollisionResponseToAllChannels(ECR_Ignore);
	WaterHISM->SetCollisionObjectType(ECC_PhysicsBody);
	WaterHISM->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Overlap);
	WaterHISM->SetCanEverAffectNavigation(false);
	WaterHISM->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	WaterHISM->SetGenerateOverlapEvents(false);
	WaterHISM->SetStaticMesh(WaterMesh);
	WaterHISM->bCastVolumetricTranslucentShadow = true;
	WaterHISM->bCastContactShadow = false;
	WaterHISM->bAffectDistanceFieldLighting = false;


#if WITH_EDITORONLY_DATA

	// Add box for visualization of bounds
	Box = CreateDefaultSubobject<UBoxComponent>(TEXT("Bounds"));
	Box->ShapeColor = FColor::Cyan;
	Box->SetBoxExtent(FVector(.5f, .5f, .5f), false);
	Box->SetRelativeTransform(FTransform(FVector(.5f, .5f, .5f)));
	Box->bDrawOnlyIfSelected = true;
	Box->SetIsVisualizationComponent(true);
	Box->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Box->SetCanEverAffectNavigation(false);
	Box->CanCharacterStepUpOn = ECanBeCharacterBase::ECB_No;
	Box->SetGenerateOverlapEvents(false);
	Box->SetupAttachment(RootComponent);

	struct FConstructorStaticsFlow
	{
		ConstructorHelpers::FObjectFinderOptional<UStaticMesh> PlaneMesh;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> PreviewMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> AddForceMaterial;
		ConstructorHelpers::FObjectFinderOptional<UMaterialInterface> CopyTextureMaterial;
		ConstructorHelpers::FObjectFinderOptional<UTextureRenderTarget2D> FlowMapRT;
		ConstructorHelpers::FObjectFinderOptional<UMaterialParameterCollection> ParameterCollection;

		FConstructorStaticsFlow()
			: PlaneMesh(FFlowMapActorLocal::PlaneMeshPath)
			, PreviewMaterial(FFlowMapActorLocal::PreviewMaterialPath)
			, AddForceMaterial(FFlowMapActorLocal::AddForceMaterialPath)
			, CopyTextureMaterial(FFlowMapActorLocal::CopyTextureMaterialPath)
			, FlowMapRT(FFlowMapActorLocal::FlowMapRTPath)
			, ParameterCollection(FFlowMapActorLocal::ParameterCollectionPath)
		{
		}
	};
	static FConstructorStaticsFlow ConstructorStaticsFlow;

	DrawPlane = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("DrawPlane"));
	DrawPlane->SetupAttachment(RootComponent);
	DrawPlane->SetStaticMesh(ConstructorStaticsFlow.PlaneMesh.Get());
	DrawPlane->SetMaterial(0, ConstructorStaticsFlow.PreviewMaterial.Get());
	DrawPlane->SetWorldScale3D(FVector::OneVector);
	DrawPlane->SetCanEverAffectNavigation(false);
	DrawPlane->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	ForceMaterial = ConstructorStaticsFlow.AddForceMaterial.Get();
	CopyTextureMaterial = ConstructorStaticsFlow.CopyTextureMaterial.Get();
	FlowMapRT = ConstructorStaticsFlow.FlowMapRT.Get();
	FluidSimulationMPC = ConstructorStaticsFlow.ParameterCollection.Get();
#endif
}

void AFlowMapActor::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	WaterHISM->SetMaterial(0, WaterMaterial);
	WaterMaterialDynamic = WaterHISM->CreateAndSetMaterialInstanceDynamic(0);

#if WITH_EDITOR
	SnapBounds();
#endif
}

void AFlowMapActor::SetSimulationNormals(UTextureRenderTarget2D* RenderTarget)
{
	if (IsValid(WaterMaterialDynamic))
	{
		WaterMaterialDynamic->SetTextureParameterValue(TEXT("RT"), RenderTarget);
	}
	else
	{
		WaterHISM->SetMaterial(0, WaterMaterial);
		WaterMaterialDynamic = WaterHISM->CreateAndSetMaterialInstanceDynamic(0);
		WaterMaterialDynamic->SetTextureParameterValue(TEXT("RT"), RenderTarget);
	}
}

float AFlowMapActor::TakeDamage(
	float DamageAmount,
	FDamageEvent const& DamageEvent,
	AController* EventInstigator,
	AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);

	if (const UWorld* World = GetWorld())
	{
		if (AFluidSurfaceActor* FluidSurfaceActor = UFluidSurfaceSubsystem::Get(World)->GetFluidSurfaceActor())
		{
			FluidSurfaceActor->TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
		}
	}

	return 0;
}

UPrimitiveComponent* AFlowMapActor::GetWaterSurface()
{
	return Cast<UPrimitiveComponent>(WaterHISM);
}

#if WITH_EDITOR
void AFlowMapActor::BuildWaterGrid()
{
	if (!Landscape.IsValid())
		return;

	if (!WaterHISM->GetStaticMesh())
		return;

	SnapBounds();

	Instances.Empty();
	WaterHISM->ClearInstances();
	const FVector MeshExtent = WaterHISM->GetStaticMesh()->GetBounds().BoxExtent;

	TArray<AFluidSurfaceShape*> OutShapes;
	FindShapes(OutShapes);

	auto PointInTriangle =
		[](const FVector& A, const FVector& B, const FVector& C, const FVector& P, const float InEpsilon)
	{
		auto VectorsOnSameSide = [](const FVector& Vec, const FVector& A, const FVector& B, const float SameSideEpsilon)
		{
			const FVector CrossA = Vec ^ A;
			const FVector CrossB = Vec ^ B;
			const float DotWithEpsilon = SameSideEpsilon + (CrossA | CrossB);
			return !FMath::IsNegativeFloat(DotWithEpsilon);
		};

		if (VectorsOnSameSide(B - A, P - A, C - A, InEpsilon) && VectorsOnSameSide(C - B, P - B, A - B, InEpsilon)
			&& VectorsOnSameSide(A - C, P - C, B - C, InEpsilon))
		{
			return true;
		}
		return false;
	};

	FVector LandCenter;
	FVector LandExtent;
	int32 LandSubsections;
	int32 LandQuadsSize;
	FFluidSurfaceUtils::GetLandscapeCenterAndExtent(
		Landscape.Get(),
		LandCenter,
		LandExtent,
		LandSubsections,
		LandQuadsSize);

	TArray<FVector> TriangleList;

	for (const AFluidSurfaceShape* Shape : OutShapes)
	{
		TArray<FVector> Positions;
		TArray<int32> Indices;
		Shape->GetPoints(Positions, Indices);

		if (Positions.Num() <= 3)
			continue;

		TArray<FVector> OutVertices;
		TArray<int32> OutIndexes;
		Triangulate(Positions, Indices, OutVertices, OutIndexes);
		TriangleList.Append(OutVertices);
	}

	if (TriangleList.Num() < 3)
		return;

	TArray<FTransform> Transforms;
	const FVector Scale = FVector(FVector2D(LandExtent) / (FVector2D(MeshExtent) * GridSize), 1);

	for (int32 x = 0; x < GridSize.X; x++)
	{
		for (int32 y = 0; y < GridSize.Y; y++)
		{
			const float XPos = MeshExtent.X * 2 * x + MeshExtent.X;
			const float YPos = MeshExtent.Y * 2 * y + MeshExtent.Y;
			const FVector Position = FVector(XPos, YPos, 0) * Scale;

			const FTransform Transform { FQuat::Identity, Position * FVector(1, 1, 0), Scale + GridStich };
			Transforms.Add(Transform);
		}
	}

	for (const FTransform& Transform : Transforms)
	{
		bool bResult = false;
		for (const FVector& Offset : FFlowMapActorLocal::kOffsetsSquare3x3)
		{
			const FVector CheckLocation = (MeshExtent * Scale) * Offset + Transform.GetLocation() - LandExtent;

			for (int32 Index = 0; Index < TriangleList.Num(); Index += 3)
			{
				const FVector PA = TriangleList[Index + 0] * FVector(1, 1, 0);
				const FVector PB = TriangleList[Index + 1] * FVector(1, 1, 0);
				const FVector PC = TriangleList[Index + 2] * FVector(1, 1, 0);

				bResult = PointInTriangle(PA, PB, PC, CheckLocation, 0);

				if (bResult)
					break;
			}
			if (bResult)
				break;
		}

		if (bResult)
		{
			Instances.Add(Transform);
		}
	}

	WaterHISM->SetRelativeLocation(FVector(-FVector2D(LandExtent), 0));
	WaterHISM->AddInstances(Instances, true);
	WaterHISM->MarkRenderStateDirty();
	Modify();
}

void AFlowMapActor::FindShapes(TArray<AFluidSurfaceShape*>& OutShapes) const
{
	UWorld* World = GetWorld();
	if (!LandscapeProxy.IsValid() && (World != nullptr))
	{
		for (TActorIterator<AFluidSurfaceShape> It(World); It; ++It)
		{
			OutShapes.Add(*It);
		}
	}
}

UPrimitiveComponent* AFlowMapActor::GetSurface()
{
	return DrawPlane;
}

UPrimitiveComponent* AFlowMapActor::GetTargetSurface()
{
	return TargetSurface;
}

UTexture2D* AFlowMapActor::GetOutputTexture()
{
	return OutputTexture;
}

void AFlowMapActor::UpdateMaterials()
{
	WaterHISM->SetMaterial(0, WaterMaterial);
	WaterMaterialDynamic = WaterHISM->CreateAndSetMaterialInstanceDynamic(0);
}

void AFlowMapActor::Triangulate(
	const TArray<FVector>& Verts,
	const TArray<int32>& Inds,
	TArray<FVector>& OutVertices,
	TArray<int32>& OutIndexes)
{
	FDelaunayTrianglation Delaunay;
	for (int32 Index = 0; Index < Verts.Num(); Index++)
	{
		Delaunay.AddSamplePoint(Verts[Index], Inds[Index]);
	}
	Delaunay.Triangulate();

	const TArray<FPoint>& Points = Delaunay.GetSamplePointList();
	const TArray<FTriangle*>& Triangles = Delaunay.GetTriangleList();

	TArray<FVector> Vertices;
	for (int32 Index = 0; Index < Triangles.Num(); Index++)
	{
		Vertices.Add(Triangles[Index]->Vertices[0]->Position);
		Vertices.Add(Triangles[Index]->Vertices[1]->Position);
		Vertices.Add(Triangles[Index]->Vertices[2]->Position);
	}

	TArray<int32> Indexes;
	for (int32 Index = 0; Index < Vertices.Num(); Index++)
	{
		Indexes.Add(Index);
	}

	OutVertices = Vertices;
	OutIndexes = Indexes;
}

void AFlowMapActor::SnapBounds()
{
	if (UPrimitiveComponent* Component = GetWaterSurface())
	{
		TargetSurface = Component;
	}

	if (Landscape.IsValid())
	{
		FVector Origin;
		FVector Extent;
		int32 Subsections;
		int32 QuadsSize;
		FFluidSurfaceUtils::GetLandscapeCenterAndExtent(Landscape.Get(), Origin, Extent, Subsections, QuadsSize);

		WorldSize = (Landscape->GetBoundingRect() + 1).Max * Landscape->GetActorScale().GetMin();
		const FVector RelativeLocation = GetActorTransform().InverseTransformPosition(Origin);

		DrawPlane->SetRelativeLocation(FVector(RelativeLocation.X, RelativeLocation.Y, 0));

		Box->SetBoxExtent(FVector(WorldSize.X * 0.5f, WorldSize.Y * 0.5f, FFlowMapActorLocal::DefaultBoxHeight));
		Box->SetRelativeLocation(FVector(RelativeLocation.X, RelativeLocation.Y, 0));

		LandscapePosition = Landscape->GetActorLocation();
		LandscapeSize = WorldSize;
	}

	DrawPlane->SetWorldScale3D(FVector(WorldSize.X, WorldSize.Y, WorldSize.GetMax()));
	DrawPlane->SetVisibility(bShowPreview);

	if (UWorld* World = GetWorld())
	{
		UKismetMaterialLibrary::SetVectorParameterValue(
			World,
			FluidSimulationMPC,
			TEXT("LandscapePosition"),
			LandscapePosition);
		UKismetMaterialLibrary::SetVectorParameterValue(
			World,
			FluidSimulationMPC,
			TEXT("LandscapeSize"),
			FLinearColor(LandscapeSize.X, LandscapeSize.Y, 0, 0));
	}
}

void AFlowMapActor::ClearTexture()
{
	if (FlowMapRT)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, FlowMapRT);
	}
}

void AFlowMapActor::SaveTexture()
{
	if (OutputTexture && FlowMapRT)
	{
		UKismetRenderingLibrary::ConvertRenderTargetToTexture2DEditorOnly(GetWorld(), FlowMapRT, OutputTexture);
	}
	else if (FlowMapRT)
	{
		FlowMapRT->ResizeTarget(TextureSize.X, TextureSize.Y);
		FString Dir = FPaths::Combine(OutputDirectory.Path, OutputTextureName);
		Dir = FPaths::CreateStandardFilename(Dir);
		OutputTexture = UKismetRenderingLibrary::RenderTargetCreateStaticTexture2DEditorOnly(
			FlowMapRT,
			Dir,
			CompressionSettings,
			MipSettings);
	}
	UpdateMaterials();
	IWaterPlaneInterface::Execute_UpdateMaterialsTrigger(this);
	IWaterPlaneInterface::Execute_EndPaint(this);
}

void AFlowMapActor::FillTexture()
{
	if (FlowMapRT)
	{
		UKismetRenderingLibrary::ClearRenderTarget2D(this, FlowMapRT, FillColor);
	}
}

void AFlowMapActor::CopyTexture()
{
	if (OutputTexture && FlowMapRT)
	{
		UMaterialInstanceDynamic* CopyMID =
			UKismetMaterialLibrary::CreateDynamicMaterialInstance(this, CopyTextureMaterial, TEXT("CopyTexture"));
		CopyMID->SetTextureParameterValue(TEXT("Tex"), OutputTexture);
		UKismetRenderingLibrary::DrawMaterialToRenderTarget(this, FlowMapRT, CopyMID);
	}
}

#endif
