﻿/*
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

#include "AssetRegisterSettings.h"

namespace UAssetRegisterSettingsLocal
{
	const FName VSPEditor(TEXT("VSP Editor"));
	const FName AssetRegisterSettings(TEXT("AssetRegisterSettings"));
}

UAssetRegisterSettings* UAssetRegisterSettings::Get()
{
	return GetMutableDefault<UAssetRegisterSettings>();
}

FName UAssetRegisterSettings::GetCategoryName() const
{
	return UAssetRegisterSettingsLocal::VSPEditor;
}

FName UAssetRegisterSettings::GetSectionName() const
{
	return UAssetRegisterSettingsLocal::AssetRegisterSettings;
}