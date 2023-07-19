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

#include "CoreMinimal.h"
#include "IVSPExternalModule.h"

class FSectorProvider;
class IListItem;
class SListWidget;

DECLARE_LOG_CATEGORY_EXTERN( MapSectorLockLog, Log, All );

class FMapSectorLockViewModule : public IVSPExternalModule // -> IModuleInterface
{
public:
	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** This function will be bound to Command (by default it will bring up plugin window) */
	void PluginButtonClicked();

	static FMapSectorLockViewModule& Get();
	static FName GetModuleName();

	virtual void RegisterTabSpawner() override;
private:
	TSharedRef< SDockTab > OnSpawnPluginTab( const class FSpawnTabArgs& SpawnTabArgs );
	TSharedRef< SDockTab > SpawnSubTab( const FSpawnTabArgs& Args, FName TabIdentifier );

private:
	TSharedPtr< FSectorProvider > SectorProvider;
};
