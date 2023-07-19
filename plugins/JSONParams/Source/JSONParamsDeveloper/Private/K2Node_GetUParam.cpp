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
#include "K2Node_GetUParam.h"

#include "ParamsRegistry.h"
#include "ParamsRegistryLibrary.h"

#include "BlueprintActionDatabaseRegistrar.h"
#include "BlueprintNodeSpawner.h"
#include "K2Node_CallFunction.h"
#include "Kismet2/BlueprintEditorUtils.h"
#include "KismetCompiler.h"


const FName UK2Node_GetUParam::ParamPinName = "Param";
const FName UK2Node_GetUParam::TypePinName =
	*(ParamPinName.ToString() + "_" + GET_MEMBER_NAME_STRING_CHECKED(FParamRegistryInfo, Type));
const FName UK2Node_GetUParam::NamePinName =
	*(ParamPinName.ToString() + "_" + GET_MEMBER_NAME_STRING_CHECKED(FParamRegistryInfo, Name));
const FName UK2Node_GetUParam::ParamNotFoundPinName = "Not found";


void UK2Node_GetUParam::GetMenuActions(FBlueprintActionDatabaseRegistrar& ActionRegistrar) const
{
	Super::GetMenuActions(ActionRegistrar);

	UClass* ActionKey = GetClass();
	if (ActionRegistrar.IsOpenForRegistration(ActionKey))
	{
		UBlueprintNodeSpawner* NodeSpawner = UBlueprintNodeSpawner::Create(ActionKey);
		check(NodeSpawner != nullptr);

		ActionRegistrar.AddBlueprintAction(ActionKey, NodeSpawner);
	}
}

FText UK2Node_GetUParam::GetNodeTitle(ENodeTitleType::Type TitleType) const
{
	if (TitleType == ENodeTitleType::MenuTitle)
	{
		return FText::FromString("Get UParam");
	}

	if (const UEdGraphPin* TypePin = GetTypePin())
	{
		if (TypePin->LinkedTo.Num() > 0 || !TypePin->DefaultObject)
		{
			return FText::FromString("Get UParam");
		}

		if (CachedNodeTitle.IsOutOfDate(this))
		{
			FFormatNamedArguments Args;
			Args.Add(TEXT("ParamTypeName"), FText::FromString(TypePin->DefaultObject->GetName()));

			const FText Format = FText::FromString("Get UParam {ParamTypeName}");
			// FText::Format() is slow, so we cache this to save on performance
			CachedNodeTitle.SetCachedText(FText::Format(Format, Args), this);
		}
	}
	else
	{
		return FText::FromString("Get UParam");
	}

	return CachedNodeTitle;
}

void UK2Node_GetUParam::NotifyPinConnectionListChanged(UEdGraphPin* Pin)
{
	Super::NotifyPinConnectionListChanged(Pin);

	if (Pin == GetResultPin())
	{
		SyncOutputPinType();
	}
}

void UK2Node_GetUParam::PinDefaultValueChanged(UEdGraphPin* Pin)
{
	Super::PinDefaultValueChanged(Pin);

	if (Pin && Pin->PinName == TypePinName)
	{
		if (UEdGraphPin* NamePin = GetNamePin())
		{
			NamePin->ResetDefaultValue();
		}

		SetReturnTypeForStruct(static_cast<UScriptStruct*>(Pin->DefaultObject));
	}
}

void UK2Node_GetUParam::AllocateDefaultPins()
{
	const UEdGraphSchema_K2* K2Schema = GetDefault<UEdGraphSchema_K2>();

	// Add execution pins
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Execute);
	CreatePin(EGPD_Input, UEdGraphSchema_K2::PC_Struct, FParamRegistryInfo::StaticStruct(), ParamPinName);

	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, UEdGraphSchema_K2::PN_Then);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Exec, ParamNotFoundPinName);
	CreatePin(EGPD_Output, UEdGraphSchema_K2::PC_Wildcard, UEdGraphSchema_K2::PN_ReturnValue);

	Super::AllocateDefaultPins();
}

void UK2Node_GetUParam::PostReconstructNode()
{
	Super::PostReconstructNode();

	SyncOutputPinType();
}

