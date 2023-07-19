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

#include "Toolkits/BaseToolkit.h"

class FFlowMapPainterEdMode;

class FFlowMapPainterEdModeToolkit : public FModeToolkit
{
public:
	FFlowMapPainterEdModeToolkit();

	/** FModeToolkit interface */
	virtual void Init(const TSharedPtr<IToolkitHost>& InitToolkitHost) override;

	/** IToolkit interface */
	virtual FName GetToolkitFName() const override;
	virtual FText GetBaseToolkitName() const override;
	virtual FEdMode* GetEditorMode() const override;
	virtual TSharedPtr<class SWidget> GetInlineContent() const override
	{
		return ToolkitWidget;
	}
	virtual FFlowMapPainterEdMode* GetToolsEditorMode() const;

private:
	TSharedPtr<SWidget> ToolkitWidget;
	TSharedPtr<IDetailsView> DetailsView;

	bool CanStartTool(const FString& ToolTypeIdentifier) const;
	bool CanAcceptActiveTool() const;
	bool CanCancelActiveTool() const;
	bool CanCompleteActiveTool() const;
	FReply StartTool(const FString& ToolTypeIdentifier) const;
	FReply EndTool(EToolShutdownType ShutdownType) const;
};
