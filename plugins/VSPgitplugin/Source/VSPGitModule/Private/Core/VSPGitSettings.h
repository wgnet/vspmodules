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

// Engine headers
#if PLATFORM_WINDOWS
#include "Windows/WindowsCriticalSection.h"
#endif
// Module headers
#include "Base/VSPGitHelpers.h"

class FRepoSettings
{
public:
	FString UserName;
	FString UserEmail;

	FString RepoRoot;
	FString LocalBranch;
	FString UpstreamBranch;
	FString RemoteName{ TEXT( "origin" ) };
	FString RemoteUrl;

	bool bUsingGitLfsLocking = false;
	TArray< FString > LockableRules;
};

class FVSPGitSettings : public TSharedFromThis< FVSPGitSettings, ESPMode::ThreadSafe >
{
public:
	void LoadSettings();
	void SaveSettings() const;

	// Getters & Setters Settings (params with save/load)
	FString GetGitBinPath() const;
	bool SetGitBinPath( const FString& InGitBinPath );

	TSharedPtr< FRepoSettings, ESPMode::ThreadSafe >& GetCurrentRepoSettings();
	bool SetCurrentRepoSettings( const TSharedPtr< FRepoSettings, ESPMode::ThreadSafe >& InRepoSettings );

public: //params
	VSPGitHelpers::FGitVersion GitVersion;
	FString GitLfsVersion;
	bool bGitLfsAvailable;
	int32 BackgroundUpdateTimeMs = 1000 /*Ms*/ * 30 /*sec*/; // * 1 /*Min*/;
	int32 AutoFetchUpdateTimeMs = 1000 /*Ms*/ * 60 /*sec*/ * 5 /*Min*/;

private:
	FString GitBinPath{ TEXT( "git" ) };
	TSharedPtr< FRepoSettings, ESPMode::ThreadSafe > CurrentRepoPtr;
#if IS_PROGRAM
	// TODO: Need change project in UI
	TMap<FString, TSharedPtr<FRepoSettings, ESPMode::ThreadSafe>> Repositories;
#endif
	mutable FCriticalSection CriticalSection;
};
