﻿/*
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

#include "FlowMapPainterModule.h"
#include "FlowMapPainterEdMode.h"

#define LOCTEXT_NAMESPACE "FFlowMapPainterModule"

FFlowMapPainterModule& FFlowMapPainterModule::Get()
{
	return FModuleManager::LoadModuleChecked<FFlowMapPainterModule>("FlowMapPainterEditor");
}

void FFlowMapPainterModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
	FEditorModeRegistry::Get().RegisterMode<FFlowMapPainterEdMode>(
		FFlowMapPainterEdMode::EM_FlowMapPainterEdModeId,
		LOCTEXT("FlowMapPainterEdModeName", "FlowMapPainter"),
		FSlateIcon(),
		true);
}

void FFlowMapPainterModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
	FEditorModeRegistry::Get().UnregisterMode(FFlowMapPainterEdMode::EM_FlowMapPainterEdModeId);
}

#undef LOCTEXT_NAMESPACE

IMPLEMENT_MODULE(FFlowMapPainterModule, FlowMapPainter)
