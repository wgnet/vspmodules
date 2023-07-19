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

#include "EdGraph/EdGraphNodeUtils.h"
#include "K2Node.h"
#include "KismetCompilerMisc.h"

#include "K2Node_GetUParam.generated.h"


UCLASS()
class JSONPARAMSDEVELOPER_API UK2Node_GetUParam : public UK2Node
{
	GENERATED_BODY()
public:
	virtual void AllocateDefaultPins() override;
	virtual void PostReconstructNode() override;
	virtual void ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph) override;
	virtual void GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const override;
	virtual FText GetNodeTitle(ENodeTitleType::Type TitleType) const override;
	virtual void NotifyPinConnectionListChanged(UEdGraphPin* Pin) override;
	virtual void PinDefaultValueChanged(UEdGraphPin* Pin) override;

	UEdGraphPin* GetResultPin() const;
	UEdGraphPin* GetParamPin() const;

	static const FName ParamPinName;
	static const FName TypePinName;
	static const FName NamePinName;
	static const FName ParamNotFoundPinName;

private:
	/** Constructing FText strings can be costly, so we cache the node's title */
	FNodeTextCache CachedNodeTitle;

	UEdGraphPin* GetTypePin() const;
	UEdGraphPin* GetNamePin() const;

	void SetReturnTypeForStruct(UScriptStruct* NewStruct) const;
	UScriptStruct* GetReturnTypeForStruct() const;
	UScriptStruct* GetParamStructType() const;
	void SyncOutputPinType();
};
