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
#include "VSPMeshPaintToolkit.h"
#include "EditorModeManager.h"

#define LOCTEXT_NAMESPACE "VSPMeshPaintMode"

static const FName VSPMeshPainting = TEXT("VSPMeshPainting");

void FVSPMeshPaintToolkit::GetToolPaletteNames(TArray<FName>& PaletteNames) const
{
	PaletteNames.Add(VSPMeshPainting);
}

FName FVSPMeshPaintToolkit::GetToolkitFName() const
{
	return VSPMeshPainting;
}

FText FVSPMeshPaintToolkit::GetBaseToolkitName() const
{
	return NSLOCTEXT("SupportToolEdModeToolkit", "DisplayName", "SupportToolEdMode Tool");
}

UEdMode* FVSPMeshPaintToolkit::GetScriptableEditorMode() const
{
	return GLevelEditorModeTools().GetActiveScriptableMode(VSPMeshPainting);
}

#undef LOCTEXT_NAMESPACE
