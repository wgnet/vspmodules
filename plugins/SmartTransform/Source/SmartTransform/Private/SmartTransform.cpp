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

#include "SmartTransform.h"
#include "SSTViewportToolBarComboMenu.h"
#include "STCommands.h"
#include "STEdMode.h"

#include "AppFramework/Public/Widgets/Colors/SColorPicker.h"
#include "EditorModeManager.h"
#include "LevelEditor.h"
#include "Widgets/Input/SNumericEntryBox.h"

#define LOCTEXT_NAMESPACE "FSmartTransformModule"

void FSmartTransformModule::StartupModule()
{
	FSTStyle::Initialize();
	FSTStyle::ReloadTextures();

	FSTCommands::Register();

	SmartTransformActions = MakeShareable(new FUICommandList);
	SmartTransformActions->MapAction(
		FSTCommands::Get().SmartTransformToggle,
		FExecuteAction::CreateRaw(this, &FSmartTransformModule::SmartTransformToggle_ExecuteAction));

	FEditorModeRegistry::Get().RegisterMode<FSTEdMode>(
		FSTEdMode::EM_STEdModeId,
		LOCTEXT("SmartTransformEdMode", "SmartTransformEdMode"),
		FSlateIcon(),
		false);

	FLevelEditorModule& LevelEditorModule = FModuleManager::LoadModuleChecked<FLevelEditorModule>("LevelEditor");
	LevelEditorModule.GetGlobalLevelEditorActions()->Append(SmartTransformActions.ToSharedRef());

	{
		TSharedPtr<FExtender> ToolbarExtender = MakeShareable(new FExtender);
		ToolbarExtender->AddToolBarExtension(
			"LocalToWorld",
			EExtensionHook::After,
			SmartTransformActions,
			FToolBarExtensionDelegate::CreateRaw(this, &FSmartTransformModule::CreateToolbarExtension));

		LevelEditorModule.GetToolBarExtensibilityManager()->AddExtender(ToolbarExtender);
	}
}

void FSmartTransformModule::ShutdownModule()
{
	FEditorModeRegistry::Get().UnregisterMode(FSTEdMode::EM_STEdModeId);
}

void FSmartTransformModule::CreateToolbarExtension(FToolBarBuilder& InToolbarBuilder)
{
	InToolbarBuilder.BeginSection("SmartTransform");
	const FName ToolBarStyle = "ViewportMenu";

	InToolbarBuilder.AddWidget(SNew(SSTViewportToolBarComboMenu)
		.Cursor(EMouseCursor::Default)
		.Style(ToolBarStyle)
		.OnCheckStateChanged_Raw(this, &FSmartTransformModule::SmartTransformToggle_OnCheckStateChanged)
		.IsChecked_Raw(this, &FSmartTransformModule::GetSmartTransformState_IsChecked)
		.OnGetMenuContent_Raw(this, &FSmartTransformModule::FillSettingsMenu_OnGetMenuContent)
		.Icon(FSTCommands::Get().SmartTransformToggle->GetIcon())
		.ToggleButtonToolTip(FSTCommands::Get().SmartTransformToggle->GetDescription())
	);

	InToolbarBuilder.EndSection();
}

