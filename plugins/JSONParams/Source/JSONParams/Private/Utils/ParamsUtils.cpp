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
#include "Utils/ParamsUtils.h"

#include "GameplayTagContainer.h"
#include "Internationalization/StringTableCore.h"
#include "JsonObjectConverter.h"

namespace FParamsUtilsLocal
{
	static const FString TableIdString = TEXT("TableId");
	static const FString KeyString = TEXT("Key");
	static const FString TagNameString = TEXT("tagName");
}
void FParamsUtils::SearchFilesRecursive(
	IPlatformFile* PlatformFile,
	const FString& Path,
	const FString& FileWildCard,
	TArray<FString>& OutFilenames)
{
	auto Visitor = [&OutFilenames, FileWildCard](const TCHAR* FilenameOrDirectory, bool bIsDirectory) -> bool
	{
		if (!bIsDirectory)
		{
			const FString Filename = FPaths::GetCleanFilename(FilenameOrDirectory);
			if (Filename.MatchesWildcard(FileWildCard))
			{
				OutFilenames.AddUnique(FilenameOrDirectory);
			}
		}
		return true;
	};

	PlatformFile->IterateDirectoryRecursively(*Path, Visitor);
}

bool FParamsUtils::LoadJsonFromString(
	const FString& JsonString,
	TArray<FJsonDataWithMeta>& OutObjects,
	const FString& ContextualPath,
	FCriticalSection* Mutex /*= nullptr*/)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*(FString("FParamsUtils::LoadJsonFromString [") + ContextualPath + "]"));

	TSharedPtr<FJsonValue> JSONValue;
	TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*JsonString);
	if (!FJsonSerializer::Deserialize(Reader, JSONValue) || !JSONValue.IsValid())
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsUtils::LoadJsonFromString: Couldn't deserialize JSON! Context: '%s'"),
			*ContextualPath);
		return false;
	}

	auto AddObject = [&](const TSharedPtr<FJsonObject>* Object, int32 Index)
	{
		auto ActualAddObject = [&]()
		{
			OutObjects.Add({ *Object, ContextualPath, Index });
		};

		if (Mutex)
		{
			FScopeLock ScopeLock(Mutex);
			ActualAddObject();
		}
		else
		{
			ActualAddObject();
		}
	};

	const TSharedPtr<FJsonObject>* SingleObject = nullptr;
	if (JSONValue->TryGetObject(SingleObject))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsUtils::LoadJsonFromString: Content must be array! Context: '%s'"),
			*ContextualPath);
		return false;
	}

	const TArray<TSharedPtr<FJsonValue>>* Array = nullptr;
	if (JSONValue->TryGetArray(Array))
	{
		for (int32 Index = 0; Index < Array->Num(); Index++)
		{
			const TSharedPtr<FJsonValue> Val = (*Array)[Index];
			const FString IndexedContext =
				(Array->Num() > 1 ? FString::Format(TEXT("Index={0} in "), { Index }) : "") + ContextualPath;

			const TSharedPtr<FJsonObject>* Object = nullptr;
			if (Val->TryGetObject(Object))
			{
				AddObject(Object, Index);
			}
			else
			{
				UE_LOG(
					LogParams,
					Warning,
					TEXT("FParamsUtils::LoadJsonFromString: invalid JSON Object in objects array! Context: '%s'"),
					*IndexedContext);
			}
		}
		return true;
	}

	UE_LOG(LogParams, Warning, TEXT("FParamsUtils::LoadJsonFromString: invalid JSON file: '%s'"), *ContextualPath);
	return false;
}

bool FParamsUtils::FillDataFromJson(UScriptStruct* Type, TArray<uint8>& Data, TSharedPtr<FJsonObject> JsonData)
{
	Data.SetNumUninitialized(Type->GetStructureSize());
	Type->InitializeStruct(Data.GetData());
	return FJsonObjectConverter::JsonObjectToUStruct(JsonData.ToSharedRef(), Type, Data.GetData(), 0, 0);
}

bool FParamsUtils::ReadJsonArrayFromFile(TArray<TSharedPtr<FJsonValue>>& ParamsArray, const FString& FilePath)
{
	FString JsonString;
	JsonString.Reserve(2048);
	if (!FFileHelper::LoadFileToString(JsonString, &IPlatformFile::GetPlatformPhysical(), *FilePath))
	{
		UE_LOG(LogParams, Error, TEXT("FParamsUtils::ReadJsonArrayFromFile: failed to read file: %s"), *FilePath);
		return false;
	}
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(*JsonString);
	if (!FJsonSerializer::Deserialize(Reader, ParamsArray) || ParamsArray.Num() == 0)
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsUtils::LoadJsonFromString: Couldn't deserialize JSON string from file '%s'"),
			*FilePath);
		return false;
	}

	return true;
}