void UK2Node_GetUParam::ExpandNode(FKismetCompilerContext& CompilerContext, UEdGraph* SourceGraph)
{
	Super::ExpandNode(CompilerContext, SourceGraph);

	const FName FunctionName = GET_FUNCTION_NAME_CHECKED(UParamsRegistryLibrary, GetUParam);
	UK2Node_CallFunction* CallFunction = CompilerContext.SpawnIntermediateNode<UK2Node_CallFunction>(this, SourceGraph);
	CallFunction->FunctionReference.SetExternalMember(FunctionName, UParamsRegistryLibrary::StaticClass());
	CallFunction->AllocateDefaultPins();
	CompilerContext.MessageLog.NotifyIntermediateObjectCreation(CallFunction, this);

	//Input
	CompilerContext.MovePinLinksToIntermediate(*GetParamPin(), *CallFunction->FindPinChecked(TEXT("Param")));

	UEdGraphPin* NodeResultPin = GetResultPin();
	UEdGraphPin* InternalResultPin = CallFunction->FindPinChecked(TEXT("OutStruct"));
	InternalResultPin->PinType = NodeResultPin->PinType;

	//Output
	CompilerContext.MovePinLinksToIntermediate(*NodeResultPin, *InternalResultPin);

	//Exec pins
	UEdGraphPin* NodeExec = GetExecPin();
	UEdGraphPin* InternalExec = CallFunction->GetExecPin();
	CompilerContext.MovePinLinksToIntermediate(*NodeExec, *InternalExec);

	UEdGraphPin* NodeThen = FindPin(UEdGraphSchema_K2::PN_Then);
	UEdGraphPin* InternalThen = CallFunction->FindPin(TEXT("True"));
	CompilerContext.MovePinLinksToIntermediate(*NodeThen, *InternalThen);

	UEdGraphPin* NodeFailed = FindPin(ParamNotFoundPinName);
	UEdGraphPin* InternalFailed = CallFunction->FindPin(TEXT("False"));
	CompilerContext.MovePinLinksToIntermediate(*NodeFailed, *InternalFailed);

	//After we are done we break all links to this node (not the internally created one)
	BreakAllNodeLinks();
}

UEdGraphPin* UK2Node_GetUParam::GetResultPin() const
{
	UEdGraphPin* Pin = FindPinChecked(UEdGraphSchema_K2::PN_ReturnValue);
	check(Pin->Direction == EGPD_Output);
	return Pin;
}

UEdGraphPin* UK2Node_GetUParam::GetTypePin() const
{
	if (UEdGraphPin* Pin = FindPin(TypePinName))
	{
		check(Pin->Direction == EGPD_Input);
		return Pin;
	}
	return nullptr;
}

UEdGraphPin* UK2Node_GetUParam::GetNamePin() const
{
	if (UEdGraphPin* Pin = FindPin(NamePinName))
	{
		check(Pin->Direction == EGPD_Input);
		return Pin;
	}
	return nullptr;
}

UEdGraphPin* UK2Node_GetUParam::GetParamPin() const
{
	UEdGraphPin* Pin = FindPinChecked(ParamPinName);
	check(Pin->Direction == EGPD_Input);
	return Pin;
}

void UK2Node_GetUParam::SetReturnTypeForStruct(UScriptStruct* NewStruct) const
{
	const UScriptStruct* OldRowStruct = GetReturnTypeForStruct();
	if (NewStruct != OldRowStruct)
	{
		UEdGraphPin* ResultPin = GetResultPin();

		if (ResultPin->SubPins.Num() > 0)
		{
			GetSchema()->RecombinePin(ResultPin);
		}

		// NOTE: purposefully not disconnecting the ResultPin (even though it changed type)... we want the user to see the old
		//       connections, and incompatible connections will produce an error (plus, some super-struct connections may still be valid)
		ResultPin->PinType.PinSubCategoryObject = NewStruct;
		ResultPin->PinType.PinCategory =
			(NewStruct == nullptr) ? UEdGraphSchema_K2::PC_Wildcard : UEdGraphSchema_K2::PC_Struct;

		UEdGraph* Graph = GetGraph();
		Graph->NotifyGraphChanged();

		CachedNodeTitle.Clear();
	}
}

UScriptStruct* UK2Node_GetUParam::GetReturnTypeForStruct() const
{
	UScriptStruct* ReturnStructType = static_cast<UScriptStruct*>(GetResultPin()->PinType.PinSubCategoryObject.Get());

	return ReturnStructType;
}

UScriptStruct* UK2Node_GetUParam::GetParamStructType() const
{
	const UEdGraphPin* TypePin = GetTypePin();
	if (TypePin && TypePin->LinkedTo.Num() == 0)
	{
		return Cast<UScriptStruct>(TypePin->DefaultObject);
	}

	UScriptStruct* ResultType = nullptr;
	UEdGraphPin* ResultPin = GetResultPin();
	if (ResultPin && ResultPin->LinkedTo.Num() > 0)
	{
		ResultType = Cast<UScriptStruct>(ResultPin->LinkedTo[0]->PinType.PinSubCategoryObject.Get());
		for (int32 LinkIndex = 1; LinkIndex < ResultPin->LinkedTo.Num(); ++LinkIndex)
		{
			UEdGraphPin* Link = ResultPin->LinkedTo[LinkIndex];
			UScriptStruct* LinkType = Cast<UScriptStruct>(Link->PinType.PinSubCategoryObject.Get());

			if (ResultType->IsChildOf(LinkType))
			{
				ResultType = LinkType;
			}
		}
	}
	return ResultType;
}

void UK2Node_GetUParam::SyncOutputPinType()
{
	SetReturnTypeForStruct(GetParamStructType());
}
