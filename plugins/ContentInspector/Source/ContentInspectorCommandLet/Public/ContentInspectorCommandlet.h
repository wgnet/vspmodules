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

#include "Commandlets/Commandlet.h"
#include "Interfaces/IHttpRequest.h"

#include "ContentInspectorCommandlet.generated.h"

class FJsonObject;
class UInspectionsManager;

USTRUCT()
struct FInspectionInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString InspectionType = "None";

	UPROPERTY()
	bool CriticalInspection = false;

	UPROPERTY()
	FString Description = "None";

	UPROPERTY()
	FString Wiki = "None";
};

USTRUCT()
struct FFailedAssetInfo
{
	GENERATED_BODY()

	UPROPERTY()
	FString AssetName = "None";

	UPROPERTY()
	TArray<FString> Labels {};

	UPROPERTY()
	TArray<FInspectionInfo> FailedInspections {};
};

USTRUCT()
struct FContentInspectorSummary
{
	GENERATED_BODY()

	UPROPERTY()
	FString TimeDate = "None";

	UPROPERTY()
	TArray<FString> ContentPaths {};

	UPROPERTY()
	bool CriticalInspectionsOnly = false;

	UPROPERTY()
	int32 InspectedAssets = 0;

	UPROPERTY()
	int32 FailedAssets = 0;

	UPROPERTY()
	int32 CriticallyFailedAssets = 0;

	UPROPERTY()
	int32 FailedInspections = 0;

	UPROPERTY()
	int32 FailedCriticalInspections = 0;

	UPROPERTY()
	TArray<FFailedAssetInfo> FailedAssetsInfo {};
};


USTRUCT()
struct FBitbucketReportParams
{
	GENERATED_BODY()

	UPROPERTY()
	FString Title = "Content inspection report";

	UPROPERTY()
	FString Result = "FAIL";

	UPROPERTY()
	FString Link = "http://example.com";

	UPROPERTY()
	uint32 CreatedDate = 0;
};

USTRUCT()
struct FInspectedContentCategoryConfig
{
	GENERATED_BODY()

	UPROPERTY()
	FString TopPath = "None";

	UPROPERTY()
	uint32 Depth = 1;

	UPROPERTY()
	TArray<FString> BlackList {};
};


USTRUCT()
struct FContentInspectorCommandletConfig
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FInspectedContentCategoryConfig> Categories {};
};

UCLASS()
class UContentInspectorCommandlet : public UCommandlet
{
	GENERATED_BODY()

	virtual int32 Main(const FString& Params) override;

private:
	FContentInspectorSummary ContentInspectorSummary {};
	FContentInspectorCommandletConfig CommandletConfig {};
	FString OutJsonDirectory {};
	FString RequestAuthorization {};
	FString RequestUrl {};
	FString ReportLink {};
	bool bTimeStampEnabled = false;
	bool bBitbucketReport = false;
	bool bWaitResponse = false;

	void ParseParams(const TCHAR* CmdLine);

	void InspectAsset(const FAssetData& AssetData, UInspectionsManager* InspectionsManager);

	void BitbucketReportPutRequest();
	TSharedPtr<FJsonValueObject> CreateTextDataForBitbucketReport(FString Title, FString Value);
	TSharedPtr<FJsonValueObject> CreateNumberDataForBitbucketReport(FString Title, uint32 Value);
	void OnBitbucketReportPutRequestCompleted(
		FHttpRequestPtr InRequest,
		FHttpResponsePtr InResponse,
		bool bWasSuccessful);
};
