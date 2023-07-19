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

#include "ContentInspectorSettings.h"

namespace UContentInspectorSettingsLocal
{
	const FName VSPEditor(TEXT("VSP Editor"));
	const FName ContentInspectorSettings(TEXT("ContentInspectorSettings"));
}

const UContentInspectorSettings* UContentInspectorSettings::Get()
{
	return GetDefault<UContentInspectorSettings>();
}

TArray<FInspectionsForClass> UContentInspectorSettings::GetClassesAndInspections() const
{
	return ClassesAndInspections;
}

bool UContentInspectorSettings::CanRejectSavingAsset() const
{
	return bRejectSavingAssets;
}

TMap<FSoftObjectPath, FName> UContentInspectorSettings::GetAssetPrefixDictionary() const
{
	return AssetPrefixDictionary;
}

FSoftObjectPath UContentInspectorSettings::GetAssetRegisterMarkClass() const
{
	return AssetRegisterMarkClass;
}

FSoftObjectPath UContentInspectorSettings::GetPainterMasterShader() const
{
	return MeshPainterMasterShader;
}

FSoftObjectPath UContentInspectorSettings::GetAnimationStatusUserDataClass() const
{
	return AnimationStatusUserDataClass;
}

TArray<FDirectoryPath> UContentInspectorSettings::GetRetainOnLoadIncludePathsArray() const
{
	return RetainOnLoadIncludePathsArray;
}

TArray<FDirectoryPath> UContentInspectorSettings::GetInspectExcludeFolders() const
{
	return InspectExcludeFolders;
}

float UContentInspectorSettings::GetMinimumPolyCountDifferenceValue() const
{
	return MinimumPolyCountDifference;
}

const FName& UContentInspectorSettings::GetConfluenceURL(const TSoftClassPtr<UInspectionBase>& InInspectionClass) const
{
	UClass* InspectionClass = InInspectionClass.Get();
	if (ConfluenceURLs.Contains(InspectionClass))
	{
		return ConfluenceURLs[InspectionClass];
	}
	return EmptyURL;
}

FName UContentInspectorSettings::GetCategoryName() const
{
	return UContentInspectorSettingsLocal::VSPEditor;
}

FName UContentInspectorSettings::GetSectionName() const
{
	return UContentInspectorSettingsLocal::ContentInspectorSettings;
}
