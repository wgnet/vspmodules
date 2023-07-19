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

#include "HitboxViewer.h"

struct FHitboxData
{
	FName BoneName;
	FName PrimName;
	FTransform Transform;
	TArray<FVector> Vertices;
	TArray<int32> Indexes;
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	TArray<FVector2D> UVs;
};

class FHitboxBuilder
{
public:
	FHitboxBuilder();

	FHitboxBuilder(
		USkeletalMeshComponent* InSkeletalMeshComponent,
		UMaterialInstanceDynamic* InMaterialInstanceDynamic,
		bool InConvex,
		bool InCapsule,
		bool InBox,
		bool InSphere);

	void Build(TArray<UProceduralMeshComponent*>& OutProceduralMeshComponents, FName Mark);

	TArray<FHitboxData> GetConvexData(UPhysicsAsset* InPhysicsAsset);
	TArray<FHitboxData> GetCapsuleData(UPhysicsAsset* InPhysicsAsset);
	TArray<FHitboxData> GetBoxData(UPhysicsAsset* InPhysicsAsset);
	TArray<FHitboxData> GetSphereData(UPhysicsAsset* InPhysicsAsset);

private:
	USkeletalMeshComponent* SkeletalMeshComponent;
	UMaterialInstanceDynamic* MaterialInstanceDynamic;
	int32 StencilABackValue = 10;
	int32 StencilFrontValue = 20;
	bool bConvex;
	bool bCapsule;
	bool bBox;
	bool bSphere;

	const int32 SphereResolution = 16;
};
