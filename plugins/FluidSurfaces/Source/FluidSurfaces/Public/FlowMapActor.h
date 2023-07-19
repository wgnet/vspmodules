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

#include "GameFramework/Actor.h"
#include "WaterPlaneInterface.h"
#include "FlowMapActor.generated.h"

class AFluidSurfaceShape;
class UBoxComponent;
class ALandscape;
class ALandscapeProxy;
class ULandscapeComponent;
class UMaterialParameterCollection;
class UHierarchicalInstancedStaticMeshComponent;
class UMaterialInterface;

UCLASS()
class FLUIDSURFACES_API AFlowMapActor
	: public AActor
	, public IWaterPlaneInterface
{
	GENERATED_BODY()
public:
	AFlowMapActor();

	virtual void OnConstruction(const FTransform& Transform) override;

	virtual float TakeDamage(
		float DamageAmount,
		FDamageEvent const& DamageEvent,
		AController* EventInstigator,
		AActor* DamageCauser) override;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowMap")
	FIntPoint GridSize = { 128, 128 };

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowMap")
	float GridStich = 0.005f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowMap")
	UMaterialInterface* WaterMaterial;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FlowMap")
	float WaterLevel = 0.f;

	void SetSimulationNormals(UTextureRenderTarget2D* RenderTarget);

protected:
	UPROPERTY()
	UStaticMesh* WaterMesh;

	UPROPERTY()
	UMaterialInstanceDynamic* WaterMaterialDynamic;

	UPROPERTY()
	TArray<FTransform> Instances;

public:
#if WITH_EDITOR
	// Begin IWaterPlaneInterface
	virtual UPrimitiveComponent* GetSurface() override;
	virtual UPrimitiveComponent* GetTargetSurface() override;
	virtual UTexture2D* GetOutputTexture() override;
	virtual void UpdateMaterials() override;
	// End IWaterPlaneInterface
#endif

	UFUNCTION(BlueprintCallable, Category = "FlowMap")
	UPrimitiveComponent* GetWaterSurface();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Settings")
	TSoftObjectPtr<ALandscape> Landscape;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settings")
	FVector LandscapePosition;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly, Category = "Settings")
	FIntPoint LandscapeSize;

#if WITH_EDITORONLY_DATA

	/** Box for visualizing surface extents. */
	UPROPERTY(Transient)
	UBoxComponent* Box = nullptr;
#endif

protected:
	UPROPERTY(Transient)
	mutable TWeakObjectPtr<ALandscapeProxy> LandscapeProxy;

	UPROPERTY(BlueprintReadWrite)
	UHierarchicalInstancedStaticMeshComponent* WaterHISM;

#if WITH_EDITORONLY_DATA
protected:
	UPROPERTY(EditAnywhere, Category = "FlowMap")
	bool bShowPreview = false;

	UPROPERTY(EditAnywhere, Category = "FlowMap")
	FLinearColor FillColor = FLinearColor::Red;

	UPROPERTY(EditAnywhere, Category = "FlowMap")
	FIntPoint WorldSize = 204800;

	UPROPERTY(EditAnywhere, Category = "FlowMap")
	FIntPoint TextureSize = 2048;

	UPROPERTY(EditAnywhere, Category = "FlowMap")
	UTexture2D* OutputTexture;

	UPROPERTY(EditAnywhere, Category = "FlowMap")
	FString OutputTextureName = TEXT("T_Flowmap");

	UPROPERTY(EditAnywhere, Category = "FlowMap", meta = (ContentDir))
	FDirectoryPath OutputDirectory = FDirectoryPath { TEXT("/Game/Environment/Flowmaps") };

	UPROPERTY(EditAnywhere, Category = "FlowMap")
	TEnumAsByte<TextureCompressionSettings> CompressionSettings = TC_HDR;

	UPROPERTY(EditAnywhere, Category = "FlowMap")
	TEnumAsByte<TextureMipGenSettings> MipSettings = TMGS_NoMipmaps;

	UPROPERTY(VisibleInstanceOnly, Category = "FlowMap")
	UTextureRenderTarget2D* FlowMapRT;
#endif

#if WITH_EDITOR

	UFUNCTION(CallInEditor)
	void BuildWaterGrid();

	UFUNCTION(CallInEditor, Category = "FlowMap")
	void SnapBounds();

	UFUNCTION(CallInEditor, Category = "FlowMap")
	void ClearTexture();

	UFUNCTION(CallInEditor, Category = "FlowMap")
	void SaveTexture();

	UFUNCTION(CallInEditor, Category = "FlowMap")
	void FillTexture();

	UFUNCTION(CallInEditor, Category = "FlowMap")
	void CopyTexture();

	static void Triangulate(
		const TArray<FVector>& Verts,
		const TArray<int32>& Inds,
		TArray<FVector>& OutVertices,
		TArray<int32>& OutIndexes);
	void FindShapes(TArray<AFluidSurfaceShape*>& OutShapes) const;

#endif

#if WITH_EDITORONLY_DATA
private:
	UPROPERTY()
	UMaterialInterface* ForceMaterial;

	UPROPERTY()
	UMaterialInterface* CopyTextureMaterial;

	UPROPERTY()
	UMaterialInstanceDynamic* ForceMID;

	UPROPERTY()
	UStaticMeshComponent* DrawPlane;

	UPROPERTY()
	UPrimitiveComponent* TargetSurface;

	UPROPERTY()
	UMaterialParameterCollection* FluidSimulationMPC;
#endif
};
