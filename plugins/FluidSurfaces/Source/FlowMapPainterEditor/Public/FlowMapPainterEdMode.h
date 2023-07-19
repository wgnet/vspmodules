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

#include "EdMode.h"

class UEdModeInteractiveToolsContext;

class FFlowMapPainterEdMode : public FEdMode
{
public:
	const static FEditorModeID EM_FlowMapPainterEdModeId;

public:
	FFlowMapPainterEdMode();
	virtual ~FFlowMapPainterEdMode();

	virtual void Enter() override;
	virtual void Exit() override;
	virtual void Tick(FEditorViewportClient* ViewportClient, float DeltaTime) override;
	virtual void Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI) override;
	virtual bool IsSelectionAllowed(AActor* InActor, bool bInSelection) const override;
	virtual bool UsesToolkits() const override;

	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;

	// these disable the standard gizmo, which is probably want we want in
	// these tools as we can't hit-test the standard gizmo...
	virtual bool AllowWidgetMove() override
	{
		return false;
	}
	virtual bool ShouldDrawWidget() const override
	{
		return false;
	}
	virtual bool UsesTransformWidget() const override
	{
		return false;
	}

	virtual bool ReceivedFocus(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool LostFocus(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;

	virtual bool MouseEnter(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;
	virtual bool MouseLeave(FEditorViewportClient* ViewportClient, FViewport* Viewport) override;
	virtual bool MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y) override;

	virtual bool InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
		override;
	virtual bool InputAxis(
		FEditorViewportClient* InViewportClient,
		FViewport* Viewport,
		int32 ControllerId,
		FKey Key,
		float Delta,
		float DeltaTime) override;
	virtual bool StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;
	virtual bool CapturedMouseMove(
		FEditorViewportClient* InViewportClient,
		FViewport* InViewport,
		int32 InMouseX,
		int32 InMouseY) override;
	virtual bool EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport) override;

	virtual UInteractiveToolManager* GetToolManager() const;

protected:
	UEdModeInteractiveToolsContext* ToolsContext;

	void DeselectAll() const;
};
