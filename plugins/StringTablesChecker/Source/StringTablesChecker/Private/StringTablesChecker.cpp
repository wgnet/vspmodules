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
#include "StringTablesChecker.h"
#include "StringTablesCheckerSettings.h"

#include "VSPCheck.h"

#include "AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "Engine/UserDefinedStruct.h"
#include "Internationalization/Regex.h"
#include "Internationalization/StringTable.h"
#include "Internationalization/StringTableRegistry.h"
#include "JsonObjectConverter.h"
#include "Kismet/KismetStringTableLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/ScopedSlowTask.h"
#include "UserDefinedStructure/UserDefinedStructEditorData.h"

#define LOCTEXT_NAMESPACE "FStringTablesChecker"

DEFINE_LOG_CATEGORY_STATIC(LogStringTablesChecker, Log, All);

namespace FStringTablesCheckerLocal
{
	constexpr int32 AssetsPerBatch = 1000;

	const FString ContentPackagePath = "/Game";
	const FString DefaultJsonDirectoryName = "StringTablesChecker";
	const FString JsonTitle = "StringTablesCheckReport";

	const FString PatternStart = "^";
	const FString PatternEnd = "$";
	const FString AnySymbols = "*";
	const FString PatternAnySymbols = ".*";

	const FName UserDefinedStructTextCategory = TEXT("text"); // Must match UEdGraphSchema_K2::PC_Text
}

class FTextFromStringTableCheckingArchive : public FArchiveUObject
{
public:
	FTextFromStringTableCheckingArchive(
		UPackage* InPackage,
		TMap<FString, TMap<FString, TArray<FString>>>& InOutStringTablesKeysTree)
		: StringTablesKeysTree(&InOutStringTablesKeysTree)
	{
		this->SetIsSaving(true);

		TArray<UObject*> AllObjectsInPackage;
		GetObjectsWithOuter(InPackage, AllObjectsInPackage, true, RF_Transient, EInternalObjectFlags::PendingKill);

		for (UObject* Obj : AllObjectsInPackage)
		{
			ProcessedObj = Obj;
			ProcessObject(Obj);
		}
	}

	void ProcessObject(UObject* Obj)
	{
		// User Defined Structs need some special handling as they store their default data in a way that serialize doesn't pick up
		if (UUserDefinedStruct* const UserDefinedStruct = Cast<UUserDefinedStruct>(Obj))
		{
			if (UUserDefinedStructEditorData* UDSEditorData =
					Cast<UUserDefinedStructEditorData>(UserDefinedStruct->EditorData))
			{
				for (FStructVariableDescription& StructVariableDesc : UDSEditorData->VariablesDescriptions)
				{
					if (StructVariableDesc.Category == FStringTablesCheckerLocal::UserDefinedStructTextCategory)
					{
						FText StructVariableValue;
						if (FTextStringHelper::ReadFromBuffer(*StructVariableDesc.DefaultValue, StructVariableValue))
						{
							KeyText(StructVariableValue);
						}
					}
				}
			}
		}

		Obj->Serialize(*this);
	}

	bool KeyText(FText& InText)
	{
		if (!InText.IsFromStringTable())
			return false;

		FName TableId;
		FString Key;
		if (FStringTableRegistry::Get().FindTableIdAndKey(InText, TableId, Key))
		{
			if (TMap<FString, TArray<FString>>* Keys = StringTablesKeysTree->Find(TableId.ToString()))
			{
				if (TArray<FString>* KeyUsages = Keys->Find(Key))
				{
					KeyUsages->AddUnique(ProcessedObj->GetPathName());
				}
				else
				{
					TArray<FString> NewKey { ProcessedObj->GetPathName() };
					Keys->Add(Key, NewKey);
				}
			}
			else
			{
				TMap<FString, TArray<FString>> NewTable {};
				NewTable.Add(Key, { ProcessedObj->GetPathName() });
				StringTablesKeysTree->Add(TableId.ToString(), NewTable);
			}
		}
		else
		{
			UE_LOG(LogStringTablesChecker, Display, TEXT("Impossible thing has happened"));
		}
		return true;
	}

	using FArchiveUObject::operator<<; // For visibility of the overloads we don't override

	virtual FArchive& operator<<(FText& Value) override
	{
		KeyText(Value);
		return *this;
	}

private:
	TMap<FString, TMap<FString, TArray<FString>>>* StringTablesKeysTree = nullptr;
	UObject* ProcessedObj = nullptr;
};

