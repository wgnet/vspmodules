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


DEFINE_LOG_CATEGORY_STATIC(LogParams, Log, All)

class FJsonValue;
class FJsonObject;

struct FJsonDataWithMeta
{
	TSharedPtr<FJsonObject> Data;

	// Contextual Path to uparam. You can get the filename from this.
	FString ContextualPath;
	// Index of object if Type==Array. INDEX_NONE - is for Type==Object.
	int32 ContextualIndex = INDEX_NONE;

	FString GetContext() const;
};

namespace FParamsUtils
{
	void SearchFilesRecursive(
		IPlatformFile* PlatformFile,
		const FString& Path,
		const FString& FileWildCard,
		TArray<FString>& OutFilenames);
	bool LoadJsonFromString(
		const FString& JsonString,
		TArray<FJsonDataWithMeta>& OutObjects,
		const FString& ContextualPath,
		FCriticalSection* Mutex = nullptr);
	bool JSONPARAMS_API FillDataFromJson(UScriptStruct* Type, TArray<uint8>& Data, TSharedPtr<FJsonObject> JsonData);
	bool ReadJsonArrayFromFile(TArray<TSharedPtr<FJsonValue>>& ParamsArray, const FString& FilePath);
	bool WriteJsonArrayToFile(const TArray<TSharedPtr<FJsonValue>>& ParamsArray, const FString& FilePath);
	TSharedPtr<FJsonValue> JSONPARAMS_API CustomExportToJsonCallback(FProperty* Property, const void* Value);
	bool CustomImportFromJsonCallback(const TSharedPtr<FJsonValue>& JsonValue, FProperty* Property, void* OutValue);
}
