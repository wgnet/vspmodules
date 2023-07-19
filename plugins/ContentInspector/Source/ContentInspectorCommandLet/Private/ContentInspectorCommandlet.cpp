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
#include "ContentInspectorCommandlet.h"

#include "AssetRegistryModule.h"
#include "ContentInspectorSettings.h"
#include "HttpManager.h"
#include "HttpModule.h"
#include "InspectionsManager.h"
#include "Interfaces/IHttpRequest.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"

DEFINE_LOG_CATEGORY_STATIC(LogContentInspectorCommandlet, Log, All)

namespace FContentInspectorCommandletLocal
{
	const FString PathParamPrefix = "-p=";
	const FString OutputParamPrefix = "-o=";
	const FString AuthParamPrefix = "-a=";
	const FString BuildIdParamPrefix = "-b=";
	const FString CommitIdParamPrefix = "-i=";
	const FString OnlyCriticalInspectionsToken = "-c";
	const FString TimeStampEnabledToken = "-t";

	const FString ContentPackagePath = "/Game";
	const FString DefaultRelativeJsonOutDirectoryName = "ContentInspections";
	const FString OutJsonTitle = "ContentInspection";
	const FString CommandletConfigFile = "ContentInspector/Config/CommandletConfig.json";

	const FString ResultFieldValuePass = "PASS";
	const FString ResultFieldValueFail = "FAIL";
	const FString DataArrayFieldName = "data";
	const FString DataTitleFieldName = "title";
	const FString DataValueFieldName = "value";
	const FString DataTypeFieldName = "type";
	const FString DataNumberTypeValue = "NUMBER";
	const FString DataTextTypeValue = "TEXT";

	const FString InspectedMultiplePathsValue = "Multiple paths";
	const FString InspectedContentTitle = "Inspected content";
	const FString InspectedAssetsTitle = "Inspected assets";
	const FString FailedAssetsTitle = "Failed assets";
	const FString CriticallyFailedAssetsTitle = "Critically failed assets";
	const FString FailedInspectionsTitle = "Failed inspections";
	const FString FailedCriticalInspectionsTitle = "Failed critical inspections";

	const FString ReportLinkBeforeBuildInfo = "http://localhost/repository/download/";
	const FString ReportLinkAfterBuildInfo = "/report/ContentInspection.json";

	const FString HttpRequestVerb = "PUT";
	const FString HttpRequestHeaderContentTitle = "Content-Type";
	const FString HttpRequestHeaderContentType = "application/json";
	const FString HttpRequestHeaderAuthTitle = "Authorization";
	const FString HttpRequestHeaderAuthType = "Basic ";
	const FString HttpRequestUrlBeforeCommitId =
		"";
	const FString HttpRequestUrlAfterCommitId = "/reports/ContentInspectionReport";

	constexpr float HttpRequestTimeout = 60.f;
	constexpr float HttpRequestTickDelta = 1.f;

	constexpr uint32 MaxLabelsCount = 3;

	void NormalizeQuotedDirectoryName(FString& DirectoryString)
	{
		if (DirectoryString.StartsWith("\"") && DirectoryString.EndsWith("\""))
			DirectoryString = DirectoryString.Mid(1, DirectoryString.Len() - 2);
		FPaths::NormalizeDirectoryName(DirectoryString);
	}
}

