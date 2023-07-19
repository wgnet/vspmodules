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
#include "Async/Async.h"
#include "UObject/Object.h"

#include "InspectionBase.generated.h"

struct FInspectionResult;
class UInspectionBase;

DECLARE_DELEGATE_OneParam(FOnInspectionCompleteSignature, FInspectionResult);

struct FInspectionResult
{
	enum EInspectionStatus
	{
		NotStarted,
		InProgress,
		Completed
	};

	bool operator==(const FInspectionResult& Other) const
	{
		return AssetPackageName == Other.AssetPackageName && InspectionClass == Other.InspectionClass;
	}

	bool operator!=(const FInspectionResult& Other) const
	{
		return AssetPackageName != Other.AssetPackageName || InspectionClass != Other.InspectionClass;
	}

	EInspectionStatus InspectionStatus = NotStarted;
	FName AssetPackageName = NAME_None;
	TSoftClassPtr<UInspectionBase> InspectionClass = nullptr;
	bool bCriticalInspection = false;
	bool bInspectionFailed = false;
	FName ShortFailedDescription = NAME_None;
	FName FullFailedDescription = NAME_None;
};

UCLASS()
class CONTENTINSPECTOR_API UInspectionBase : public UObject
{
	GENERATED_BODY()

public:
	bool bAssetLoadingRequired = true;
	virtual void CreateInspectionTask(
		const FAssetData& InAssetData,
		FInspectionResult& OutInspectionResult,
		bool bAsynchronous = false,
		bool bCommandLetMode = false);
};

class FInspectionTask
{
public:
	FInspectionTask()
	{
	}

	virtual ~FInspectionTask()
	{
	}

	virtual void DoInspection(
		const FAssetData& InAssetData,
		FInspectionResult& OutInspectionResult,
		bool bAsynchronous = false,
		bool bCommandLetMode = false);
};
