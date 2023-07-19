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
#include "JSONParamsCommands.h"

#include "JSONParamsStyle.h"


#define LOCTEXT_NAMESPACE "FJSONParamsModule"


FJSONParamsCommands::FJSONParamsCommands()
	: TCommands<FJSONParamsCommands>(
		TEXT("JSONParams"),
		FText::FromString("JSONParams Plugin"),
		NAME_None,
		FJSONParamsStyle::GetStyleSetName())
{
	;
}

void FJSONParamsCommands::RegisterCommands()
{
	UI_COMMAND(
		OpenPluginWindow,
		"Create JSON Params",
		"Bring up an create JSONParams window",
		EUserInterfaceActionType::Button,
		FInputChord());
	UI_COMMAND(
		SaveOpenedParam,
		"Save param",
		"Saves opened JSON param",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Control, EKeys::S));
	UI_COMMAND(
		RevertOpenedParam,
		"Revert param",
		"Reverts opened param from original JSON",
		EUserInterfaceActionType::Button,
		FInputChord());
	UI_COMMAND(
		BrowseToOpenedParam,
		"Browse to param",
		"Browses to the param and selects it in Content Browser",
		EUserInterfaceActionType::Button,
		FInputChord(EModifierKey::Control, EKeys::B));
}

#undef LOCTEXT_NAMESPACE
