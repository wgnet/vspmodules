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
#include "RequestParamsFromServer.h"

#include "ParamsRegistry.h"
#include "ParamsSettings.h"
#include "Utils/ParamsUtils.h"

#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "JsonObjectConverter.h"


void FRequestParamsFromServerTask::DoWork()
{
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(
		*(FString("FRequestParamsFromServer::DoWork [") + UParamsSettings::Get()->GetParamsServerURL() + "]"));

	Worker = MakeShareable(new FRequestParamsFromServerWorker());

	Worker->RequestFolderContent(UParamsSettings::Get()->GetParamsServerURL());

	auto WaitAndCheckAllRequests = [WeakThis = GetWeakWorker()]()
	{
		if (!WeakThis.IsValid())
			return true;
		FRequestParamsFromServerWorker& This = *WeakThis.Pin();

		FScopeLock RequestsLock(&This.RequestsMutex);

		while (This.RequestsToReprocess.Num())
		{
			// We are re-processing request here due to the specification in the docs:
			// "A request can be re-used but not while still being processed."
			This.ProcessRequest(This.RequestsToReprocess[0]);
		}

		return This.Requests.Num() == 0;
	};

	FGenericPlatformProcess::ConditionalSleep(WaitAndCheckAllRequests, 1.f);

	UE_LOG(
		LogParams,
		Log,
		TEXT(
			"FRequestParamsFromServer::DoWork: Downloading stage finished. Stats: HasConnection=%d, ConnectionErrorCounter=%d, ErrorCounter=%d"),
		static_cast<int32>(Worker->HasConnection),
		Worker->ConnectionErrorCounter.GetValue(),
		Worker->ErrorCounter.GetValue());

	if (Worker->HasConnection
		&& Worker->ConnectionErrorCounter.GetValue() <= UParamsSettings::Get()->MaxConnectionErrors
		&& Worker->ErrorCounter.GetValue() <= UParamsSettings::Get()->MaxErrors)
		FParamsRegistry::Get().AddParamsFromJsonObjects(Worker->DataWithContexts);

	FParamsRegistry::Get().FinishParamsInitialization();
}

void FRequestParamsFromServerWorker::RequestFolderContent(FString URL)
{
	const FHttpRequestRef Request = CreateRequest(URL);

	auto OnComplete = [WeakThis = GetWeakThis()](FHttpRequestPtr Request, FHttpResponsePtr Response, bool Connected)
	{
		if (!WeakThis.IsValid())
			return;
		FRequestParamsFromServerWorker& This = *WeakThis.Pin();

		if (!This.CheckResponse(Request, Response, Connected))
			return;

		TArray<FNGINXContent> Data;
		if (!FJsonObjectConverter::JsonArrayStringToUStruct(Response->GetContentAsString(), &Data))
		{
			UE_LOG(LogParams, Warning, TEXT("FRequestParamsFromServer::RequestFolderContent: Can't parse responce"))
			return;
		}

		for (const FNGINXContent& Content : Data)
		{
			if (Content.Type == ENGINXTypes::Directory)
			{
				UE_LOG(
					LogParams,
					Verbose,
					TEXT("FRequestParamsFromServer::RequestFolderContent: Dir: %s"),
					*Content.Name)
				This.RequestFolderContent(Request->GetURL() / Content.Name);
			}
			else if (Content.Type == ENGINXTypes::File)
			{
				UE_LOG(
					LogParams,
					Verbose,
					TEXT("FRequestParamsFromServer::RequestFolderContent: File: %s"),
					*Content.Name)
				This.RequestFile(Request->GetURL() / Content.Name);
			}
		}

		This.FreeRequest(Request);
	};

	Request->OnProcessRequestComplete().BindLambda(OnComplete);

	ProcessRequest(Request);
}

void FRequestParamsFromServerWorker::RequestFile(FString URL)
{
	const FHttpRequestRef Request = CreateRequest(URL);

	auto OnComplete = [WeakThis = GetWeakThis()](FHttpRequestPtr Request, FHttpResponsePtr Response, bool Connected)
	{
		if (!WeakThis.IsValid())
			return;
		FRequestParamsFromServerWorker& This = *WeakThis.Pin();

		if (!This.CheckResponse(Request, Response, Connected))
			return;

		FParamsUtils::LoadJsonFromString(
			Response->GetContentAsString(),
			This.DataWithContexts,
			Request->GetURL(),
			&This.DataWithContextsMutex);

		This.FreeRequest(Request);
	};

	Request->OnProcessRequestComplete().BindLambda(OnComplete);

	ProcessRequest(Request);
}

TSharedRef<IHttpRequest, ESPMode::ThreadSafe> FRequestParamsFromServerWorker::CreateRequest(FString URL)
{
	FHttpModule& HttpModule = FHttpModule::Get();

	const FHttpRequestRef Request = HttpModule.CreateRequest();
	Request->SetVerb(TEXT("GET"));
	Request->SetURL(URL);

	return Request;
}

void FRequestParamsFromServerWorker::ProcessRequest(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request)
{
	FScopeLock RequestsLock(&RequestsMutex);

	Requests.Add(Request);
	RequestsToReprocess.Remove(Request);

	Request->ProcessRequest();
}

void FRequestParamsFromServerWorker::FreeRequest(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request)
{
	FScopeLock RequestsLock(&RequestsMutex);
	Requests.Remove(Request);
}

bool FRequestParamsFromServerWorker::CheckResponse(
	TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request,
	TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response,
	bool Connected)
{
	static const TArray<int32> SkipCodes = {
		EHttpResponseCodes::NotFound,
		EHttpResponseCodes::Denied,
		EHttpResponseCodes::BadMethod
	};

	if (Connected)
	{
		HasConnection = true;

		if (EHttpResponseCodes::IsOk(Response->GetResponseCode()))
			return true;
	}

	if (Request->GetStatus() == EHttpRequestStatus::Failed_ConnectionError)
	{
		if (UParamsSettings::Get()->MaxConnectionErrors >= 0
			&& ConnectionErrorCounter.Increment() > UParamsSettings::Get()->MaxConnectionErrors)
		{
			UE_LOG(
				LogParams,
				Error,
				TEXT("FRequestParamsFromServer::RequestFolderContent: Connection failed, aborting."));
			FreeRequest(Request);
			return false;
		}

		UE_LOG(
			LogParams,
			Warning,
			TEXT("FRequestParamsFromServer::RequestFolderContent: Connection failed, retrying..."));
	}
	else
	{
		const int32 ResponseCode = Response->GetResponseCode();

		if (SkipCodes.Contains(ResponseCode))
		{
			UE_LOG(
				LogParams,
				Warning,
				TEXT("FRequestParamsFromServer::RequestFolderContent: Request failed, skipping. Responce code: [%d]"),
				ResponseCode);
			FreeRequest(Request);
			return false;
		}

		if (UParamsSettings::Get()->MaxErrors >= 0 && ErrorCounter.Increment() > UParamsSettings::Get()->MaxErrors)
		{
			UE_LOG(
				LogParams,
				Error,
				TEXT("FRequestParamsFromServer::RequestFolderContent: Request failed, aborting. Responce code: [ %d ]"),
				ResponseCode);
			FreeRequest(Request);
			return false;
		}

		UE_LOG(
			LogParams,
			Warning,
			TEXT("FRequestParamsFromServer::RequestFolderContent: Request failed, retrying... Responce code: [ %d ]"),
			ResponseCode);
	}

	FScopeLock RequestsLock(&RequestsMutex);

	RequestsToReprocess.Add(Request);
	Requests.Remove(Request);
	return false;
}
