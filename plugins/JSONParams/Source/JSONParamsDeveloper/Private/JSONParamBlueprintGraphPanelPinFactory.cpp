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
#include "JSONParamBlueprintGraphPanelPinFactory.h"

#include "EdGraphSchema_K2.h"
#include "K2Node_CallFunction.h"
#include "K2Node_GetUParam.h"
#include "ParamsRegistry.h"
#include "SGraphPinDataTableRowName.h"


TSharedPtr<SGraphPin> FJSONParamBlueprintGraphPanelPinFactory::CreatePin(UEdGraphPin* InPin) const
{
	if (InPin->PinType.PinCategory == UEdGraphSchema_K2::PC_Name)
	{
		UObject* Outer = InPin->GetOuter();

		const UEdGraphPin* ParamPin = nullptr;
		if (const UK2Node_CallFunction* CallFunctionNode = Cast<UK2Node_CallFunction>(Outer))
		{
			if (CallFunctionNode->GetTargetFunction())
			{
				ParamPin = CallFunctionNode->FindPin(UK2Node_GetUParam::ParamPinName);
			}
		}
		else if (const UK2Node_GetUParam* GetUParamNode = Cast<UK2Node_GetUParam>(Outer))
		{
			ParamPin = GetUParamNode->GetParamPin();
		}

		if (!ParamPin || ParamPin->SubPins.Num() != 2 || ParamPin->LinkedTo.Num() > 0)
			return nullptr;

		UEdGraphPin* const* TypePinPointer = ParamPin->SubPins.FindByPredicate(
			[](const UEdGraphPin* Pin)
			{
				return Pin->PinName == UK2Node_GetUParam::TypePinName;
			});
		if (!TypePinPointer)
			return nullptr;

		auto CreateDefaultPinWidget = [&InPin]()
		{
			return SNew(SGraphPin, InPin);
		};

		const UEdGraphPin* TypePin = *TypePinPointer;
		if (!TypePin->DefaultObject || !TypePin->DefaultObject->IsA(UScriptStruct::StaticClass()))
			return CreateDefaultPinWidget();

		const UScriptStruct* ParamType = static_cast<UScriptStruct*>(TypePin->DefaultObject);
		if (!ParamType)
			return CreateDefaultPinWidget();

		TArray<TSharedPtr<FName>> RowNames;
		for (const FName& Name : FParamsRegistry::Get().GetRegisteredNames(ParamType))
		{
			TSharedPtr<FName> RowNameItem = MakeShareable(new FName(Name));
			RowNames.Add(RowNameItem);
		}
		return RowNames.Num() ? SNew(SGraphPinNameList, InPin, RowNames) : CreateDefaultPinWidget();
	}

	return nullptr;
}
