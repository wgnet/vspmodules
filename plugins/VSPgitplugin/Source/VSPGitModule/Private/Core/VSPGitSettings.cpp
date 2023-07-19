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
#include "VSPGitSettings.h"
// Engine headers
#include "Core.h"
#include "SourceControl/Public/SourceControlHelpers.h"

namespace VSPGitSettingsConstants
{
	static const FString SettingsSection{ TEXT( "VSPGitSourceControl.VSPGitSettings" ) };
}

void FVSPGitSettings::LoadSettings()
{
	FScopeLock ScopeLock( &CriticalSection );

	const FString& IniFile = SourceControlHelpers::GetSettingsIni();

	GConfig->GetInt( *VSPGitSettingsConstants::SettingsSection, TEXT( "BackgroundUpdateTimeMs" ), BackgroundUpdateTimeMs, IniFile );
	GConfig->GetInt( *VSPGitSettingsConstants::SettingsSection, TEXT( "AutoFetchUpdateTimeMs" ), AutoFetchUpdateTimeMs, IniFile );
	GConfig->GetString( *VSPGitSettingsConstants::SettingsSection, TEXT( "GitBinPath" ), GitBinPath, IniFile );

#if IS_PROGRAM
    TArray<FString> RepositoriesList;
    GConfig->GetArray(*VSPGitSettingsConstants::SettingsSection, TEXT("Repositories"), RepositoriesList, IniFile);
    Repositories.Empty();
    for (FString RepoPath : RepositoriesList)
    {
        TSharedPtr<FRepoSettings> RepoSettings = MakeShareable(new FRepoSettings());
        RepoSettings->RepoRoot = RepoPath;
        Repositories.Add(RepoPath, RepoSettings);
    }
#endif

	FString RepoRoot;
	GConfig->GetString( *VSPGitSettingsConstants::SettingsSection, TEXT( "CurrentRepo" ), RepoRoot, IniFile );
#if IS_PROGRAM
    if (Repositories.Contains(RepoRoot))
        CurrentRepoPtr = *Repositories.Find(RepoRoot);
#else
	CurrentRepoPtr = MakeShareable( new FRepoSettings() );
	CurrentRepoPtr->RepoRoot = RepoRoot;
#endif
}

void FVSPGitSettings::SaveSettings() const
{
	FScopeLock ScopeLock( &CriticalSection );

	const FString& IniFile = SourceControlHelpers::GetSettingsIni();

	GConfig->SetInt( *VSPGitSettingsConstants::SettingsSection, TEXT( "BackgroundUpdateTimeMs" ), BackgroundUpdateTimeMs, IniFile );
	GConfig->SetInt( *VSPGitSettingsConstants::SettingsSection, TEXT( "AutoFetchUpdateTimeMs" ), AutoFetchUpdateTimeMs, IniFile );
	GConfig->SetString( *VSPGitSettingsConstants::SettingsSection, TEXT( "GitBinPath" ), *GitBinPath, IniFile );
	GConfig->SetString( *VSPGitSettingsConstants::SettingsSection, TEXT( "CurrentRepo" ), *( CurrentRepoPtr->RepoRoot ), IniFile );

#if IS_PROGRAM
    TArray<FString> RepositoriesList;
    for (auto && RepositoryPair : Repositories)
    {
        RepositoriesList.Add(RepositoryPair.Key);
    }
    GConfig->SetArray(*VSPGitSettingsConstants::SettingsSection, TEXT("Repositories"), RepositoriesList, IniFile);
#endif
	GConfig->Flush( false, IniFile );
}

FString FVSPGitSettings::GetGitBinPath() const
{
	FScopeLock ScopeLock( &CriticalSection );
	return GitBinPath;
}

bool FVSPGitSettings::SetGitBinPath( const FString& InGitBinPath )
{
	FScopeLock ScopeLock( &CriticalSection );
	if ( GitBinPath != InGitBinPath )
	{
		GitBinPath = InGitBinPath;
		return true;
	}
	return false;
}

TSharedPtr< FRepoSettings, ESPMode::ThreadSafe >& FVSPGitSettings::GetCurrentRepoSettings()
{
	FScopeLock ScopeLock( &CriticalSection );
	return CurrentRepoPtr;
}

bool FVSPGitSettings::SetCurrentRepoSettings( const TSharedPtr< FRepoSettings, ESPMode::ThreadSafe >& InRepoSettings )
{
	FScopeLock ScopeLock( &CriticalSection );
#if IS_PROGRAM
    if (!Repositories.Contains(InRepoSettings->RepoRoot))
        Repositories.Add(InRepoSettings->RepoRoot, InRepoSettings);
    // Need update settings logic
    CurrentRepoPtr = *Repositories.Find(InRepoSettings->RepoRoot);
#else
	CurrentRepoPtr = InRepoSettings;
#endif
	return true;
}
