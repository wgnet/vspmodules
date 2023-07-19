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
#include "CoreMinimal.h"

#include "ISourceControlProvider.h"
#include "Framework/Docking/TabManager.h"
#include "Core/VSPGitState.h"
#include "Core/Base/VSPGitHelpers.h"

class FVSPGitSlateLayout
{
public:
	static void Initialize();

	static void Shutdown();

	static FVSPGitSlateLayout& Get();
	static FName GetLayoutName();
	TSharedPtr< FTabManager > GetTabManager();

	void RestoreLayout() const;

	void ShowUiExtenders();
	void HideUiExtenders();

	void SetBranchesDivergence( FString InLocalBranch, FString InUpstreamBranch, VSPGitHelpers::FDivergence InDivergence );

	void PushClicked();
	void PullClicked();
	void FetchClicked();
	void LockMapsAsync(const TArray< FString >& InContentPathList);
	void UnlockMapsAsync(const TArray< FString >& InContentPathList, bool bIsForce);

	static void DisplaySuccessNotification( const FSourceControlOperationRef& InOperation );
	static void DisplayFailureNotification( const FSourceControlOperationRef& InOperation );
	static void DisplayErrorMessage( const FSourceControlOperationRef& InOperation );

	void DisplayInProgressNotification(const FText& Text);
	void RemoveInProgressNotification();
private:
	void RegisterTabSpawners();
	void RegisterToolbarExtender();
	void RegisterMenusAndButton();

	bool HandleToolbarButtonVisible();

	// Temp holder
	const FSlateBrush* GetHolderImage() const;
	TSharedRef< SWidget > GetHolderWidget();

	TSharedRef< SDockTab > SpawnMainTab( const FSpawnTabArgs& Args );
	TSharedRef< SDockTab > SpawnSubTab( const FSpawnTabArgs& Args, FName TabIdentifier );

	static TSharedRef< class FVSPGitSlateLayout > Create();

	void OnSourceControlOperationComplete( const FSourceControlOperationRef& InOperation, ECommandResult::Type InResult );
private:
	static TSharedPtr< class FVSPGitSlateLayout > LayoutInstance;
	TSharedPtr< FTabManager::FLayout > MainLayout = nullptr;
	TSharedPtr< class FUICommandList > PluginCommands;
	bool bIsToolbarButtonVisible = false;
	FString LocalBranch;
	FString UpstreamBranch;
	VSPGitHelpers::FDivergence Divergence;

	TWeakPtr< class SNotificationItem > OperationInProgressNotification;
};
