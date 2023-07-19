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
#include "STCommands.h"

#define LOCTEXT_NAMESPACE "SmartTransformModule"

void FSTCommands::RegisterCommands()
{
	UI_COMMAND(
		SmartTransformToggle,
		"Smart Transform",
		"On/Off Smart Transform (Alt + Shift + S)",
		EUserInterfaceActionType::ToggleButton,
		FInputChord(EKeys::S, EModifierKey::Alt | EModifierKey::Shift));
	UI_COMMAND(
		SmartTransformOptions,
		"Smart Transform Settings",
		"Smart Transform Settings",
		EUserInterfaceActionType::ToggleButton,
		FInputChord());
}

#undef LOCTEXT_NAMESPACE