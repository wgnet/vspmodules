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
#include "ISkeletalMeshEditor.h"
#include "Modules/ModuleManager.h"
#include "WorkflowOrientedApp/WorkflowTabFactory.h"

class UHitboxViewer;

class FHitboxToolModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	TSharedRef<FExtender> ExtendSkeletalMeshEditor(
		const TSharedRef<FUICommandList> CommandList,
		const TArray<UObject*> Objects);
	TSharedRef<FApplicationMode> ExtendApplicationMode(const FName ModeName, TSharedRef<FApplicationMode> InMode);
	void CreateToolbar(
		FToolBarBuilder& ToolbarBuilder,
		const TSharedRef<FUICommandList> CommandList,
		ISkeletalMeshEditor* InSkeletalMeshEditor);
	void ShowTabForEditor(ISkeletalMeshEditor* InSkeletalMeshEditor);

	void OnLevelAddedToWorld(ULevel* Level, UWorld* World);
	void OnMapOpened(const FString& Filename, bool bLoadAsTemplate);
	void OnWorldCleanup(UWorld* World, bool bSessionEnded, bool bCleanupResources);

	static UHitboxViewer* GetInstanceToolSetting(ISkeletalMeshEditor* InSkeletalMeshEditor);

protected:
	static void ReleaseInstanceToolSetting();
	TArray<TWeakPtr<FApplicationMode>> RegisteredApplicationModes;
	FWorkflowApplicationModeExtender AppExtender;

private:
	static TMap<ISkeletalMeshEditor*, TWeakObjectPtr<UHitboxViewer>> EditorTabMap;
	FVector2D InitialWindowSize = { 512, 1024 };

	void RegisterEditorDelegates();
	void UnregisterEditorDelegates();
};
