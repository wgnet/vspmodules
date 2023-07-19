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
#include "ParamsSettings.h"

#include "ParamsRegistry.h"


const UParamsSettings* UParamsSettings::Get()
{
	return GetDefault<UParamsSettings>();
}

FName UParamsSettings::GetContainerName() const
{
	return TEXT("Project");
}

FName UParamsSettings::GetCategoryName() const
{
	return TEXT("Plugins");
}

FName UParamsSettings::GetSectionName() const
{
	return TEXT("JSON Params Settings");
}

#if WITH_EDITOR
FText UParamsSettings::GetSectionText() const
{
	return FText::FromString("JSON Params Settings");
}

void UParamsSettings::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	const FName MemberPropertyName = PropertyChangedEvent.MemberProperty->GetFName();

	if (MemberPropertyName == GET_MEMBER_NAME_CHECKED(UParamsSettings, ParamFilesRootPaths))
	{
		for (FDirectoryPath& RootPath : ParamFilesRootPaths)
		{
			FPaths::MakePathRelativeTo(RootPath.Path, *FPaths::ProjectDir());
		}

		FParamsRegistry::Get().Init();
	}
}

bool UParamsSettings::GetEnableContentBrowserIntegration() const
{
	return EnableContentBrowserIntegration;
}

void UParamsSettings::SetEnableContentBrowserIntegration(bool bIsEnabled)
{
	UParamsSettings* Settings = GetMutableDefault<UParamsSettings>();
	Settings->EnableContentBrowserIntegration = bIsEnabled;
	Settings->SaveConfig();
}

FName UParamsSettings::GetContentBrowserVirtualRoot() const
{
	return VirtualRoot;
}
#endif

FString UParamsSettings::GetParamsServerURL() const
{
	if (IsEnvVariable(ParamsServerURL))
		return GetEnvVariable(ParamsServerURL);
	return ParamsServerURL;
}

TArray<FString> UParamsSettings::GetParamsRootPaths(bool IncludeServerPath) const
{
	TArray<FString> ParamsRootPaths = {};
	for (const FDirectoryPath& ParamFilesRootPath : ParamFilesRootPaths)
		ParamsRootPaths.Add(FPaths::Combine(FPaths::ProjectDir(), ParamFilesRootPath.Path));

	if (IncludeServerPath)
		ParamsRootPaths.Add(GetParamsServerURL());

	return ParamsRootPaths;
}

bool UParamsSettings::IsEnvVariable(const FString& Variable)
{
	return Variable.StartsWith("${") && Variable.EndsWith("}");
}

FString UParamsSettings::GetEnvVariable(const FString& Variable) const
{
	if (IsEnvVariable(Variable))
		return FPlatformMisc::GetEnvironmentVariable(*Variable.Mid(2, Variable.Len() - 3));
	return FPlatformMisc::GetEnvironmentVariable(*Variable);
}