TSharedRef<SWidget> FSmartTransformModule::FillSettingsMenu_OnGetMenuContent()
{
	FMenuBuilder SettingsMenuBuilder(true, SmartTransformActions);
	SettingsMenuBuilder.BeginSection("Vertex", LOCTEXT("Vertex", "Vertex settings"));

	SettingsMenuBuilder.AddWidget(SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(2.0f, 2.0f, 60.0f, 2.0f))
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("PointSize", "Point size for vertices"))
		]
	+ SVerticalBox::Slot()
		.Padding(FMargin(4.0f, 4.0f))
		.AutoHeight()
		[
			SNew(SNumericEntryBox<float>)
			.Value(
			TAttribute<TOptional<float>>::Create(TAttribute<TOptional<float>>::FGetter::CreateStatic([] {
				const FSTEdMode* STEdMode = GetSmartTransformMode();
				return TOptional<float>(STEdMode->SmartTransformSettings.VertexSize);	}))
		)
		.OnValueChanged(
			SNumericEntryBox<float>::FOnValueChanged::CreateStatic([](float Val) {
				FSTEdMode* STEdMode = GetSmartTransformMode();
				STEdMode->SmartTransformSettings.VertexSize = Val;
		}))
			.MinValue(10.f)
			.MaxValue(30.f)
			.MinSliderValue(10.f)
			.MaxSliderValue(30.f)
			.AllowSpin(true)
		]
	+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(2.0f, 2.0f, 60.0f, 2.0f))
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("VertexCommonColor", "Color"))
		]
	+ SVerticalBox::Slot()
		.Padding(FMargin(4.0f, 4.0f))
		.AutoHeight()
		[
			SNew(SColorBlock)
			.Color_Raw(this, &FSmartTransformModule::GetColorFromSettings_Color, FSTColorNames::VertexColorName)
			.OnMouseButtonDown_Raw(this, &FSmartTransformModule::OpenColorPicker_OnMouseButtonDown, FSTColorNames::VertexColorName)
		]
	,FText::GetEmpty()
	);
	SettingsMenuBuilder.EndSection();

	SettingsMenuBuilder.BeginSection("LineForSnapping", LOCTEXT("LineForSnapping", "Line for snapping settings"));
	SettingsMenuBuilder.AddWidget(SNew(SVerticalBox)
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(2.0f, 2.0f, 60.0f, 2.0f))
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("LineThickness", "Thickness"))
		]
	+ SVerticalBox::Slot()
		.Padding(FMargin(4.0f, 4.0f))
		.AutoHeight()
		[
			SNew(SNumericEntryBox<float>)
			.Value(
				TAttribute<TOptional<float>>::Create(TAttribute<TOptional<float>>::FGetter::CreateStatic([] {
		const FSTEdMode* STEdMode = GetSmartTransformMode();
		return TOptional<float>(STEdMode->SmartTransformSettings.LineForSnappingThickness);}))
			)
		.OnValueChanged(
			SNumericEntryBox<float>::FOnValueChanged::CreateStatic([](float Val) {
			FSTEdMode* STEdMode = GetSmartTransformMode();
			STEdMode->SmartTransformSettings.LineForSnappingThickness = Val;
		}))
			.MinValue(1.0f)
			.MaxValue(5.0f)
			.MinSliderValue(1.0f)
			.MaxSliderValue(5.0f)
			.AllowSpin(true)
		]
		+ SVerticalBox::Slot()
		.AutoHeight()
		.Padding(FMargin(2.0f, 2.0f, 60.0f, 2.0f))
		.HAlign(HAlign_Left)
		[
			SNew(STextBlock)
			.Text(LOCTEXT("LineColorForSnapping", "Color"))
		]
	+ SVerticalBox::Slot()
		.Padding(FMargin(4.0f, 4.0f))
		.AutoHeight()
		[
			SNew(SColorBlock)
			.Color_Raw(this, &FSmartTransformModule::GetColorFromSettings_Color, FSTColorNames::LineColorForSnappingName)
		.OnMouseButtonDown_Raw(this, &FSmartTransformModule::OpenColorPicker_OnMouseButtonDown, FSTColorNames::LineColorForSnappingName)
		]
	, FText::GetEmpty()
	);
	SettingsMenuBuilder.EndSection();

	SettingsMenuBuilder.BeginSection("Snapping", LOCTEXT("Snapping", "Snapping settings"));
	SettingsMenuBuilder.AddWidget(SNew(SHorizontalBox)
		+ SHorizontalBox::Slot()
		.HAlign(HAlign_Fill)
		[
			SNew(STextBlock)
		.Text(LOCTEXT("2D snapping checkbox", "2D Snapping"))
		]
	+ SHorizontalBox::Slot()
		.HAlign(HAlign_Right)
		[
			SNew(SCheckBox)
			.IsChecked_Raw(this, &FSmartTransformModule::Get2DSnappingState_IsChecked)
			.OnCheckStateChanged_Raw(this, &FSmartTransformModule::Snapping2DToggle_OnCheckStateChanged)
		],FText::GetEmpty());

	SettingsMenuBuilder.EndSection();
	return SettingsMenuBuilder.MakeWidget();
}

