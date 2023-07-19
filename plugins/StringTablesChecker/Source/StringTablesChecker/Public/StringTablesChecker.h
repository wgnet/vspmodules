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
#include "Kismet/BlueprintFunctionLibrary.h"

#include "StringTablesChecker.generated.h"

USTRUCT(BlueprintType)
struct FStringTableKeyUsage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString Key = "None";

	UPROPERTY(BlueprintReadWrite)
	TArray<FString> Usages {};
};

USTRUCT(BlueprintType)
struct FAllStringTableKeysUsage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	FString TableID = "None";

	UPROPERTY(BlueprintReadWrite)
	TArray<FStringTableKeyUsage> UsedKeys {};

	UPROPERTY(BlueprintReadWrite)
	TArray<FStringTableKeyUsage> MissingKeys {};

	UPROPERTY(BlueprintReadWrite)
	TArray<FString> UnusedKeys {};
};

USTRUCT(BlueprintType)
struct FStringTablesUsage
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TArray<FAllStringTableKeysUsage> UsedTables {};

	UPROPERTY(BlueprintReadWrite)
	TArray<FAllStringTableKeysUsage> MissingTables {};

	UPROPERTY(BlueprintReadWrite)
	TArray<FString> UnusedTables {};
};

UCLASS()
class STRINGTABLESCHECKER_API UStringTablesChecker : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Utilities|String Table Checker")
	static FStringTablesUsage CheckStringTablesUsage();

	UFUNCTION(BlueprintCallable, Category = "Utilities|String Table Checker")
	static FStringTablesUsage LoadStringTablesUsageFromJson();

	UFUNCTION(BlueprintCallable, Category = "Utilities|String Table Checker")
	static void SaveStringTablesUsageToJson(const FStringTablesUsage& StringTablesUsage);

private:
	static void GetStringTablesExclusionsPatterns(TArray<FString>& OutExclusionsPatterns);
	static bool IsStringTableNeedExclude(const FName StringTableAssetName, const TArray<FString>& ExclusionsPatterns);
};