FStringTablesUsage UStringTablesChecker::CheckStringTablesUsage()
{
	using namespace FStringTablesCheckerLocal;

	FStringTablesUsage OutUsages;
	TArray<FAssetData> AssetList;
	FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	Filter.bRecursiveClasses = true;

	// Get all assets we need
	{
		TArray<FString> AssetsPaths = UStringTablesCheckerSettings::Get()->GetAssetsPaths();
		if (AssetsPaths.Num() != 0)
		{
			for (auto AssetsPath : AssetsPaths)
			{
				VSPCheckContinueF(
					FPaths::DirectoryExists(FPaths::ProjectContentDir() / AssetsPath),
					*FString::Printf(TEXT("Content path '%s' from settings doesn't exist"), *AssetsPath));

				Filter.PackagePaths.Add(FName(ContentPackagePath / AssetsPath));
			}
		}
		if (Filter.PackagePaths.Num() == 0)
		{
			UE_LOG(
				LogStringTablesChecker,
				Warning,
				TEXT(
					"Assets paths for search are not set. Please check settings. Full content directory will be searched."));
			Filter.PackagePaths.Add(FName(ContentPackagePath));
		}

		TArray<FSoftClassPath> SupportedClasses = UStringTablesCheckerSettings::Get()->GetSupportedAssetsClasses();
		if (SupportedClasses.Num() != 0)
		{
			for (auto SupportedClass : SupportedClasses)
			{
				VSPCheckContinueF(
					SupportedClass,
					*FString::Printf(TEXT("Asset class '%s' from settings doesn't exist"), *SupportedClass.ToString()));
				UClass* FoundSupportedClass = SupportedClass.ResolveClass();
				VSPCheckContinueF(
					IsValid(FoundSupportedClass),
					*FString::Printf(TEXT("Class '%s' from settings is not loaded"), *SupportedClass.ToString()));

				Filter.ClassNames.Add(FoundSupportedClass->GetFName());
			}
		}
		if (Filter.ClassNames.Num() == 0)
		{
			UE_LOG(
				LogStringTablesChecker,
				Warning,
				TEXT(
					"Supported classes are not set! Please check settings. Data assets, data tables and blueprints will be checked."));
			Filter.ClassNames.Add(UBlueprint::StaticClass()->GetFName());
			Filter.ClassNames.Add(UDataAsset::StaticClass()->GetFName());
			Filter.ClassNames.Add(UDataTable::StaticClass()->GetFName());
		}


		AssetRegistryModule.Get().GetAssets(Filter, AssetList);
	}


	// Collect text from string tables entries and usages from assets
	TMap<FString, TMap<FString, TArray<FString>>> UsedStringTablesKeysTree {};
	{
		FScopedSlowTask CheckingSlowTask(AssetList.Num());
		CheckingSlowTask.MakeDialog();

		const int32 AssetsNum = AssetList.Num();
		for (int32 AssetIndex = 0; AssetIndex < AssetsNum;)
		{
			const int32 AssetBatchEndIndex = FMath::Min(AssetIndex + AssetsPerBatch, AssetsNum);
			for (; AssetIndex < AssetBatchEndIndex; AssetIndex++)
			{
				FAssetData& Asset = AssetList[AssetIndex];

				FText ValidatingMessage = FText::Format(
					LOCTEXT("CheckingAssetName", "Text checking in asset: {0}"),
					FText::FromName(Asset.AssetName));
				CheckingSlowTask.EnterProgressFrame(1, ValidatingMessage);

				UE_LOG(
					LogStringTablesChecker,
					Display,
					TEXT("Loading asset package %d of %d: '%s'."),
					AssetIndex + 1,
					AssetsNum,
					*Asset.GetFullName());
				UPackage* Package = Asset.GetPackage();
				if (!Package)
				{
					UE_LOG(
						LogStringTablesChecker,
						Warning,
						TEXT("Failed to load package from: '%s'."),
						*Asset.GetFullName());
				}
				else
				{
					FTextFromStringTableCheckingArchive(Package, UsedStringTablesKeysTree);
				}
			}
			CollectGarbage(RF_NoFlags);
		}
	}

	// Get used string tables IDs
	TArray<FString> UsedStringTablesID {};
	UsedStringTablesKeysTree.GetKeys(UsedStringTablesID);

	// Get all string tables IDs
	TArray<FString> StringTablesID {};
	{
		AssetList.Empty();
		Filter.PackagePaths.Empty();
		TArray<FString> StringTablesPaths = UStringTablesCheckerSettings::Get()->GetStringTablesPaths();
		for (auto AssetsPath : StringTablesPaths)
		{
			VSPCheckContinueF(
				FPaths::DirectoryExists(FPaths::ProjectContentDir() / AssetsPath),
				*FString::Printf(TEXT("String table content path '%s' from settings doesn't exist"), *AssetsPath));

			Filter.PackagePaths.Add(FName(ContentPackagePath / AssetsPath));
		}
		if (Filter.PackagePaths.Num() == 0)
		{
			UE_LOG(
				LogStringTablesChecker,
				Warning,
				TEXT(
					"String tables paths for search are not set. Please check settings. Full content directory will be searched."));
			Filter.PackagePaths.Add(FName(ContentPackagePath));
		}
		Filter.ClassNames.Empty();
		Filter.ClassNames.Add(UStringTable::StaticClass()->GetFName());
		AssetRegistryModule.Get().GetAssets(Filter, AssetList);

		TArray<FString> ExclusionsPatterns {};
		GetStringTablesExclusionsPatterns(ExclusionsPatterns);
		for (auto Asset : AssetList)
		{
			if (IsStringTableNeedExclude(Asset.AssetName, ExclusionsPatterns))
				continue;

			UObject* AssetObj = Asset.GetAsset();
			if (UStringTable* StringTable = Cast<UStringTable>(AssetObj))
				StringTablesID.Add(StringTable->GetStringTableId().ToString());
		}
	}

	// Check string tables usages
	for (auto TableId : UsedStringTablesID)
	{
		FAllStringTableKeysUsage TableInfo;
		TableInfo.TableID = TableId;
		TMap<FString, TArray<FString>>* UsedKeysMap = UsedStringTablesKeysTree.Find(TableId);

		int32 StringTableIndex = StringTablesID.Find(TableId);
		if (StringTableIndex != INDEX_NONE)
		{
			TArray<FString> TableKeys = UKismetStringTableLibrary::GetKeysFromStringTable(FName(TableId));

			for (auto UsedKey : *UsedKeysMap)
			{
				FStringTableKeyUsage KeyUsage;
				KeyUsage.Key = UsedKey.Key;
				KeyUsage.Usages = UsedKey.Value;

				int32 KeyIndex = TableKeys.Find(UsedKey.Key);
				if (KeyIndex != INDEX_NONE)
				{
					TableInfo.UsedKeys.Add(KeyUsage);
					TableKeys.RemoveAt(KeyIndex);
				}
				else
				{
					TableInfo.MissingKeys.Add(KeyUsage);
				}
			}

			TableInfo.UnusedKeys.Append(TableKeys);
			OutUsages.UsedTables.Add(TableInfo);
			StringTablesID.RemoveAt(StringTableIndex);
		}
		else
		{
			for (auto UsedKey : *UsedKeysMap)
			{
				FStringTableKeyUsage KeyUsage;
				KeyUsage.Key = UsedKey.Key;
				KeyUsage.Usages = UsedKey.Value;
				TableInfo.MissingKeys.Add(KeyUsage);
			}
			OutUsages.MissingTables.Add(TableInfo);
		}
	}
	OutUsages.UnusedTables.Append(StringTablesID);

	return OutUsages;
}