int32 UContentInspectorCommandlet::Main(const FString& Params)
{
	using namespace FContentInspectorCommandletLocal;

	UInspectionsManager* InspectionsManager = NewObject<UInspectionsManager>();
	if (!InspectionsManager)
	{
		UE_LOG(
			LogContentInspectorCommandlet,
			Warning,
			TEXT("Content Inspection has been skipped: InspectionsManager loading failed"));
		return -1;
	}
	UE_LOG(LogContentInspectorCommandlet, Display, TEXT("Content Inspection started"));
	InspectionsManager->SetCommandLetMode(true);

	// Get params
	ParseParams(*Params);

	const FString CommandletConfigFileName = FPaths::ProjectPluginsDir() / CommandletConfigFile;
	FString CommandletConfigJsonFile;
	if (FFileHelper::LoadFileToString(CommandletConfigJsonFile, *CommandletConfigFileName))
	{
		const TSharedRef<TJsonReader<> > JsonReader = TJsonReaderFactory<>::Create(CommandletConfigJsonFile);
		TSharedPtr<FJsonValue> JsonValue;
		if (FJsonSerializer::Deserialize(JsonReader, JsonValue))
		{
			FJsonObjectConverter::JsonObjectToUStruct<FContentInspectorCommandletConfig>(
				JsonValue->AsObject().ToSharedRef(),
				&CommandletConfig);
		}
	}

	// Get inspected assets
	FARFilter Filter;
	Filter.bRecursivePaths = true;
	if (ContentInspectorSummary.ContentPaths.Num() == 0)
	{
		ContentInspectorSummary.ContentPaths.Add("AllContent");
		Filter.PackagePaths.Add(FName(ContentPackagePath));
	}
	else
	{
		for (FString PackagePath : ContentInspectorSummary.ContentPaths)
			Filter.PackagePaths.Add(FName(ContentPackagePath / PackagePath));
	}
	const FAssetRegistryModule& AssetRegistryModule =
		FModuleManager::Get().LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
	TArray<FAssetData> AssetList;
	AssetRegistryModule.Get().GetAssets(Filter, AssetList);

	// Inspect assets
	for (const FAssetData& Asset : AssetList)
		InspectAsset(Asset, InspectionsManager);

	ContentInspectorSummary.TimeDate = FDateTime::Now().ToString();
	ContentInspectorSummary.FailedAssets = ContentInspectorSummary.FailedAssetsInfo.Num();
	ContentInspectorSummary.InspectedAssets = AssetList.Num();

	// Convert and save inspections summary to JSON
	if (OutJsonDirectory.IsEmpty())
		OutJsonDirectory = FPaths::ProjectSavedDir() / DefaultRelativeJsonOutDirectoryName;
	FString ContentInspectorSummaryJSON;
	FJsonObjectConverter::UStructToJsonObjectString(ContentInspectorSummary, ContentInspectorSummaryJSON);

	static IPlatformFile& PlatformFile = FPlatformFileManager::Get().GetPlatformFile();
	if (PlatformFile.CreateDirectoryTree(*OutJsonDirectory))
	{
		const FString FileName = bTimeStampEnabled
			? OutJsonDirectory / OutJsonTitle + "_" + ContentInspectorSummary.TimeDate + FString(".json")
			: OutJsonDirectory / OutJsonTitle + FString(".json");

		if (!FFileHelper::SaveStringToFile(
				ContentInspectorSummaryJSON,
				*FileName,
				FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,
				&IFileManager::Get()))
		{
			UE_LOG(LogContentInspectorCommandlet, Warning, TEXT("Saving to file is failed"));
		}
	}
	else
	{
		UE_LOG(LogContentInspectorCommandlet, Warning, TEXT("Inspections directory creation is failed"));
	}

	if (bBitbucketReport)
		BitbucketReportPutRequest();

	UE_LOG(LogContentInspectorCommandlet, Display, TEXT("Content Inspection finished"));
	return 0;
}

void UContentInspectorCommandlet::ParseParams(const TCHAR* CmdLine)
{
	using namespace FContentInspectorCommandletLocal;

	FString NextToken;
	while (FParse::Token(CmdLine, NextToken, false))
	{
		if (NextToken == OnlyCriticalInspectionsToken)
		{
			ContentInspectorSummary.CriticalInspectionsOnly = true;
		}
		else if (NextToken == TimeStampEnabledToken)
		{
			bTimeStampEnabled = true;
		}
		else if (NextToken.StartsWith(PathParamPrefix))
		{
			FString PathString = NextToken.RightChop(PathParamPrefix.Len());
			NormalizeQuotedDirectoryName(PathString);

			if (FPaths::DirectoryExists(FPaths::ProjectContentDir() / PathString))
				ContentInspectorSummary.ContentPaths.Add(PathString);
			else
				UE_LOG(LogContentInspectorCommandlet, Warning, TEXT("Content path '%s' doesn't exist"), *PathString);
		}
		else if (NextToken.StartsWith(OutputParamPrefix))
		{
			FString JsonOutDirectoryString = NextToken.RightChop(OutputParamPrefix.Len());
			NormalizeQuotedDirectoryName(JsonOutDirectoryString);

			if (FPaths::IsRelative(JsonOutDirectoryString))
				OutJsonDirectory = FPaths::ProjectDir() / JsonOutDirectoryString;
			else
				OutJsonDirectory = JsonOutDirectoryString;
		}
		else if (NextToken.StartsWith(AuthParamPrefix))
		{
			RequestAuthorization = NextToken.RightChop(AuthParamPrefix.Len());
		}
		else if (NextToken.StartsWith(BuildIdParamPrefix))
		{
			ReportLink =
				ReportLinkBeforeBuildInfo + NextToken.RightChop(BuildIdParamPrefix.Len()) + ReportLinkAfterBuildInfo;
		}
		else if (NextToken.StartsWith(CommitIdParamPrefix))
		{
			RequestUrl = HttpRequestUrlBeforeCommitId + NextToken.RightChop(CommitIdParamPrefix.Len())
				+ HttpRequestUrlAfterCommitId;
		}
	}

	if (!RequestAuthorization.IsEmpty() && !ReportLink.IsEmpty() && !RequestUrl.IsEmpty())
	{
		bBitbucketReport = true;
	}
}

