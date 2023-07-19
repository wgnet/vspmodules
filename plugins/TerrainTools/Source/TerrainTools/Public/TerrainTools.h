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
#include "Modules/ModuleManager.h"

class FSlateStyleSet;

class FTerrainToolsModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
	void RegisterPlacementCategory() const;
	void UnregisterPlacementCategory() const;

	void RegisterDetailsPanels() const;
	void UnregisterDetailsPanels() const;

	void RegisterComponentVisualizers();
	void UnregisterComponentVisualizers();

	void RegisterSlateStyle() const;
	void UnregisterSlateStyle() const;
	static TSharedPtr<FSlateStyleSet> StyleSet;
};
