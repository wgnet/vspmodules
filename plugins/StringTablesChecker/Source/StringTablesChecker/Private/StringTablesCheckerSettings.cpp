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
#include "StringTablesCheckerSettings.h"

namespace FStringTablesCheckerSettingsLocal
{
	const FName CategoryName(TEXT("VSP Editor"));
	const FName SectionName(TEXT("StringTablesCheckerSettings"));
}

UStringTablesCheckerSettings* UStringTablesCheckerSettings::Get()
{
	return GetMutableDefault<UStringTablesCheckerSettings>();
}

FString UStringTablesCheckerSettings::GetConfluenceURL() const
{
	return ConfluenceURL;
}

TArray<FSoftClassPath> UStringTablesCheckerSettings::GetSupportedAssetsClasses() const
{
	return SupportedAssetsClasses;
}

TArray<FString> UStringTablesCheckerSettings::GetAssetsPaths() const
{
	return AssetsPaths;
}

TArray<FString> UStringTablesCheckerSettings::GetStringTablesPaths() const
{
	return StringTablesPaths;
}

TArray<FString> UStringTablesCheckerSettings::GetStringTablesExclusions() const
{
	return StringTablesExclusions;
}

FName UStringTablesCheckerSettings::GetCategoryName() const
{
	return FStringTablesCheckerSettingsLocal::CategoryName;
}

FName UStringTablesCheckerSettings::GetSectionName() const
{
	return FStringTablesCheckerSettingsLocal::SectionName;
}
