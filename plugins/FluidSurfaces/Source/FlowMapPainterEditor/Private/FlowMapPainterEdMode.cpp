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

#include "FlowMapPainterEdMode.h"

#include "ActorGroupingUtils.h"
#include "Editor/GroupActor.h"
#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "Experimental/EditorInteractiveToolsFramework/Public/EdModeInteractiveToolsContext.h"
#include "FlowMapDrawTool.h"
#include "FlowMapPainterEdModeToolkit.h"
#include "Kismet2/ComponentEditorUtils.h"
#include "Toolkits/ToolkitManager.h"

const FEditorModeID FFlowMapPainterEdMode::EM_FlowMapPainterEdModeId = TEXT("EM_FlowMapPainterEdMode");

FFlowMapPainterEdMode::FFlowMapPainterEdMode()
{
	ToolsContext = nullptr;
}

FFlowMapPainterEdMode::~FFlowMapPainterEdMode()
{
	// this should have happend already in ::Exit()
	if (ToolsContext != nullptr)
	{
		ToolsContext->ShutdownContext();
		ToolsContext = nullptr;
	}
}

void FFlowMapPainterEdMode::Enter()
{
	FEdMode::Enter();

	if (!Toolkit.IsValid() && UsesToolkits())
	{
		Toolkit = MakeShareable(new FFlowMapPainterEdModeToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}

	// initialize the adapter that attaches the ToolsContext to this FEdMode
	ToolsContext = NewObject<UEdModeInteractiveToolsContext>(GetTransientPackage(), TEXT("ToolsContext"), RF_Transient);
	ToolsContext->InitializeContextFromEdMode(this);

	UFlowMapDrawToolBuilder* FlowMapDrawToolBuilder = NewObject<UFlowMapDrawToolBuilder>();
	ToolsContext->ToolManager->RegisterToolType(TEXT("FlowMapDrawTool"), FlowMapDrawToolBuilder);

	// active tool type is not relevant here, we just set to default
	ToolsContext->ToolManager->SelectActiveToolType(EToolSide::Left, TEXT("FlowMapDrawTool"));
}

void FFlowMapPainterEdMode::Exit()
{
	// shutdown and clean up the ToolsContext
	ToolsContext->ShutdownContext();
	ToolsContext = nullptr;

	if (Toolkit.IsValid())
	{
		FToolkitManager::Get().CloseToolkit(Toolkit.ToSharedRef());
		Toolkit.Reset();
	}

	// Call base Exit method to ensure proper cleanup
	FEdMode::Exit();
}

void FFlowMapPainterEdMode::Tick(FEditorViewportClient* ViewportClient, float DeltaTime)
{
	FEdMode::Tick(ViewportClient, DeltaTime);

	if (ToolsContext != nullptr)
	{
		ToolsContext->Tick(ViewportClient, DeltaTime);
	}
}

void FFlowMapPainterEdMode::Render(const FSceneView* View, FViewport* Viewport, FPrimitiveDrawInterface* PDI)
{
	FEdMode::Render(View, Viewport, PDI);

	if (ToolsContext != nullptr)
	{
		ToolsContext->Render(View, Viewport, PDI);
	}
}

bool FFlowMapPainterEdMode::IsSelectionAllowed(AActor* InActor, bool bInSelection) const
{
	return true;
}

bool FFlowMapPainterEdMode::UsesToolkits() const
{
	return true;
}

void FFlowMapPainterEdMode::AddReferencedObjects(FReferenceCollector& Collector)
{
	Collector.AddReferencedObject(ToolsContext);
}

bool FFlowMapPainterEdMode::ReceivedFocus(FEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	return FEdMode::ReceivedFocus(ViewportClient, Viewport);
}

bool FFlowMapPainterEdMode::LostFocus(FEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	return FEdMode::LostFocus(ViewportClient, Viewport);
}

bool FFlowMapPainterEdMode::MouseEnter(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y)
{
	bool bHandled = ToolsContext->MouseEnter(ViewportClient, Viewport, x, y);
	return bHandled;
}

bool FFlowMapPainterEdMode::MouseLeave(FEditorViewportClient* ViewportClient, FViewport* Viewport)
{
	bool bHandled = ToolsContext->MouseLeave(ViewportClient, Viewport);
	return bHandled;
}

bool FFlowMapPainterEdMode::MouseMove(FEditorViewportClient* ViewportClient, FViewport* Viewport, int32 x, int32 y)
{
	bool bHandled = ToolsContext->MouseMove(ViewportClient, Viewport, x, y);
	return bHandled;
}

bool FFlowMapPainterEdMode::InputKey(
	FEditorViewportClient* ViewportClient,
	FViewport* Viewport,
	FKey Key,
	EInputEvent Event)
{
	bool bHandled = FEdMode::InputKey(ViewportClient, Viewport, Key, Event);
	bHandled |= ToolsContext->InputKey(ViewportClient, Viewport, Key, Event);
	return bHandled;
}

bool FFlowMapPainterEdMode::InputAxis(
	FEditorViewportClient* InViewportClient,
	FViewport* Viewport,
	int32 ControllerId,
	FKey Key,
	float Delta,
	float DeltaTime)
{
	return FEdMode::InputAxis(InViewportClient, Viewport, ControllerId, Key, Delta, DeltaTime);
}

bool FFlowMapPainterEdMode::StartTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	bool bHandled = FEdMode::StartTracking(InViewportClient, InViewport);
	bHandled |= ToolsContext->StartTracking(InViewportClient, InViewport);
	return bHandled;
}

