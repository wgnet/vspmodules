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

#include "VSPSyntheticFilters.h"
#include "VSPTimerInfo.h"
#include "Dom/JsonObject.h"

struct FVSPEventInfo;
class FVSPSyntheticEvent;

// Calculator base, that make some calculations on filtered metrics
class VSPPERFCOLLECTOR_API IVSPSyntheticCalculator
{
public:
	static TSharedPtr<IVSPSyntheticCalculator> TryCreateFromName(const FString& Name, TSharedPtr<FVSPEventInfo> Info);

	IVSPSyntheticCalculator(TSharedPtr<FVSPEventInfo> Info);
	virtual ~IVSPSyntheticCalculator() = default;

	virtual bool SetupFromJson(const TSharedPtr<FJsonObject>& JsonObject) = 0;
	virtual void Search(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread) = 0;
	virtual double CalculateAndDumpTo(FVSPEventInfo& Info) = 0;

	TWeakPtr<FVSPEventInfo> EventInfo;
};


class VSPPERFCOLLECTOR_API FVSPAccumulationCalculator : public IVSPSyntheticCalculator
{
public:
	static const FString JsonName;

	FVSPAccumulationCalculator(TSharedPtr<FVSPEventInfo> Info);

	virtual bool SetupFromJson(const TSharedPtr<FJsonObject>& JsonObject) override;
	virtual void Search(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread) override;
	virtual double CalculateAndDumpTo(FVSPEventInfo& Info) override;


protected:
	TSharedPtr<IVSPSyntheticFilter> IncFilter;
	TSharedPtr<IVSPSyntheticFilter> DecFilter;

	TMap<uint32, FVSPTimerInfo> IncTimers;
	TMap<uint32, FVSPTimerInfo> DecTimers;
};


class VSPPERFCOLLECTOR_API FVSPInstanceCalculator : public IVSPSyntheticCalculator
{
public:
	static const FString JsonName;

	FVSPInstanceCalculator(TSharedPtr<FVSPEventInfo> Info);

	virtual bool SetupFromJson(const TSharedPtr<FJsonObject>& JsonObject) override;
	virtual void Search(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread) override;
	virtual double CalculateAndDumpTo(FVSPEventInfo& Info) override;


protected:
	TSharedPtr<IVSPSyntheticFilter> Filter;
	TUniquePtr<FRegexPattern> InstanceIdExtractor;
};

class VSPPERFCOLLECTOR_API FVSPHighestTimedCalculator : public IVSPSyntheticCalculator
{
public:
	static const FString JsonName;

	FVSPHighestTimedCalculator(TSharedPtr<FVSPEventInfo> Info);

	bool SetupFromJson(const TSharedPtr<FJsonObject>& JsonObject) override;
	void Search(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread) override;
	double CalculateAndDumpTo(FVSPEventInfo& Info) override;

protected:
	TSharedPtr<IVSPSyntheticCalculator> InnerCalculator;

	double FadeOutTime;
	double LastTime;

	double HighestValue;
};