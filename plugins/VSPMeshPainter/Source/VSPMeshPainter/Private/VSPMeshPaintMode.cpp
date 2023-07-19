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
#include "VSPMeshPaintMode.h"

#include "AssetTypeActions_Base.h"
#include "EditorUtilitySubsystem.h"
#include "EditorUtilityWidget.h"
#include "EditorUtilityWidgetBlueprint.h"
#include "InLayerMaskBakeTool.h"
#include "MeshPaintingToolset/Public/MeshTexturePaintingTool.h"
#include "MeshSelect.h"
#include "PackageTools.h"
#include "VSPMeshPaintBrush.h"
#include "VSPMeshPaintToolkit.h"
#include "VSPMeshPaintUVWindow.h"
#include "VSPMeshPainterSettings.h"
#include "UnrealEd/Public/EditorModeManager.h"

#define LOCTEXT_NAMESPACE "VSPMeshPaintMode"

void FVSPMeshPaintModeCommands::RegisterCommands()
{
	UI_COMMAND(SelectMesh, "Select Mesh", "Select Mesh", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(Paint, "Paint", "Paint", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(BakeMask, "Bake Mask", "Bake Mask", EUserInterfaceActionType::ToggleButton, FInputChord());
	UI_COMMAND(
		CommitChanges,
		"Commit Changes",
		"Commit Changes",
		EUserInterfaceActionType::Button,
		FInputChord(EKeys::C, EModifierKey::Control | EModifierKey::Shift));
	UI_COMMAND(
		UndoLastSmear,
		"Undo Last Smear",
		"Undo last Smear",
		EUserInterfaceActionType::Button,
		FInputChord(EKeys::Z, EModifierKey::Control))
	UI_COMMAND(
		ApplyMaskDilation,
		"Apply mask dilation",
		"Apply mask dilation",
		EUserInterfaceActionType::Button,
		FInputChord(EKeys::D, EModifierKey::Control))
	UI_COMMAND(
		OpenUVPainter,
		"OpenUVPainter",
		"OpenUVPainter",
		EUserInterfaceActionType::Button,
		FInputChord(EKeys::NumPadZero, EModifierKey::Control))
}

UVSPMeshPaintMode::UVSPMeshPaintMode()
{
	SettingsClass = UVSPMeshPainterModeDummySettings::StaticClass();
	ToolsContextClass = UMeshToolsContext::StaticClass();
	Info = FEditorModeInfo(
		FName(TEXT("VSPMeshPainting")),
		LOCTEXT("ModeName", "VSP Mesh Paint"),
		FSlateIcon(FEditorStyle::GetStyleSetName(), "LevelEditor.MeshPaintMode", "LevelEditor.MeshPaintMode.Small"),
		true,
		600);

	FVSPMeshPaintModeCommands::Register();
}

void UVSPMeshPaintMode::Enter()
{
	Super::Enter();

	BindCommands();

	RegisterTool({ FVSPMeshPaintModeCommands::Get().Paint }, TEXT("VSPBrushTool"), NewObject<UVSPMeshPaintBrushBuilder>());
	RegisterTool(
		{ FVSPMeshPaintModeCommands::Get().SelectMesh },
		TEXT("MeshSelectionTool"),
		NewObject<UTextureAdapterClickToolBuilder>());
	RegisterTool(
		{ FVSPMeshPaintModeCommands::Get().BakeMask },
		TEXT("Bake Mask"),
		NewObject<UInLayerMaskBakeToolBuilder>());
	ToolsContext->StartTool(TEXT("MeshSelectionTool"));
}

void UVSPMeshPaintMode::Exit()
{
	UnbindCommands();

	Super::Exit();
}

void UVSPMeshPaintMode::CreateToolkit()
{
	if (!Toolkit.IsValid())
	{
		FVSPMeshPaintToolkit* PaintToolkit = new FVSPMeshPaintToolkit();
		Toolkit = MakeShareable(PaintToolkit);
		Toolkit->Init(Owner->GetToolkitHost());
	}

	Super::CreateToolkit();
}

TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> UVSPMeshPaintMode::GetModeCommands() const
{
	TMap<FName, TArray<TSharedPtr<FUICommandInfo>>> Map;
	Map.Add(
		TEXT("VSPMeshPainting"),
		{ FVSPMeshPaintModeCommands::Get().SelectMesh,
		  FVSPMeshPaintModeCommands::Get().Paint,
		  FVSPMeshPaintModeCommands::Get().BakeMask,
		  FVSPMeshPaintModeCommands::Get().CommitChanges,
		  FVSPMeshPaintModeCommands::Get().UndoLastSmear,
		  FVSPMeshPaintModeCommands::Get().ApplyMaskDilation,
		  FVSPMeshPaintModeCommands::Get().OpenUVPainter });

	return MoveTemp(Map);
}

bool UVSPMeshPaintMode::HandleClick(
	FEditorViewportClient* InViewportClient,
	HHitProxy* HitProxy,
	const FViewportClick& Click)
{
	return GetToolManager()->GetActiveToolName(EToolSide::Mouse) == TEXT("VSPBrushTool");
}

bool UVSPMeshPaintMode::InputKey(FEditorViewportClient* ViewportClient, FViewport* Viewport, FKey Key, EInputEvent Event)
{
	bool bHadled = false;
	UVSPMeshPaintBrush* Tool = GetActiveBrushTool();
	if (Tool)
	{
		bHadled = Tool->InputKey(ViewportClient, Viewport, Key, Event);
	}

	return bHadled ? true : Super::InputKey(ViewportClient, Viewport, Key, Event);
}

bool UVSPMeshPaintMode::InputDelta(
	FEditorViewportClient* InViewportClient,
	FViewport* InViewport,
	FVector& InDrag,
	FRotator& InRot,
	FVector& InScale)
{
	UVSPMeshPaintBrush* Tool = GetActiveBrushTool();
	return Tool ? Tool->InputDelta(InViewportClient, InViewport, InDrag, InRot, InScale) : false;
}

void UVSPMeshPaintMode::BindCommands()
{
	const TSharedRef<FUICommandList>& CommandList = Toolkit->GetToolkitCommands();
	CommandList->MapAction(
		FVSPMeshPaintModeCommands::Get().CommitChanges,
		FExecuteAction::CreateLambda(
			[this]()
			{
				auto Brush = CastChecked<UVSPMeshPaintBrush>(GetToolManager()->GetActiveTool(EToolSide::Mouse));
				Brush->CommitChanges();
			}),
		FCanExecuteAction::CreateLambda(
			[this]()
			{
				return GetActiveBrushTool() != nullptr;
			}));

	CommandList->MapAction(
		FVSPMeshPaintModeCommands::Get().UndoLastSmear,
		FExecuteAction::CreateLambda(
			[this]()
			{
				auto Brush = CastChecked<UVSPMeshPaintBrush>(GetToolManager()->GetActiveTool(EToolSide::Mouse));
				Brush->UndoLastSmear();
			}),
		FCanExecuteAction::CreateLambda(
			[this]()
			{
				return GetActiveBrushTool() != nullptr;
			}));

	CommandList->MapAction(
		FVSPMeshPaintModeCommands::Get().ApplyMaskDilation,
		FExecuteAction::CreateLambda(
			[this]()
			{
				auto Brush = CastChecked<UVSPMeshPaintBrush>(GetToolManager()->GetActiveTool(EToolSide::Mouse));
				Brush->ApplyMaskDilation();
			}),
		FCanExecuteAction::CreateLambda(
			[this]()
			{
				return GetActiveBrushTool() != nullptr;
			}));

	CommandList->MapAction(
		FVSPMeshPaintModeCommands::Get().OpenUVPainter,
		FExecuteAction::CreateLambda(
			[this]()
			{
				UVSPMeshPainterSettings* UVSPMeshPainterSettings = UVSPMeshPainterSettings::Get();
				if (UVSPMeshPainterSettings == nullptr)
					return;
				UEditorUtilityWidgetBlueprint* WidgetBlueprint = UVSPMeshPainterSettings->GetUVWindowWidgetBlueprint();

				if (WidgetBlueprint != nullptr
					&& WidgetBlueprint->GeneratedClass->IsChildOf(UEditorUtilityWidget::StaticClass()))
				{
					UEditorUtilitySubsystem* EditorUtilitySubsystem =
						GEditor->GetEditorSubsystem<UEditorUtilitySubsystem>();
					UVSPMeshPaintUVWindow* SpawnedEditorUtilityWidget =
						Cast<UVSPMeshPaintUVWindow>(EditorUtilitySubsystem->SpawnAndRegisterTab(WidgetBlueprint));

					FName RegistrationName = WidgetBlueprint->GetRegistrationName();

					SpawnedEditorUtilityWidget->MeshPaintBrush = GetActiveBrushTool();
					;
					SpawnedEditorUtilityWidget->Setup(RegistrationName);
				}
			}),
		FCanExecuteAction::CreateLambda(
			[this]()
			{
				return GetActiveBrushTool() != nullptr;
			}));
}

void UVSPMeshPaintMode::UnbindCommands()
{
	const TSharedRef<FUICommandList>& CommandList = Toolkit->GetToolkitCommands();
	CommandList->UnmapAction(FVSPMeshPaintModeCommands::Get().Paint);
	CommandList->UnmapAction(FVSPMeshPaintModeCommands::Get().BakeMask);
	CommandList->UnmapAction(FVSPMeshPaintModeCommands::Get().SelectMesh);
	CommandList->UnmapAction(FVSPMeshPaintModeCommands::Get().CommitChanges);
	CommandList->UnmapAction(FVSPMeshPaintModeCommands::Get().UndoLastSmear);
	CommandList->UnmapAction(FVSPMeshPaintModeCommands::Get().ApplyMaskDilation);
	CommandList->UnmapAction(FVSPMeshPaintModeCommands::Get().OpenUVPainter);
}

UVSPMeshPaintBrush* UVSPMeshPaintMode::GetActiveBrushTool() const
{
	return Cast<UVSPMeshPaintBrush>(GetToolManager()->GetActiveTool(EToolSide::Mouse));
}
