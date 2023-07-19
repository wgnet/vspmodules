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
#include "SSTViewportToolBarComboMenu.h"
#include "STEdMode.h"

#include "EditorModeManager.h"

void SSTViewportToolBarComboMenu::Construct(const FArguments& InArgs)
{
	const FName ButtonStyle = FEditorStyle::Join(InArgs._Style.Get(), ".Button");
	const FName CheckboxStyle = FEditorStyle::Join(InArgs._Style.Get(), ".ToggleButton");

	const FSlateIcon& Icon = InArgs._Icon.Get();

	TSharedPtr<SCheckBox> ToggleControl;
	{
		ToggleControl = SNew(SCheckBox)
			.Cursor(EMouseCursor::Default)
			.Padding(FMargin(4.0f))
			.Style(FEditorStyle::Get(), ToName(CheckboxStyle, InArgs._BlockLocation))
			.OnCheckStateChanged(InArgs._OnCheckStateChanged)
			.ToolTipText(InArgs._ToggleButtonToolTip)
			.IsChecked(InArgs._IsChecked)
			.Content()
			[
				SNew(SBox)
				.WidthOverride(16)
				.HeightOverride(16)
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Center)
				[
					SNew(SImage)
					.Image(Icon.GetIcon())
				]
			];
	}

	{
		TSharedRef<SWidget> ButtonContents =
			SNew(SButton)
			.ButtonStyle(FEditorStyle::Get(), ToName(ButtonStyle, EMultiBlockLocation::End))
			.ContentPadding(FMargin(5.0f, 0.0f))
			.ToolTipText(InArgs._MenuButtonToolTip)
			.OnClicked(this, &SSTViewportToolBarComboMenu::ChangeMenuAnchor_OnMenuClicked)
			.IsEnabled_Raw(this, &SSTViewportToolBarComboMenu::IsSmartTransformActive_IsEnabled)
			[
				SNew(SVerticalBox)
				+ SVerticalBox::Slot()
				.AutoHeight()
				.HAlign(HAlign_Center)
				.VAlign(VAlign_Top)
				[
					SNew(STextBlock)
					.TextStyle(FEditorStyle::Get(), FEditorStyle::Join(InArgs._Style.Get(), ".Label"))
					.Text(InArgs._Label)
				]
				+ SVerticalBox::Slot()
				.AutoHeight()
				.VAlign(VAlign_Bottom)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)

					+ SHorizontalBox::Slot()
					.AutoWidth()
					[
						SNew(SBox)
						.WidthOverride(4)
						.HeightOverride(4)
						[
							SNew(SImage)
							.Image(FEditorStyle::GetBrush("ComboButton.Arrow"))
							.ColorAndOpacity(FLinearColor::Black)
						]
					]

					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
				]
			];

		if (InArgs._MinDesiredButtonWidth > 0.0f)
		{
			ButtonContents =
				SNew(SBox)
				.MinDesiredWidth(InArgs._MinDesiredButtonWidth)
				[
					ButtonContents
				];
		}

		MenuAnchor = SNew(SMenuAnchor)

			.Placement(MenuPlacement_BelowRightAnchor)
			[
				ButtonContents
			]
			.OnGetMenuContent(InArgs._OnGetMenuContent);
	}

	ChildSlot
	[
		SNew(SHorizontalBox)

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			ToggleControl->AsShared()
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew(SBorder)
			.Padding(FMargin(1.0f, 0.0f, 0.0f, 0.0f))
			.BorderImage(FEditorStyle::GetDefaultBrush())
			.BorderBackgroundColor(FLinearColor::Black)
		]

		+ SHorizontalBox::Slot()
		.AutoWidth()

		[
			MenuAnchor->AsShared()
		]
	];
}

FReply SSTViewportToolBarComboMenu::ChangeMenuAnchor_OnMenuClicked()
{
	MenuAnchor->SetIsOpen(!MenuAnchor->IsOpen());

	return FReply::Handled();
}

void SSTViewportToolBarComboMenu::OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent)
{
	// Disable the check for validity
}

bool SSTViewportToolBarComboMenu::IsSmartTransformActive_IsEnabled() const
{
	return GLevelEditorModeTools().IsModeActive(FSTEdMode::EM_STEdModeId);
}
