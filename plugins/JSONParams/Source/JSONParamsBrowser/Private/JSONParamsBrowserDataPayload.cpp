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
#include "JSONParamsBrowserDataPayload.h"

#include "ParamsRegistry.h"
#include "ParamsRegistryLibrary.h"

FJSONParamsBrowserDataPayload::FJSONParamsBrowserDataPayload(const FName ParamName, UScriptStruct* ParamType)
	: Name(ParamName)
	, Type(ParamType)
{
	ParamRegistryDataPtr = FParamsRegistry::Get().GetParam(ParamType, ParamName);
}

FName FJSONParamsBrowserDataPayload::GetParamName() const
{
	return Name;
}

UScriptStruct* FJSONParamsBrowserDataPayload::GetParamType() const
{
	return Type;
}

FString FJSONParamsBrowserDataPayload::GetParamPhysicalPath() const
{
	const FParamRegistryMeta* Meta = GetMeta();
	return Meta != nullptr ? Meta->FilePath : TEXT_EMPTY;
}

int32 FJSONParamsBrowserDataPayload::GetParamIndexInFile() const
{
	const FParamRegistryMeta* Meta = GetMeta();
	return Meta != nullptr ? Meta->ParamIndex : 0;
}

bool FJSONParamsBrowserDataPayload::IsParamChanged() const
{
	const FParamRegistryMeta* Meta = GetMeta();
	return Meta != nullptr ? Meta->bIsChanged : false;
}

bool FJSONParamsBrowserDataPayload::IsParamSavable() const
{
	const FParamRegistryMeta* Meta = GetMeta();
	return Meta != nullptr ? Meta->bIsSavable : false;
}

bool FJSONParamsBrowserDataPayload::IsParamValid() const
{
	return ParamRegistryDataPtr.IsValid();
}

void FJSONParamsBrowserDataPayload::UpdateThumbnail(FAssetThumbnail& InThumbnail) const
{
	if (const TSharedPtr<FParamRegistryData> ParamRegistryDataSharedPtr = ParamRegistryDataPtr.Pin())
	{
		const FAssetData tempAssetData(
			NAME_None,
			NAME_None,
			ParamRegistryDataSharedPtr->Info.Name,
			ParamRegistryDataSharedPtr->Info.Type->GetFName());
		InThumbnail.SetAsset(tempAssetData);
	}
}

const FParamRegistryMeta* FJSONParamsBrowserDataPayload::GetMeta() const
{
	if (const TSharedPtr<FParamRegistryData> ParamRegistryDataSharedPtr = ParamRegistryDataPtr.Pin())
		return &ParamRegistryDataSharedPtr->Meta;

	return nullptr;
}