void FSmartTransformModule::SmartTransformToggle_OnCheckStateChanged(ECheckBoxState InState)
{
	if (InState == ECheckBoxState::Unchecked)
	{
		GLevelEditorModeTools().DeactivateMode(FSTEdMode::EM_STEdModeId);
		return;
	}

	if (InState == ECheckBoxState::Checked)
		GLevelEditorModeTools().ActivateMode(FSTEdMode::EM_STEdModeId);
}

void FSmartTransformModule::SmartTransformToggle_ExecuteAction()
{
	if (GetSmartTransformState_IsChecked() == ECheckBoxState::Unchecked)
	{
		GLevelEditorModeTools().ActivateMode(FSTEdMode::EM_STEdModeId);
		return;
	}

	if (GetSmartTransformState_IsChecked() == ECheckBoxState::Checked)
		GLevelEditorModeTools().DeactivateMode(FSTEdMode::EM_STEdModeId);
}

ECheckBoxState FSmartTransformModule::GetSmartTransformState_IsChecked() const
{
	const ECheckBoxState OutState = GLevelEditorModeTools().IsModeActive(FSTEdMode::EM_STEdModeId)
		? ECheckBoxState::Checked
		: ECheckBoxState::Unchecked;
	return OutState;
}

FSTEdMode* FSmartTransformModule::GetSmartTransformMode()
{
	return static_cast<FSTEdMode*>(GLevelEditorModeTools().GetActiveMode(FSTEdMode::EM_STEdModeId));
}

void FSmartTransformModule::SetColorByName(FLinearColor InNewColor, FName InName) const
{
	if (FSTEdMode* STEdMode = GetSmartTransformMode())
		STEdMode->SmartTransformSettings.GetColorReferenceByName(InName) = InNewColor;
}

ECheckBoxState FSmartTransformModule::Get2DSnappingState_IsChecked() const
{
	ECheckBoxState OutState = ECheckBoxState::Undetermined;
	GetSmartTransformMode()->bIs2DSnapping ? OutState = ECheckBoxState::Checked : OutState = ECheckBoxState::Unchecked;
	return OutState;
}

void FSmartTransformModule::Snapping2DToggle_OnCheckStateChanged(ECheckBoxState InState)
{
	if (InState == ECheckBoxState::Unchecked)
	{
		GetSmartTransformMode()->bIs2DSnapping = false;
		return;
	}

	if (InState == ECheckBoxState::Checked)
		GetSmartTransformMode()->bIs2DSnapping = true;
}

FLinearColor FSmartTransformModule::GetColorFromSettings_Color(FName InName) const
{
	if (FSTEdMode* STEdMode = GetSmartTransformMode())
		return STEdMode->SmartTransformSettings.GetColorReferenceByName(InName);

	return FLinearColor::Black;
}


FReply FSmartTransformModule::OpenColorPicker_OnMouseButtonDown(
	const FGeometry& MyGeometry,
	const FPointerEvent& MouseEvent,
	FName InName) const
{
	TSharedRef<SWindow> NewSlateWindow = FSlateApplication::Get().AddWindow(
		SNew(SWindow)
		.Title(LOCTEXT("ColorPicker", "Select Color"))
		.ClientSize(SColorPicker::DEFAULT_WINDOW_SIZE)
	);

	TSharedPtr<SColorPicker> ColorPicker = SNew(SColorPicker)
		.UseAlpha(false)
		.ParentWindow(NewSlateWindow)
		.TargetColorAttribute_Raw(this, &FSmartTransformModule::GetColorFromSettings_Color, InName)
		.OnColorCommitted_Raw(this, &FSmartTransformModule::SetColorByName, InName)
		.ExpandAdvancedSection(true);

	NewSlateWindow->SetContent(ColorPicker.ToSharedRef());

	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FSmartTransformModule, SmartTransform)