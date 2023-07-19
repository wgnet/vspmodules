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

#include "VSPCheckFailCounter.generated.h"

UCLASS(Config = Game, DefaultConfig)
class VSPCOMMONUTILS_API UVSPCheckFailCounter : public UObject
{
	GENERATED_BODY()

public:
	static UVSPCheckFailCounter* Get();

	bool TestAndSetNumLogMessages();
	bool TestAndSetNumScreenMessages();
	bool TestAndSetNumCatReports();
	bool TestAndSetNumDebugBreaks();

private:
	UPROPERTY(Config)
	int32 MaxNumLogMessagesPerSession = 30;

	UPROPERTY(Config)
	int32 MaxNumScreenMessagesPerSession = 10;

	UPROPERTY(Config)
	int32 MaxNumCatReportsPerSession = 1;

	UPROPERTY(Config)
	int32 MaxNumDebugBreaksPerSession = 1;

	int32 NumLogMessages = 0;
	int32 NumScreenMessages = 0;
	int32 NumCatReports = 0;
	int32 NumDebugBreaks = 0;
};
