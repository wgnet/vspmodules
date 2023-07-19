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
#include "StaticMeshHitboxVisualizer.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class USceneCaptureComponent2D;
struct FProcMeshTangent;

struct FHitboxStaticMeshData
{
	FName PrimName;
	FTransform Transform;
	TArray<FVector> Vertices;
	TArray<int32> Indexes;
	TArray<FVector> Normals;
	TArray<FProcMeshTangent> Tangents;
	TArray<FVector2D> UVs;
};

UCLASS(ClassGroup = "Hitbox", meta = (BlueprintSpawnableComponent), hidecategories = (Tags, ComponentReplication, Activation, Variable, Cooking, Collision))
class SMCOLLISIONVISUALIZER_API UStaticMeshHitboxVisualizer : public UActorComponent
{
	GENERATED_BODY()
public:
	UStaticMeshHitboxVisualizer();

	virtual void BeginDestroy() override;

	UPROPERTY(EditAnywhere, Category = "Shapes", NonTransactional)
	bool bConvex = true;

	UPROPERTY(EditAnywhere, Category = "Shapes", NonTransactional)
	bool bCapsule = true;

	UPROPERTY(EditAnywhere, Category = "Shapes", NonTransactional)
	bool bBox = true;

	UPROPERTY(EditAnywhere, Category = "Shapes", NonTransactional)
	bool bSphere = true;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional)
	FLinearColor FrontColor { 0.f, 0.5f, 0.f, 1.f };

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional)
	FLinearColor BackColor { 1.f, 0.f, 0.f, 1.f };

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional)
	float Glow = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional)
	float Opacity = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional)
	float Exponent = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional)
	float BaseReflect = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional)
	float FadeLenght = 64.f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional)
	float FadeOffset = 8.f;

	void UpdateMaterials();

	UMaterialInstanceDynamic* GetMaterial();

	UFUNCTION(BlueprintCallable)
	void BuildStaticMeshHitboxes(UStaticMeshComponent* InStaticMeshComponent);

	UFUNCTION(BlueprintCallable)
	void ClearAndMarkPendingKill();

	TArray<FHitboxStaticMeshData> GetConvexData(UStaticMesh* InStaticMesh);

	TArray<FHitboxStaticMeshData> GetCapsuleData(UStaticMesh* InStaticMesh);

	TArray<FHitboxStaticMeshData> GetBoxData(UStaticMesh* InStaticMesh);

	TArray<FHitboxStaticMeshData> GetSphereData(UStaticMesh* InStaticMesh);

private:
	UPROPERTY(Transient)
	UStaticMeshComponent* StaticMeshComponent = nullptr;

	UPROPERTY(Transient)
	TArray<UProceduralMeshComponent*> ProceduralMeshComponents;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialInstanceDynamic = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* BackMaterialInstanceDynamic = nullptr;

	UPROPERTY(Transient)
	USceneCaptureComponent2D* SceneCapture = nullptr;

	UPROPERTY(Transient)
	UMaterialInterface* GeometryMaterial = nullptr;

	UPROPERTY(Transient)
	AActor* PrevOwner = nullptr;

	const int32 SphereResolution = 16;

	int32 StencilABackValue = 10;
	int32 StencilFrontValue = 20;

	void Clear();
};