bool FParamsUtils::WriteJsonArrayToFile(const TArray<TSharedPtr<FJsonValue>>& ParamsArray, const FString& FilePath)
{
	const FString PathPart = FPaths::GetPath(FilePath);
	if (!IPlatformFile::GetPlatformPhysical().CreateDirectoryTree(*PathPart))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsUtils::WriteJsonArrayToFile: Failed to create directory '%s'"),
			*PathPart);
		return false;
	}
	FString OutString;
	const TSharedRef<TJsonWriter<TCHAR>> JsonWriter = TJsonWriterFactory<>::Create(&OutString);
	if (!FJsonSerializer::Serialize(ParamsArray, JsonWriter))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsUtils::WriteJsonArrayToFile: Couldn't serialize JSON object array for writing in file '%s' "),
			*FilePath);
		return false;
	}
	if (!FFileHelper::SaveStringToFile(
			OutString,
			*FilePath,
			FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
			&IFileManager::Get(),
			EFileWrite::FILEWRITE_None))
	{
		UE_LOG(LogParams, Warning, TEXT("FParamsUtils::WriteJsonArrayToFile: Saving file '%s' is failed"), *FilePath);
		return false;
	}
	return true;
}

TSharedPtr<FJsonValue> FParamsUtils::CustomExportToJsonCallback(FProperty* Property, const void* Value)
{
	using namespace FParamsUtilsLocal;

	// Custom export for text from StringTable
	if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		if (TextProperty->GetPropertyValue(Value).IsFromStringTable())
		{
			FName TableId;
			FString Key;
			const FText PropertyValue = TextProperty->GetPropertyValue(Value);
			FTextInspector::GetTableIdAndKey(PropertyValue, TableId, Key);

			// Create json object, add properties for TableId and Key
			TSharedPtr<FJsonObject> TextFromStringTableObject = MakeShared<FJsonObject>();
			TextFromStringTableObject->SetField(TableIdString, MakeShared<FJsonValueString>(TableId.ToString()));
			TextFromStringTableObject->SetField(KeyString, MakeShared<FJsonValueString>(Key));

			return MakeShared<FJsonValueObject>(TextFromStringTableObject);
		}
	}

	return TSharedPtr<FJsonValue>();
}

bool FParamsUtils::CustomImportFromJsonCallback(
	const TSharedPtr<FJsonValue>& JsonValue,
	FProperty* Property,
	void* OutValue)
{
	using namespace FParamsUtilsLocal;

	// Custom import for text from StringTable (stored as a JsonObject)
	if (const FTextProperty* TextProperty = CastField<FTextProperty>(Property))
	{
		const TSharedPtr<FJsonObject>* JsonObject;
		if (JsonValue->TryGetObject(JsonObject))
		{
			// If value for text property is an object - this text is from string table
			FString TableId;
			FString Key;
			if ((*JsonObject)->TryGetStringField(TableIdString, TableId)
				&& (*JsonObject)->TryGetStringField(KeyString, Key))
			{
				const FText Value = FText::FromStringTable(FName(TableId), Key);

				if (!Value.ToString().Equals(FStringTableEntry::GetPlaceholderSourceString()))
				{
					TextProperty->SetPropertyValue(OutValue, Value);
					return true;
				}

				UE_LOG(
					LogParams,
					Error,
					TEXT(
						"FParamsUtils::CustomImportFromJsonCallback:  Couldn't find text for '%s' propery of struct '%s': TableId='%s', Key='%s'"),
					*TextProperty->GetName(),
					*TextProperty->GetOutermost()->GetName(),
					*TableId,
					*Key);
			}
			else
			{
				UE_LOG(
					LogParams,
					Error,
					TEXT(
						"FParamsUtils::CustomImportFromJsonCallback:  Couldn't convert data for '%s' propery of struct '%s', wrong format for text from string table"),
					*TextProperty->GetName(),
					*TextProperty->GetOutermost()->GetName());
			}
		}
	}

	// Gameplay tag container processing
	if (const FStructProperty* StructProperty = CastField<FStructProperty>(Property))
	{
		const TSharedPtr<FJsonObject>* JsonObject;
		if (JsonValue->TryGetObject(JsonObject))
		{
			if (StructProperty->Struct->IsChildOf(FGameplayTagContainer::StaticStruct()))
			{
				if (FJsonObjectConverter::JsonObjectToUStruct(
						JsonObject->ToSharedRef(),
						StructProperty->Struct,
						OutValue))
				{
					FGameplayTagContainer* GameplayTagContainer = static_cast<FGameplayTagContainer*>(OutValue);
					GameplayTagContainer->FillParentTags();
					return true;
				}
			}
		}
	}

	return false;
}

FString FJsonDataWithMeta::GetContext() const
{
	return FString::Format(TEXT("Index={0} {1}"), { ContextualIndex, ContextualPath });
}
