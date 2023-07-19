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
#include "MapSectorLockView.h"

#include "IVSPGitModule.h"
#include "MapSectorLockViewStyle.h"
#include "LevelEditor.h"
#include "Widgets/Docking/SDockTab.h"
#include "Widgets/Layout/SBox.h"
#include "ToolMenus.h"
#include "Widgets/SMapLayoutView.h"
#include "Widgets/SListWidget.h"

#define LOCTEXT_NAMESPACE "FMapSectorLockViewModule"

DEFINE_LOG_CATEGORY( MapSectorLockLog );

class IVSPGitModule;
static const FName MapSectorLockViewTabName( "MapSectorLockView" );

static TSharedPtr< FTabManager > MSLTabManager;

void FMapSectorLockViewModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module

	FMapSectorLockViewStyle::Initialize();
	FMapSectorLockViewStyle::ReloadTextures();

	SectorProvider = MakeShared< FSectorProvider >();
	SectorProvider->Initialize();
}

void FMapSectorLockViewModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.

	UToolMenus::UnRegisterStartupCallback( this );

	UToolMenus::UnregisterOwner( this );

	FMapSectorLockViewStyle::Shutdown();
}

TSharedRef< SDockTab > FMapSectorLockViewModule::OnSpawnPluginTab( const FSpawnTabArgs& SpawnTabArgs )
{
	TSharedPtr< FTabManager::FLayout > Layout = FTabManager::NewLayout( "MapSectorLockView_Layout" )
		->AddArea(
			FTabManager::NewPrimaryArea()
			->Split(
				FTabManager::NewStack()
				->SetSizeCoefficient( 0.8f )
				->SetHideTabWell( true )
				->AddTab( "MapSectorViewTab", ETabState::OpenedTab )
			)
		);

	TSharedRef< SDockTab > MSLTab =
		SNew( SDockTab )
		.TabRole( NomadTab )
		.Label( LOCTEXT( "MapSectorLockViewTabLabel", "Map Sector Lock View" ) );

	MSLTabManager = FGlobalTabmanager::Get()->NewTabManager( MSLTab );

	MSLTabManager->RegisterTabSpawner( "MapSectorViewTab", FOnSpawnTab::CreateRaw( this, &FMapSectorLockViewModule::SpawnSubTab, FName( "MapSectorViewTab" ) ) )
		.SetDisplayName( LOCTEXT( "MapSectorViewTab", "Sectors Viewport" ) );

	MSLTab->SetContent(
		MSLTabManager->RestoreFrom( Layout.ToSharedRef(), SpawnTabArgs.GetOwnerWindow() ).ToSharedRef()
	);

	return MSLTab;
}

TSharedRef< SDockTab > FMapSectorLockViewModule::SpawnSubTab( const FSpawnTabArgs& Args, FName TabIdentifier )
{
	if ( TabIdentifier == FName( "MapSectorViewTab" ) )
		return SNew( SDockTab )
			.Label( LOCTEXT( "MapSectorViewTab", "Sectors Viewport" ) )
			.Clipping( EWidgetClipping::ClipToBounds )
			[
				SNew( SBox )
				.HAlign( HAlign_Center )
				.VAlign( VAlign_Center )
				[
					SNew( SMapLayoutView )
					.SectorProvider( SectorProvider )
				]
			];
	ensure( false );

	return SNew( SDockTab );
}

void FMapSectorLockViewModule::PluginButtonClicked()
{
	FGlobalTabmanager::Get()->TryInvokeTab( MapSectorLockViewTabName );
}

FMapSectorLockViewModule& FMapSectorLockViewModule::Get()
{
	return FModuleManager::LoadModuleChecked< FMapSectorLockViewModule >( GetModuleName() );
}

FName FMapSectorLockViewModule::GetModuleName()
{
	static FName ModuleName = FName( "MapSectorLockView" );
	return ModuleName;
}

void FMapSectorLockViewModule::RegisterTabSpawner()
{
	IVSPGitModule& GitModule = FModuleManager::LoadModuleChecked< IVSPGitModule >( "VSPGitModule" );
	GitModule.GetTabManager()->RegisterTabSpawner( MapSectorLockViewTabName, FOnSpawnTab::CreateRaw( this, &FMapSectorLockViewModule::OnSpawnPluginTab ) )
		.SetDisplayName( LOCTEXT( "FMapSectorLockViewTabTitle", "MapSectorLockView" ) )
		.SetMenuType( ETabSpawnerMenuType::Hidden );
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE( FMapSectorLockViewModule, MapSectorLockView )