FStringTablesUsage UStringTablesChecker::LoadStringTablesUsageFromJson()
{
	using namespace FStringTablesCheckerLocal;

	FStringTablesUsage StringTablesUsage {};

	// load file
	const FString FileName = FPaths::ProjectSavedDir() / DefaultJsonDirectoryName / JsonTitle + FString(".json");
	FString JsonFile;
	if (!FFileHelper::LoadFileToString(JsonFile, *FileName))
	{
		UE_LOG(LogStringTablesChecker, Warning, TEXT("JSON file loading is failed"));
		return StringTablesUsage;
	}

	// parse as json
	const TSharedRef<TJsonReader<>> JsonReader = TJsonReaderFactory<>::Create(JsonFile);
	TSharedPtr<FJsonValue> JsonValue;
	if (!FJsonSerializer::Deserialize(JsonReader, JsonValue))
	{
		UE_LOG(LogStringTablesChecker, Warning, TEXT("Import data from JSON is failed"));
		return StringTablesUsage;
	}

	FJsonObjectConverter::JsonObjectToUStruct<FStringTablesUsage>(
		JsonValue->AsObject().ToSharedRef(),
		&StringTablesUsage);
	return StringTablesUsage;
}

void UStringTablesChecker::SaveStringTablesUsageToJson(const FStringTablesUsage& StringTablesUsage)
{
	using namespace FStringTablesCheckerLocal;

	IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	const FString JsonOutDirectory = FPaths::ProjectSavedDir() / DefaultJsonDirectoryName;

	if (PlatformFile.CreateDirectoryTree(*JsonOutDirectory))
	{
		const FString FileName = JsonOutDirectory / JsonTitle + FString(".json");

		FString StringTablesCheckerReportJSON;
		FJsonObjectConverter::UStructToJsonObjectString(StringTablesUsage, StringTablesCheckerReportJSON);

		if (!FFileHelper::SaveStringToFile(
				StringTablesCheckerReportJSON,
				*FileName,
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
				&IFileManager::Get()))
		{
			UE_LOG(
				LogStringTablesChecker,
				Warning,
				TEXT("Saving string tables checking result to JSON file is failed"));
		}
	}
	else
	{
		UE_LOG(LogStringTablesChecker, Warning, TEXT("String tables checking result directory creation is failed"));
	}
}

void UStringTablesChecker::GetStringTablesExclusionsPatterns(TArray<FString>& OutExclusionsPatterns)
{
	using namespace FStringTablesCheckerLocal;

	for (const FString& Exclusion : UStringTablesCheckerSettings::Get()->GetStringTablesExclusions())
		OutExclusionsPatterns.Add(PatternStart + Exclusion.Replace(*AnySymbols, *PatternAnySymbols) + PatternEnd);
}

bool UStringTablesChecker::IsStringTableNeedExclude(
	const FName StringTableAssetName,
	const TArray<FString>& ExclusionsPatterns)
{
	for (const FString& PatternString : ExclusionsPatterns)
	{
		FRegexPattern RegexPattern(PatternString);
		FRegexMatcher RegexMatcher(RegexPattern, StringTableAssetName.ToString());

		if (RegexMatcher.FindNext())
			return true;
	}

	return false;
}
