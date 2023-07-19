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

#include "RequestParamsFromServer.generated.h"


class IHttpResponse;
class IHttpRequest;
struct FJsonDataWithMeta;


UENUM()
enum class ENGINXTypes : uint8
{
	Unknown,
	Other,
	Directory,
	File,
};

USTRUCT()
struct FNGINXContent
{
	GENERATED_BODY()

	UPROPERTY()
	FString Name = "";

	UPROPERTY()
	ENGINXTypes Type = ENGINXTypes::Unknown;

	UPROPERTY()
	FString MTime = "";
};

struct FRequestParamsFromServerWorker : public TSharedFromThis<FRequestParamsFromServerWorker, ESPMode::ThreadSafe>
{
	TArray<TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>> Requests;
	TArray<TSharedPtr<IHttpRequest, ESPMode::ThreadSafe>> RequestsToReprocess;
	FCriticalSection RequestsMutex;

	TArray<FJsonDataWithMeta> DataWithContexts;
	FCriticalSection DataWithContextsMutex;

	// Connection controller
	FThreadSafeBool HasConnection;
	FThreadSafeCounter ConnectionErrorCounter;
	FThreadSafeCounter ErrorCounter = 0;

	TWeakPtr<FRequestParamsFromServerWorker, ESPMode::ThreadSafe> GetWeakThis()
	{
		return AsShared();
	};

	void RequestFolderContent(FString URL);
	void RequestFile(FString URL);

	// Create simple GET request
	TSharedRef<IHttpRequest, ESPMode::ThreadSafe> CreateRequest(FString URL);
	// Start processing request and add to Requests queue we are waiting before data process starts.
	void ProcessRequest(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request);
	// Remove process from Requests queue. E.g. it's complete.
	void FreeRequest(TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request);
	/*
	 * Check response and retry request if it's failed
	 * Return false if request can't be processed
	 */
	bool CheckResponse(
		TSharedPtr<IHttpRequest, ESPMode::ThreadSafe> Request,
		TSharedPtr<IHttpResponse, ESPMode::ThreadSafe> Response,
		bool Connected);
};

struct FRequestParamsFromServerTask : public FNonAbandonableTask
{
	friend class FAutoDeleteAsyncTask<FRequestParamsFromServerTask>;

public:
	virtual ~FRequestParamsFromServerTask() = default;

	static void RequestParamsFromServer(FQueuedThreadPool* InQueuedPool = GBackgroundPriorityThreadPool)
	{
		(new FAutoDeleteAsyncTask<FRequestParamsFromServerTask>())->StartBackgroundTask(InQueuedPool);
	}

protected:
	virtual void DoWork();

	TSharedPtr<FRequestParamsFromServerWorker, ESPMode::ThreadSafe> Worker;
	TWeakPtr<FRequestParamsFromServerWorker, ESPMode::ThreadSafe> GetWeakWorker()
	{
		return Worker;
	};

	// This next section of code needs to be here.  Not important as to why.
	FORCEINLINE TStatId GetStatId() const
	{
		RETURN_QUICK_DECLARE_CYCLE_STAT(FRequestParamsFromServerTask, STATGROUP_ThreadPoolAsyncTasks);
	}
};