bool FFlowMapPainterEdMode::CapturedMouseMove(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	int32 InMouseX,
	int32 InMouseY)
{
	bool bHandled = ToolsContext->CapturedMouseMove(InViewportClient, InViewport, InMouseX, InMouseY);
	return bHandled;
}

bool FFlowMapPainterEdMode::EndTracking(FEditorViewportClient* InViewportClient, FViewport* InViewport)
{
	bool bHandled = ToolsContext->EndTracking(InViewportClient, InViewport);
	return bHandled;
}

UInteractiveToolManager* FFlowMapPainterEdMode::GetToolManager() const
{
	return ToolsContext->ToolManager;
}

void FFlowMapPainterEdMode::DeselectAll() const
{
	TArray<AActor*> ActorsToDeselect;
	for (FSelectionIterator It(GEditor->GetSelectedActorIterator()); It; ++It)
	{
		AActor* Actor = static_cast<AActor*>(*It);
		checkSlow(Actor->IsA(AActor::StaticClass()));

		ActorsToDeselect.Add(Actor);
	}
	for (int32 ActorIndex = 0; ActorIndex < ActorsToDeselect.Num(); ++ActorIndex)
	{
		AActor* Actor = ActorsToDeselect[ActorIndex];
		if (UActorGroupingUtils::IsGroupingActive())
		{
			AGroupActor* SelectedGroupActor = Cast<AGroupActor>(Actor);
			if (SelectedGroupActor)
			{
				GEditor->SelectGroup(SelectedGroupActor, true, false, false);
			}
			else
			{
				AGroupActor* ActorLockedRootGroup = AGroupActor::GetRootForActor(Actor, true);
				if (ActorLockedRootGroup)
				{
					GEditor->SelectGroup(ActorLockedRootGroup, false, false, false);
				}
			}
		}
		const bool bActorSelected = Actor->IsSelected();
		if (bActorSelected)
		{
			GEditor->GetSelectedActors()->Select(Actor, false);
			{
				if (GEditor->GetSelectedComponentCount() > 0)
				{
					GEditor->GetSelectedComponents()->Modify();
				}

				GEditor->GetSelectedComponents()->BeginBatchSelectOperation();
				for (UActorComponent* Component : Actor->GetComponents())
				{
					if (Component)
					{
						GEditor->GetSelectedComponents()->Deselect(Component);
						if (USceneComponent* SceneComponent = Cast<USceneComponent>(Component))
						{
							FComponentEditorUtils::BindComponentSelectionOverride(SceneComponent, false);
						}
					}
				}
				GEditor->GetSelectedComponents()->EndBatchSelectOperation(false);
			}
		}
	}
}
