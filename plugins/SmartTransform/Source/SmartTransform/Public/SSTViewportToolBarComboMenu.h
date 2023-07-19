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

class SMenuAnchor;
class SViewportToolBar;

/**
 * Custom widget to display a toggle/drop down menu. 
 *	Displayed as shown:
 * +--------+-------------+
 * | Toggle | Menu button |
 * +--------+-------------+
 */
class SSTViewportToolBarComboMenu : public SCompoundWidget
{
public:
	SLATE_BEGIN_ARGS(SSTViewportToolBarComboMenu)
		: _BlockLocation(EMultiBlockLocation::Start)
		, _MinDesiredButtonWidth(-1.0f)
	{
	}

	/** Content for the drop down menu  */
	SLATE_EVENT(FOnGetContent, OnGetMenuContent)

	/** Called upon state change with the value of the next state */
	SLATE_EVENT(FOnCheckStateChanged, OnCheckStateChanged)

	/** Sets the current checked state of the checkbox */
	SLATE_ATTRIBUTE(ECheckBoxState, IsChecked)

	/** Icon shown on the toggle button */
	SLATE_ATTRIBUTE(FSlateIcon, Icon)

	/** Label shown on the menu button */
	SLATE_ATTRIBUTE(FText, Label)

	/** Overall style */
	SLATE_ATTRIBUTE(FName, Style)

	/** ToolTip shown on the menu button */
	SLATE_ATTRIBUTE(FText, MenuButtonToolTip)

	/** ToolTip shown on the toggle button */
	SLATE_ATTRIBUTE(FText, ToggleButtonToolTip)

	/** The button location */
	SLATE_ARGUMENT(EMultiBlockLocation::Type, BlockLocation)

	/** The minimum desired width of the menu button contents */
	SLATE_ARGUMENT(float, MinDesiredButtonWidth)

	SLATE_END_ARGS()

	/**
	 * Construct this widget
	 *
	 * @param	InArgs	The declaration data for this widget
	 */
	void Construct(const FArguments& InArgs);

private:
	/**
	 * Called when the menu button is clicked.  Will toggle the visibility of the menu content
	 */
	FReply ChangeMenuAnchor_OnMenuClicked();

	/**
	 * Called when the mouse enters a menu button.  If there was a menu previously opened
	 * we open this menu automatically
	 */
	virtual void OnMouseEnter(const FGeometry& MyGeometry, const FPointerEvent& MouseEvent) override;

	bool IsSmartTransformActive_IsEnabled() const;

	/** Our menus anchor */
	TSharedPtr<SMenuAnchor> MenuAnchor;

	/** Parent tool bar for querying other open menus */
	TWeakPtr<SViewportToolBar> ParentToolBar;
};