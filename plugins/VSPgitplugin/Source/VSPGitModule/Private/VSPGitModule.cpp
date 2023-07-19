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
#include "VSPGitModule.h"
// Engine headers
#include "Features/IModularFeatures.h"
// Module headers
#include "UI/VSPGitStyle.h"
#include "UI/VSPGitSlateLayout.h"
#include "Core/Commands/GitHighLevelWorkers.h"
#include "UI/Widgets/VSPMessageBox.h"

FName FeatureName = FName( TEXT( "SourceControl" ) );

DEFINE_LOG_CATEGORY( VSPGitLog );

#define LOCTEXT_NAMESPACE "FVSPGitModule"

template < typename Type >
static TSharedRef< IVSPGitWorker, ESPMode::ThreadSafe > CreateWorker()
{
	return MakeShareable( new Type() );
}

void FVSPGitModule::StartupModule()
{
	UE_LOG( VSPGitLog, Log, TEXT("VSPGitModule => StartupModule") );

	// Init style
	FVSPGitStyle::Initialize();
	FVSPGitStyle::ReloadTextures();
	// Init UI
	FVSPGitSlateLayout::Initialize();
	// Register provider
	IModularFeatures::Get().RegisterModularFeature( FeatureName, &Provider );

	// Adding some source control workers
	Provider.AddWorker( "Connect", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FConnectWorker > ) );
	Provider.AddWorker( "UpdateStatus", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FUpdateStatusWorker > ) );
	Provider.AddWorker( "MarkForAdd", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FMarkForAddWorker > ) );
	Provider.AddWorker( "Delete", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FDeleteWorker > ) );
	Provider.AddWorker( "Revert", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FRevertWorker > ) );
	Provider.AddWorker( "Commit", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FCommitWorker > ) );
	Provider.AddWorker( "Fetch", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FFetchWorker > ) );
	Provider.AddWorker( "Push", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FPushWorker > ) );
	Provider.AddWorker( "Pull", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FPullWorker > ) );
	Provider.AddWorker( "LfsLocks", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FLfsLocksWorker > ) );
	Provider.AddWorker( "Lock", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FLockWorker > ) );
	Provider.AddWorker( "Unlock", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FUnlockWorker > ) );
	Provider.AddWorker( "CheckOut", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FLockWorker > ) );
	Provider.AddWorker( "CheckIn", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FCommitWorker > ) );
	Provider.AddWorker( "Background", FGetVSPGitWorker::CreateStatic( &CreateWorker< GitHighLevelWorkers::FBackgroundWorker > ) );

#if IS_PROGRAM
	// Note: TThis call allows you to open a window when the program starts.
	// In plugin mode, the button in the toolbar will be responsible for opening the window.
	FVSPGitSlateLayout::Get().RestoreLayout();
#endif
}

void FVSPGitModule::ShutdownModule()
{
	// Deinitialize and unregister the provider
	IModularFeatures::Get().UnregisterModularFeature( FeatureName, &Provider );
	// Shutdown UI
	FVSPGitSlateLayout::Shutdown();
	// Shutdown style
	FVSPGitStyle::Shutdown();

	UE_LOG( VSPGitLog, Log, TEXT("VSPGitModule => ShutdownModule") )
}

TSharedPtr< FTabManager > FVSPGitModule::GetTabManager()
{
	return FVSPGitSlateLayout::Get().GetTabManager();
}

FVSPGitModule& FVSPGitModule::Get()
{
	return FModuleManager::LoadModuleChecked< FVSPGitModule >( GetModuleName() );
}

FName FVSPGitModule::GetModuleName()
{
	static FName ModuleName = FName( "VSPGitModule" );
	return ModuleName;
}

FVSPGitProvider& FVSPGitModule::GetProvider()
{
	return Provider;
}

FVSPGitProvider& FVSPGitModule::AccessProvider()
{
	return Provider;
}

FString FVSPGitModule::GetGitUserName() const
{
	return Provider.GetSettings()->GetCurrentRepoSettings()->UserName;
}

FString FVSPGitModule::GetGitUserEmail() const
{
	return Provider.GetSettings()->GetCurrentRepoSettings()->UserEmail;
}

bool FVSPGitModule::IsFileLocked( const FString& InContentPath, FString& OutUserName, int32& OutLockId )
{
	return Provider.IsFileLocked( InContentPath, OutUserName, OutLockId );
}

void FVSPGitModule::LockMaps( const TArray< FString >& InContentPathList )
{
	FVSPGitSlateLayout::Get().LockMapsAsync(InContentPathList);
}

void FVSPGitModule::UnlockMaps( const TArray< FString >& InContentPathList, bool bIsForce )
{
	FVSPGitSlateLayout::Get().UnlockMapsAsync(InContentPathList, bIsForce);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FVSPGitModule, VSPGitModule )