void UContentInspectorCommandlet::InspectAsset(const FAssetData& AssetData, UInspectionsManager* InspectionsManager)
{
	using namespace FContentInspectorCommandletLocal;

	FString AssetPackagePath = AssetData.PackagePath.ToString().RightChop(ContentPackagePath.Len() + 1);
	FInspectedContentCategoryConfig* Category = nullptr;
	for (FInspectedContentCategoryConfig& CategoryConfig : CommandletConfig.Categories)
	{
		if (AssetPackagePath.StartsWith(CategoryConfig.TopPath))
		{
			for (FString SkippedPath : CategoryConfig.BlackList)
			{
				if (AssetPackagePath.StartsWith(CategoryConfig.TopPath / SkippedPath))
					return;
			}
			Category = &CategoryConfig;
			break;
		}
	}

	uint32 FailedCriticalInspections = 0;
	FFailedAssetInfo FailedAssetInfo {};
	TArray<FInspectionResult> AssetInspectionsResult {};

	InspectionsManager->SendInspectionsRequest(AssetData);
	InspectionsManager->GetInspectionResults(AssetData.PackageName, AssetInspectionsResult);

	for (const FInspectionResult& AssetInspectionResult : AssetInspectionsResult)
	{
		if (!AssetInspectionResult.bInspectionFailed)
			continue;

		if (AssetInspectionResult.bCriticalInspection)
		{
			FailedCriticalInspections++;
		}
		else if (ContentInspectorSummary.CriticalInspectionsOnly)
		{
			continue;
		}
		FInspectionInfo InspectionInfo {};
		InspectionInfo.CriticalInspection = AssetInspectionResult.bCriticalInspection;
		InspectionInfo.InspectionType = AssetInspectionResult.InspectionClass->GetName();
		InspectionInfo.Description = AssetInspectionResult.FullFailedDescription.ToString();
		InspectionInfo.Wiki =
			UContentInspectorSettings::Get()->GetConfluenceURL(AssetInspectionResult.InspectionClass).ToString();
		FailedAssetInfo.FailedInspections.Add(InspectionInfo);
	}

	if (FailedAssetInfo.FailedInspections.Num() > 0)
	{
		FString ClassName;
		FString PackageName;
		FString ObjectName;
		FString SubObjectName;
		FPackageName::SplitFullObjectPath(AssetData.GetFullName(), ClassName, PackageName, ObjectName, SubObjectName);
		FailedAssetInfo.AssetName =
			FPaths::GetBaseFilename(PackageName) == ObjectName ? PackageName : AssetData.ObjectPath.ToString();

		if (Category)
		{
			for (uint32 DepthLevel = 0; DepthLevel < Category->Depth; DepthLevel++)
			{
				uint32 NextLabelCharacterIndex = INDEX_NONE;
				FString Label;
				bool bIsLastCategoryLabel = false;

				if (DepthLevel > 0)
				{
					NextLabelCharacterIndex = AssetPackagePath.Find("/");
				}
				else
				{
					if (Category->TopPath.Len() != AssetPackagePath.Len())
						NextLabelCharacterIndex = Category->TopPath.Len();
				}

				if (NextLabelCharacterIndex != INDEX_NONE)
				{
					Label = AssetPackagePath.Left(NextLabelCharacterIndex);
					AssetPackagePath = AssetPackagePath.RightChop(NextLabelCharacterIndex + 1);
				}
				else
				{
					Label = AssetPackagePath;
					bIsLastCategoryLabel = true;
				}

				if (DepthLevel == 0)
				{
					FailedAssetInfo.Labels.Add("Content inspections: " + Label);
				}
				else if (DepthLevel < MaxLabelsCount)
				{
					FailedAssetInfo.Labels.Add(Label);
				}
				else
				{
					FailedAssetInfo.Labels.Last() /= Label;
				}

				if (bIsLastCategoryLabel)
					break;
			}
		}
		else
		{
			FailedAssetInfo.Labels.Add("Unknown category");
		}

		ContentInspectorSummary.FailedAssetsInfo.Add(FailedAssetInfo);
		ContentInspectorSummary.FailedInspections += FailedAssetInfo.FailedInspections.Num();
		if (FailedCriticalInspections)
		{
			ContentInspectorSummary.CriticallyFailedAssets++;
			ContentInspectorSummary.FailedCriticalInspections += FailedCriticalInspections;
		}
	}
}

