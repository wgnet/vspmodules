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
#include "UObject/NoExportTypes.h"
#include "HitboxViewer.generated.h"

class UProceduralMeshComponent;
class UMaterialInstanceDynamic;
class USceneCaptureComponent2D;
struct FProcMeshTangent;

UENUM()
enum EHitboxViewOptions
{
	Top,
	Bottom,
	Left,
	Right,
	Front,
	Back
};

UCLASS(ClassGroup = "Hitbox", meta = (BlueprintSpawnableComponent), hidecategories = (Tags, ComponentReplication, Activation, Variable, Cooking, AssetUserData, Collision))
class UHitboxViewer : public UActorComponent
{
	GENERATED_BODY()
public:
	UHitboxViewer();

	virtual void BeginDestroy() override;
	virtual void PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent) override;

	UPROPERTY(EditAnywhere, Category = "View", NonTransactional, meta = (UpdateView))
	TEnumAsByte<EHitboxViewOptions> ViewOptions;

	UPROPERTY(EditAnywhere, Category = "Shapes", NonTransactional, meta = (RebuildHitboxes))
	bool bConvex = true;

	UPROPERTY(EditAnywhere, Category = "Shapes", NonTransactional, meta = (RebuildHitboxes))
	bool bCapsule = true;

	UPROPERTY(EditAnywhere, Category = "Shapes", NonTransactional, meta = (RebuildHitboxes))
	bool bBox = true;

	UPROPERTY(EditAnywhere, Category = "Shapes", NonTransactional, meta = (RebuildHitboxes))
	bool bSphere = true;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional, meta = (UpdateMaterial))
	FLinearColor FrontColor { 0.f, 0.5f, 0.f, 1.f };

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional, meta = (UpdateMaterial))
	FLinearColor BackColor { 1.f, 0.f, 0.f, 1.f };

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional, meta = (UpdateMaterial))
	float Glow = 1.25f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional, meta = (UpdateMaterial))
	float Opacity = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional, meta = (UpdateMaterial))
	float Exponent = 0.5f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional, meta = (UpdateMaterial))
	float BaseReflect = 0.05f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional, meta = (UpdateMaterial))
	float FadeLenght = 64.f;

	UPROPERTY(EditAnywhere, Category = "Material", NonTransactional, meta = (UpdateMaterial))
	float FadeOffset = 8.f;

	void UpdateMaterials();

	UMaterialInstanceDynamic* GetMaterial();

	UTextureRenderTarget2D* GetRenderTarget();

	USceneCaptureComponent2D* GetSceneCapture() const;

	void BuildHitboxes(USkeletalMeshComponent* InSkeletalMeshComponent);

	void StartRendering();

	void ClearAndMarkPendingKill();

private:
	UPROPERTY(Transient)
	USkeletalMeshComponent* SkeletalMeshComponent;

	UPROPERTY(Transient)
	TArray<UProceduralMeshComponent*> ProceduralMeshComponents;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* MaterialInstanceDynamic;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* BackMaterialInstanceDynamic;

	UPROPERTY(Transient)
	USceneCaptureComponent2D* SceneCapture;

	UPROPERTY(Transient)
	UMaterialInterface* GeometryMaterial;

	UPROPERTY(Transient)
	UMaterialInterface* PostProcessMaterial;

	UPROPERTY(Transient)
	UTextureRenderTarget2D* RenderTarget;

	UPROPERTY(Transient)
	AActor* PrevOwner;

	const FSoftObjectPath GeometryMaterialPath { TEXT("Material'/HitboxTool/M_HitboxPreview.M_HitboxPreview'") };
	const FSoftObjectPath PostProcessMaterialPath { TEXT("Material'/HitboxTool/M_HitboxPP.M_HitboxPP'") };
	const FSoftObjectPath RenderTargetPath { TEXT("Material'/HitboxTool/RT_HitboxPreview.RT_HitboxPreview'") };

	void Clear();
};
