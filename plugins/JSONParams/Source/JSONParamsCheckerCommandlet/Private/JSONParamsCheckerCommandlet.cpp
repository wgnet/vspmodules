﻿/*
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
#include "JSONParamsCheckerCommandlet.h"

#include "ParamsRegistry.h"

DEFINE_LOG_CATEGORY_STATIC(LogJSONParamsCheckerCommandlet, Log, All)

int32 UJSONParamsCheckerCommandlet::Main(const FString& Params)
{
	UE_LOG(LogJSONParamsCheckerCommandlet, Verbose, TEXT("JSONParamsCheckerCommandlet started: validating params."));

	ParamsCheckFinished.AtomicSet(false);

	FParamsRegistry::Get().CallWhenInitialized(
		[this]
		{
			FailedParamsNum = FParamsRegistry::Get().GetFailedParamsNumber();
			ParamsCheckFinished.AtomicSet(true);
		});

	FGenericPlatformProcess::ConditionalSleep(
		[this]()
		{
			return ParamsCheckFinished;
		},
		1.f);

	if (FailedParamsNum > 0)
	{
		UE_LOG(
			LogJSONParamsCheckerCommandlet,
			Error,
			TEXT("JSONParamsCheckerCommandlet finished: found %d invalid params."),
			FailedParamsNum);
		return 1;
	}

	UE_LOG(
		LogJSONParamsCheckerCommandlet,
		Verbose,
		TEXT("JSONParamsCheckerCommandlet finished: all params are valid."));

	return 0;
}
