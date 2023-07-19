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

#include "FlowMapPainterEdModeToolkit.h"
#include "EditorModeManager.h"
#include "Engine/Selection.h"
#include "FlowMapPainterEdMode.h"
#include "InteractiveToolManager.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "FFlowMapPainterEdModeToolkit"

FFlowMapPainterEdModeToolkit::FFlowMapPainterEdModeToolkit()
{
}

FFlowMapPainterEdMode* FFlowMapPainterEdModeToolkit::GetToolsEditorMode() const
{
	return (FFlowMapPainterEdMode*)GetEditorMode();
}

void FFlowMapPainterEdModeToolkit::Init(const TSharedPtr<IToolkitHost>& InitToolkitHost)
{
	FPropertyEditorModule& PropertyEditorModule =
		FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");

	FDetailsViewArgs
		DetailsViewArgs(false, false, false, FDetailsViewArgs::HideNameArea, true, nullptr, false, NAME_None);

	DetailsViewArgs.DefaultsOnlyVisibility = EEditDefaultsOnlyNodeVisibility::Automatic;
	DetailsViewArgs.bShowOptions = false;
	DetailsViewArgs.bAllowMultipleTopLevelObjects = true;

	DetailsView = PropertyEditorModule.CreateDetailView(DetailsViewArgs);

	SAssignNew( ToolkitWidget, SBorder )
		.HAlign( HAlign_Fill )
		.Padding( 25 )
		.VAlign( VAlign_Fill )
		[
			SNew( SVerticalBox )

			+ SVerticalBox::Slot()
			.AutoHeight()
			.HAlign( HAlign_Fill )
			.Padding( 50 )
			[
				SNew( SVerticalBox )
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( STextBlock )
					.AutoWrapText( true )
					.Text( LOCTEXT("hotkey1", "Paint : LMB + Drag" ) )
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( STextBlock )
					.AutoWrapText( true )
					.Text( LOCTEXT("hotkey2", "Erase : Shift + LMB + Drag" ) )
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				[
					SNew( STextBlock )
					.AutoWrapText( true )
					.Text( LOCTEXT("hotkey3", "Size : Ctrl + LMB + Drag" ) )
				]
			]

			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.AutoHeight()
			[
				SNew(SButton).Text(LOCTEXT("DrawFlowMapToolLabel", "Start Draw"))
				.OnClicked_Lambda([this]() { return this->StartTool(TEXT("FlowMapDrawTool")); })
				.IsEnabled_Lambda([this]() { return this->CanStartTool(TEXT("FlowMapDrawTool")); })
				.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
				.ForegroundColor(FLinearColor::White)
				.ContentPadding(FMargin(6, 2))
				.HAlign( HAlign_Center )
			]

			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.AutoHeight()
			[
				SNew(SButton).Text(LOCTEXT("CompletedToolButtonLabel", "Complete"))
				.OnClicked_Lambda([this]() { return this->EndTool(EToolShutdownType::Completed); })
				.IsEnabled_Lambda([this]() { return this->CanCompleteActiveTool(); })
				.ButtonStyle(FEditorStyle::Get(), "FlatButton.Success")
				.ForegroundColor(FLinearColor::White)
				.ContentPadding(FMargin(6, 2))
				.HAlign( HAlign_Center )
			]
			
			+ SVerticalBox::Slot()
			.HAlign(HAlign_Fill)
			.AutoHeight()
			[
				DetailsView->AsShared()
			]

			+ SVerticalBox::Slot()
			[
				SNew(SSpacer)
				.Size(FVector2D(500.0f, 1.0f))
			]
	
		];

	FModeToolkit::Init(InitToolkitHost);
}

FName FFlowMapPainterEdModeToolkit::GetToolkitFName() const
{
	return FName("FlowMapPainterEdMode");
}

FText FFlowMapPainterEdModeToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("FlowMapPainterEdModeToolkit", "DisplayName", "FlowMapPainterEdMode Tool");
}

bool FFlowMapPainterEdModeToolkit::CanStartTool(const FString& ToolTypeIdentifier) const
{
	UInteractiveToolManager* Manager = GetToolsEditorMode()->GetToolManager();

	return (Manager->HasActiveTool(EToolSide::Left) == false)
		&& (Manager->CanActivateTool(EToolSide::Left, ToolTypeIdentifier) == true);
}

bool FFlowMapPainterEdModeToolkit::CanAcceptActiveTool() const
{
	return GetToolsEditorMode()->GetToolManager()->CanAcceptActiveTool(EToolSide::Left);
}

bool FFlowMapPainterEdModeToolkit::CanCancelActiveTool() const
{
	return GetToolsEditorMode()->GetToolManager()->CanCancelActiveTool(EToolSide::Left);
}

bool FFlowMapPainterEdModeToolkit::CanCompleteActiveTool() const
{
	return GetToolsEditorMode()->GetToolManager()->HasActiveTool(EToolSide::Left) && CanCancelActiveTool() == false;
}

FReply FFlowMapPainterEdModeToolkit::StartTool(const FString& ToolTypeIdentifier) const
{
	if (GetToolsEditorMode()->GetToolManager()->SelectActiveToolType(EToolSide::Left, ToolTypeIdentifier) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ToolManager: Unknown Tool Type %s"), *ToolTypeIdentifier);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ToolManager: Starting Tool Type %s"), *ToolTypeIdentifier);
		GetToolsEditorMode()->GetToolManager()->ActivateTool(EToolSide::Left);

		// Update properties panel
		const UInteractiveTool* InteractiveTool =
			GetToolsEditorMode()->GetToolManager()->GetActiveTool(EToolSide::Left);
		DetailsView->SetObjects(InteractiveTool->GetToolProperties());
	}
	return FReply::Handled();
}

FReply FFlowMapPainterEdModeToolkit::EndTool(EToolShutdownType ShutdownType) const
{
	UE_LOG(LogTemp, Warning, TEXT("ENDING TOOL"));
	GetToolsEditorMode()->GetToolManager()->DeactivateTool(EToolSide::Left, ShutdownType);
	DetailsView->SetObject(nullptr);
	return FReply::Handled();
}

FEdMode* FFlowMapPainterEdModeToolkit::GetEditorMode() const
{
	return GLevelEditorModeTools().GetActiveMode(FFlowMapPainterEdMode::EM_FlowMapPainterEdModeId);
}

#undef LOCTEXT_NAMESPACE