void UContentInspectorCommandlet::BitbucketReportPutRequest()
{
	using namespace FContentInspectorCommandletLocal;

	TArray<TSharedPtr<FJsonValue> > DataArray;
	const FString InspectedContentValue = ContentInspectorSummary.ContentPaths.Num() == 1
		? ContentInspectorSummary.ContentPaths[0]
		: InspectedMultiplePathsValue;
	DataArray.Add(CreateTextDataForBitbucketReport(InspectedContentTitle, InspectedContentValue));
	DataArray.Add(CreateNumberDataForBitbucketReport(InspectedAssetsTitle, ContentInspectorSummary.InspectedAssets));
	DataArray.Add(CreateNumberDataForBitbucketReport(FailedAssetsTitle, ContentInspectorSummary.FailedAssets));
	DataArray.Add(CreateNumberDataForBitbucketReport(
		CriticallyFailedAssetsTitle,
		ContentInspectorSummary.CriticallyFailedAssets));
	DataArray.Add(
		CreateNumberDataForBitbucketReport(FailedInspectionsTitle, ContentInspectorSummary.FailedInspections));
	DataArray.Add(CreateNumberDataForBitbucketReport(
		FailedCriticalInspectionsTitle,
		ContentInspectorSummary.FailedCriticalInspections));

	FBitbucketReportParams BitbucketReportParams {};
	BitbucketReportParams.CreatedDate = FDateTime::Now().ToUnixTimestamp();
	BitbucketReportParams.Link = ReportLink;
	BitbucketReportParams.Result =
		(ContentInspectorSummary.CriticallyFailedAssets == 0) ? ResultFieldValuePass : ResultFieldValueFail;

	const TSharedPtr<FJsonObject> BitbucketReportContent =
		FJsonObjectConverter::UStructToJsonObject(BitbucketReportParams);
	BitbucketReportContent->SetArrayField(DataArrayFieldName, DataArray);

	FString OutputString;
	TSharedRef<TJsonWriter<TCHAR> > JsonWriter = TJsonWriterFactory<>::Create(&OutputString);
	FJsonSerializer::Serialize(BitbucketReportContent.ToSharedRef(), JsonWriter);

	FHttpModule* Http = &FHttpModule::Get();
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> HttpRequest = Http->CreateRequest();

	bWaitResponse = true;
	HttpRequest->SetTimeout(HttpRequestTimeout);
	HttpRequest->SetVerb(HttpRequestVerb);
	HttpRequest->SetHeader(HttpRequestHeaderContentTitle, HttpRequestHeaderContentType);
	HttpRequest->SetHeader(
		HttpRequestHeaderAuthTitle,
		HttpRequestHeaderAuthType + FBase64::Encode(RequestAuthorization));
	HttpRequest->SetURL(RequestUrl);
	HttpRequest->SetContentAsString(OutputString);
	HttpRequest->OnProcessRequestComplete().BindUObject(
		this,
		&UContentInspectorCommandlet::OnBitbucketReportPutRequestCompleted);
	HttpRequest->ProcessRequest();
	UE_LOG(LogContentInspectorCommandlet, Verbose, TEXT("Bitbucket put request has been sent: '%s'"), *OutputString);

	while (bWaitResponse)
	{
		Http->GetHttpManager().Tick(HttpRequestTickDelta);
		FPlatformProcess::Sleep(HttpRequestTickDelta);
	}
}

TSharedPtr<FJsonValueObject> UContentInspectorCommandlet::CreateTextDataForBitbucketReport(FString Title, FString Value)
{
	using namespace FContentInspectorCommandletLocal;

	auto JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(DataTitleFieldName, Title);
	JsonObj->SetStringField(DataValueFieldName, Value);
	JsonObj->SetStringField(DataTypeFieldName, DataTextTypeValue);

	return MakeShared<FJsonValueObject>(JsonObj);
}

TSharedPtr<FJsonValueObject> UContentInspectorCommandlet::CreateNumberDataForBitbucketReport(
	FString Title,
	uint32 Value)
{
	using namespace FContentInspectorCommandletLocal;

	auto JsonObj = MakeShared<FJsonObject>();
	JsonObj->SetStringField(DataTitleFieldName, Title);
	JsonObj->SetNumberField(DataValueFieldName, Value);
	JsonObj->SetStringField(DataTypeFieldName, DataNumberTypeValue);

	return MakeShared<FJsonValueObject>(JsonObj);
}

void UContentInspectorCommandlet::OnBitbucketReportPutRequestCompleted(
	FHttpRequestPtr InRequest,
	FHttpResponsePtr InResponse,
	bool bWasSuccessful)
{
	if (InResponse.IsValid())
	{
		const int32 ResponseCode = InResponse->GetResponseCode();
		const FString ResponseMessage = InResponse->GetContentAsString();

		UE_LOG(
			LogContentInspectorCommandlet,
			Verbose,
			TEXT("Bitbucket put request response code:'%d', response message:'%s'"),
			ResponseCode,
			*ResponseMessage);
	}
	else
	{
		UE_LOG(LogContentInspectorCommandlet, Verbose, TEXT("Bitbucket put request had not response"));
	}

	bWaitResponse = false;
}
