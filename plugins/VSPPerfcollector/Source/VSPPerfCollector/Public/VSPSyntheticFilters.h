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

#include "VSPTimerInfo.h"
#include "Internationalization/Regex.h"


struct FVSPThreadInfo;
struct FVSPEventInfo;
class FJsonValue;

/// Base class for custom metric filter
/// It takes stack of metrics, where the top is ended now
/// After, pass through some logic, and returns event if it was suite, likely - the first item in EventStack
/// Each child might be added to IVSPSyntheticFilter::TryCreateFromName
/// TODO: Remake IVSPSyntheticFilter::TryCreateFromName to more flexibility
class VSPPERFCOLLECTOR_API IVSPSyntheticFilter
{
public:
	static TSharedPtr<IVSPSyntheticFilter> TryCreateFromName(const FString& Name, TSharedPtr<FVSPEventInfo> EventInfo);
	static TSharedPtr<IVSPSyntheticFilter> TryCreateFromJson(const TSharedPtr<FJsonValue>& JsonValue,
	                                                        TSharedPtr<FVSPEventInfo> EventInfo);

	IVSPSyntheticFilter(TSharedPtr<FVSPEventInfo> EventInfo);
	virtual ~IVSPSyntheticFilter() = default;

	/// Fill filter data from json structure, each filter has own json-data requirement
	virtual bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) = 0;
	/// Processing logic here
	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread) = 0;

	// Could be removed, careful
	TWeakPtr<FVSPEventInfo> EventInfo;
};


class VSPPERFCOLLECTOR_API FVSPSequenceFilter : public IVSPSyntheticFilter
{
public:
	FVSPSequenceFilter(TSharedPtr<FVSPEventInfo> EventInfo);
	virtual ~FVSPSequenceFilter() override;

	virtual bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
											 FVSPThreadInfo& Thread) override;

protected:
	TArray<TSharedPtr<IVSPSyntheticFilter>> InnerFilters;
};


class VSPPERFCOLLECTOR_API FVSPGroupFilter : public IVSPSyntheticFilter
{
public:
	static const FString JsonName;

	FVSPGroupFilter(TSharedPtr<FVSPEventInfo> EventInfo);

	virtual ~FVSPGroupFilter() override;

	virtual bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
											 FVSPThreadInfo& Thread) override;

protected:
	TArray<TSharedPtr<IVSPSyntheticFilter>> InnerFilters;
};


class VSPPERFCOLLECTOR_API FVSPEqualFilter : public IVSPSyntheticFilter
{
public:
	FVSPEqualFilter(TSharedPtr<FVSPEventInfo> EventInfo);
	FVSPEqualFilter(FString Name, TSharedPtr<FVSPEventInfo> EventInfo);
	static const FString JsonName;

	virtual bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
											 FVSPThreadInfo& Thread) override;

private:
	FString SearchName;
};


class VSPPERFCOLLECTOR_API FVSPRegexpFilter : public IVSPSyntheticFilter
{
public:
	static const FString JsonName;

	FVSPRegexpFilter(TSharedPtr<FVSPEventInfo> EventInfo);

	virtual bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
											 FVSPThreadInfo& Thread) override;

	const TArray<FString>& GetFoundedGroups() const { return FoundedGroups; }

private:
	TUniquePtr<FRegexPattern> RegexpPattern;
	TArray<FString> FoundedGroups;
};


class VSPPERFCOLLECTOR_API FVSPStartsWithFilter : public IVSPSyntheticFilter
{
public:
	static const FString JsonName;

	FVSPStartsWithFilter(TSharedPtr<FVSPEventInfo> EventInfo);

	virtual bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
											 FVSPThreadInfo& Thread) override;

private:
	FString Beginning;
};


class VSPPERFCOLLECTOR_API FVSPHaveAnyParentFilter : public IVSPSyntheticFilter
{
public:
	static const FString JsonName;

	FVSPHaveAnyParentFilter(TSharedPtr<FVSPEventInfo> EventInfo);

	virtual bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
	                                         FVSPThreadInfo& Thread) override;

protected:
	TSharedPtr<IVSPSyntheticFilter> ParentFilter = nullptr;
};


class VSPPERFCOLLECTOR_API FVSPHaveParentFilter : public FVSPHaveAnyParentFilter
{
public:
	static const FString JsonName;

	FVSPHaveParentFilter(TSharedPtr<FVSPEventInfo> EventInfo);

	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
	                                         FVSPThreadInfo& Thread) override;
};


class VSPPERFCOLLECTOR_API FVSPHaveChildFilter : public IVSPSyntheticFilter
{
public:
	static const FString JsonName;

	FVSPHaveChildFilter(TSharedPtr<FVSPEventInfo> EventInfo);
	virtual bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	virtual const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
											 FVSPThreadInfo& Thread) override;

protected:
	TSharedPtr<IVSPSyntheticFilter> ChildFilter = nullptr;
};


class VSPPERFCOLLECTOR_API FVSPMetadataFilter : public IVSPSyntheticFilter
{
	struct FNameFormatter
	{
		FString Template;
		TArray<FString> Fields;

	};

	struct FMetaField
	{
		FString Name;
		FNameFormatter Formatter;
		TSharedPtr<IVSPSyntheticFilter> Filter;

		FString Format(const TArray<TTuple<FString, FString>>& Meta);
	};

public:
	static const FString JsonName;

	FVSPMetadataFilter(TSharedPtr<FVSPEventInfo> EventInfo);
	bool SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue) override;
	const FVSPTimerInfo* ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
									 FVSPThreadInfo& Thread) override;

protected:
	TArray<FMetaField> MetaFields;
};