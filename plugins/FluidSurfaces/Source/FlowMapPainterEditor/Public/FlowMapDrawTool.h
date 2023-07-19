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

#include "BaseTools/MeshSurfacePointTool.h"
#include "FlowMapDrawTool.generated.h"


UENUM()
enum class EFlowmapToolBrushType : uint8
{
	FMT_Flowmap = 0 UMETA(DisplayName = "Flowmap [RG]"),
	FMT_Swamp = 1	UMETA(DisplayName = "Swamp [B]"),
	FMT_Foam = 2	UMETA(DisplayName = "Foam [A]")
};

UCLASS()
class FLOWMAPPAINTEREDITOR_API UFlowMapDrawToolBuilder : public UMeshSurfacePointToolBuilder
{
	GENERATED_BODY()
public:
	virtual UMeshSurfacePointTool* CreateNewTool(const FToolBuilderState& SceneState) const override;
	virtual bool CanBuildTool(const FToolBuilderState& SceneState) const override;
	virtual void InitializeNewTool(UMeshSurfacePointTool* Tool, const FToolBuilderState& SceneState) const override;
};

UCLASS()
class FLOWMAPPAINTEREDITOR_API UFlowMapDrawToolProperties : public UInteractiveToolPropertySet
{
	GENERATED_BODY()
public:
	UFlowMapDrawToolProperties();

	UPROPERTY(EditAnywhere, Category = Brush)
	EFlowmapToolBrushType BrushType;

	UPROPERTY(EditAnywhere, Category = Brush, meta = (UIMin = "0.0", ClampMin = "0.0"))
	float BrushSize = 2048.f;

	UPROPERTY(EditAnywhere, Category = Brush, meta = (UIMin = "0", UIMax = "1", ClampMin = "0", ClampMax = "1"))
	float Density = 0.1f;

	UPROPERTY(EditAnywhere, Category = Brush, meta = (UIMin = "0", UIMax = "1", ClampMin = "0", ClampMax = "1"))
	float Hardness = 0.5f;

	UPROPERTY(EditAnywhere, Category = Brush, meta = (UIMin = "0", UIMax = "1", ClampMin = "0", ClampMax = "1"))
	float Erase = 0.1f;

	UPROPERTY(EditAnywhere, Category = Brush, meta = (UIMin = "1", UIMax = "16", ClampMin = "1", ClampMax = "16"))
	int32 Flow = 1;
};

UCLASS()
class FLOWMAPPAINTEREDITOR_API UFlowMapDrawTool : public UMeshSurfacePointTool
{
	GENERATED_BODY()
public:
	UFlowMapDrawTool();

	virtual void Setup() override;
	virtual void Shutdown(EToolShutdownType ShutdownType) override;
	virtual void Render(IToolsContextRenderAPI* RenderAPI) override;

	virtual void OnBeginDrag(const FRay& Ray) override;
	virtual void OnEndDrag(const FRay& Ray) override;
	virtual void OnUpdateDrag(const FRay& Ray) override;
	virtual bool OnUpdateHover(const FInputDeviceRay& DevicePos) override;
	virtual void OnUpdateModifierState(int ModifierID, bool bIsOn) override;
	virtual bool HitTest(const FRay& Ray, FHitResult& OutHit) override;

protected:
	TWeakObjectPtr<UFlowMapDrawToolProperties> Settings;
	FVector Location = FVector::ZeroVector;
	FVector LastLocation = FVector::ZeroVector;
	FVector Normal = FVector::UpVector;

private:
	static constexpr int32 EraseModifierID = 1; // identifier we associate with the shift key
	static constexpr int32 SizeModifierID = 2; // identifier we associate with the ctrl key
	bool bEraseModifierDown = false; // flag we use to keep track of modifier state
	bool bSizeModifierDown = false; // flag we use to keep track of modifier state

	UPROPERTY(Transient)
	UMaterialInterface* ForceMaterial = nullptr;

	UPROPERTY(Transient)
	UMaterialInterface* CopyTextureMaterial = nullptr;

	UPROPERTY(Transient)
	UMaterialInstanceDynamic* ForceMID = nullptr;

	UPROPERTY(Transient)
	UTextureRenderTarget2D* FlowMapRT = nullptr;

	UPROPERTY(Transient)
	UTexture2D* OutputTexture = nullptr;

	FIntPoint TextureSize = 2048;
	FIntPoint WorldSize = 204800;
	FTransform Transform = FTransform::Identity;

	float GetDrawBrushSize() const;
	float GetDrawDensity() const;
	float GetEraseDensity() const;
	float GetHardness() const;

	static void DefaultApplyOrRemoveTextureOverride(
		UMeshComponent* InMeshComponent,
		UTexture* SourceTexture,
		UTexture* OverrideTexture);

	void DrawTexture(
		UObject* WorldContextObject,
		FVector ForceLocation,
		FVector ForceVelocity,
		float BrushSize,
		float Density,
		float Hardness,
		bool bErase,
		FLinearColor Channel);
};
