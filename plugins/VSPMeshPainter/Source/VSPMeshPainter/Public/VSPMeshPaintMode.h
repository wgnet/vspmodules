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
#include "Editor/Blutility/Classes/EditorUtilityWidget.h"
#include "Tools/UEdMode.h"
#include "VSPMeshPaintMode.generated.h"

class UVSPMeshPaintBrush;

class FVSPMeshPaintModeCommands : public TCommands<FVSPMeshPaintModeCommands>
{
public:
	FVSPMeshPaintModeCommands()
		: TCommands<FVSPMeshPaintModeCommands>(
			"VSPMeshPainting",
			NSLOCTEXT("MeshPaintEditorMode", "VSPMeshPaintingModeCommands", "VSP Mesh Paint"),
			NAME_None,
			FEditorStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> SelectMesh;
	TSharedPtr<FUICommandInfo> Paint;
	TSharedPtr<FUICommandInfo> BakeMask;
	TSharedPtr<FUICommandInfo> CommitChanges;
	TSharedPtr<FUICommandInfo> UndoLastSmear;
	TSharedPtr<FUICommandInfo> ApplyMaskDilation;
	TSharedPtr<FUICommandInfo> OpenUVPainter;
};

UCLASS()
class VSPMESHPAINTER_API UVSPMeshPaintMode : public UEdMode
{
	GENERATED_BODY()
public:
	UVSPMeshPaintMode();
	virtual void Enter() override;
	virtual void Exit() override;
	virtual void CreateToolkit() override;
	virtual TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> GetModeCommands() const override;

	virtual bool HandleClick(FEditorViewportClient* InViewportClient, HHitProxy* HitProxy, const FViewportClick& Click)
		override;
	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
		override;
	virtual bool InputDelta(
		FEditorViewportClient* InViewportClient,
		FViewport* InViewport,
		FVector& InDrag,
		FRotator& InRot,
		FVector& InScale) override;

private:
	void BindCommands();
	void UnbindCommands();
	UVSPMeshPaintBrush* GetActiveBrushTool() const;
};
