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
#include "VSPSyntheticFilters.h"

#include "CborReader.h"
#include "VSPPerfCollector.h"
#include "VSPPerfUtils.h"
#include "VSPEventInfo.h"
#include "Algo/Accumulate.h"
#include "Dom/JsonObject.h"
#include "Dom/JsonValue.h"
#include "Serialization/MemoryReader.h"

#define MAX_REGEXP_GROUP_COUNT 4


const FString FVSPGroupFilter::JsonName = "OneOf";
const FString FVSPEqualFilter::JsonName = "Equal";
const FString FVSPRegexpFilter::JsonName = "Regexp";
const FString FVSPStartsWithFilter::JsonName = "StartsWith";
const FString FVSPHaveAnyParentFilter::JsonName = "HaveAnyParent";
const FString FVSPHaveParentFilter::JsonName = "HaveParent";
const FString FVSPHaveChildFilter::JsonName = "HaveChild";
const FString FVSPMetadataFilter::JsonName = "MetaFields";


TSharedPtr<IVSPSyntheticFilter> IVSPSyntheticFilter::TryCreateFromName(const FString& Name,
                                                                     TSharedPtr<FVSPEventInfo> EventInfo)
{
	if (Name == FVSPGroupFilter::JsonName)
		return MakeShared<FVSPGroupFilter>(EventInfo);

	if (Name == FVSPEqualFilter::JsonName)
		return MakeShared<FVSPEqualFilter>(EventInfo);

	if (Name == FVSPRegexpFilter::JsonName)
		return MakeShared<FVSPRegexpFilter>(EventInfo);

	if (Name == FVSPStartsWithFilter::JsonName)
		return MakeShared<FVSPStartsWithFilter>(EventInfo);

	if (Name == FVSPHaveAnyParentFilter::JsonName)
		return MakeShared<FVSPHaveAnyParentFilter>(EventInfo);

	if (Name == FVSPHaveParentFilter::JsonName)
		return MakeShared<FVSPHaveParentFilter>(EventInfo);

	if (Name == FVSPHaveChildFilter::JsonName)
		return MakeShared<FVSPHaveChildFilter>(EventInfo);

	if (Name == FVSPMetadataFilter::JsonName)
		return MakeShared<FVSPMetadataFilter>(EventInfo);

	return nullptr;
}


TSharedPtr<IVSPSyntheticFilter> IVSPSyntheticFilter::TryCreateFromJson(const TSharedPtr<FJsonValue>& JsonValue,
                                                                     TSharedPtr<FVSPEventInfo> EventInfo)
{
	if (!JsonValue.IsValid())
		return nullptr;

	if (JsonValue->Type == EJson::String)
		return MakeShared<FVSPEqualFilter>(JsonValue->AsString(), EventInfo);

	if (JsonValue->Type == EJson::Object)
	{
		TSharedPtr<FVSPSequenceFilter> Filter = MakeShared<FVSPSequenceFilter>(EventInfo);
		if (Filter->SetupFromJson(JsonValue))
			return Filter;
	}
	else if (JsonValue->Type == EJson::Array)
	{
		TSharedPtr<FVSPGroupFilter> Filter = MakeShared<FVSPGroupFilter>(EventInfo);
		if (Filter->SetupFromJson(JsonValue))
			return Filter;
	}

	return nullptr;
}

IVSPSyntheticFilter::IVSPSyntheticFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	EventInfo(EventInfo)
{
}

bool FVSPSequenceFilter::SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::Object || !EventInfo.IsValid())
		return false;

	const TSharedPtr<FJsonObject> JsonObject = JsonValue->AsObject();
	for (TTuple<FString, TSharedPtr<FJsonValue>>& Item : JsonObject->Values)
	{
		if (TSharedPtr<IVSPSyntheticFilter> Filter = TryCreateFromName(Item.Key, EventInfo.Pin()))
		{
			if (Filter->SetupFromJson(Item.Value))
			{
				InnerFilters.Push(Filter);
			}
			else
			{
				UE_LOG(LogVSPPerfCollector, Error, TEXT("Couldn't setup Filter %s"), *Item.Key);
				return false;
			}
		}
		else
		{
			UE_LOG(LogVSPPerfCollector, Error, TEXT("Couldn't find Filter %s"), *Item.Key);
			return false;
		}
	}

	return true;
}

const FVSPTimerInfo* FVSPSequenceFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	if (Stack.Num() == 0)
		return nullptr;

	const FVSPTimerInfo* Info = nullptr;
	for (const TSharedPtr<IVSPSyntheticFilter>& Filter : InnerFilters)
	{
		Info = Filter->ProcessStack(Stack, Thread);
		if (!Info)
			break;
	}

	return Info;
}

FVSPSequenceFilter::FVSPSequenceFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo)
{
}

FVSPSequenceFilter::~FVSPSequenceFilter()
{
}

FVSPGroupFilter::FVSPGroupFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo)
{
}

FVSPGroupFilter::~FVSPGroupFilter()
{
}

bool FVSPGroupFilter::SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::Array || !EventInfo.IsValid())
		return false;

	for (const TSharedPtr<FJsonValue>& Item : JsonValue->AsArray())
	{
		if (TSharedPtr<IVSPSyntheticFilter> Filter = TryCreateFromJson(Item, EventInfo.Pin()))
			InnerFilters.Push(Filter);
		else
			return false;
	}

	return true;
}

const FVSPTimerInfo* FVSPGroupFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	if (Stack.Num() == 0)
		return nullptr;

	for (const TSharedPtr<IVSPSyntheticFilter>& Filter : InnerFilters)
	{
		if (const FVSPTimerInfo* Result = Filter->ProcessStack(Stack, Thread))
			return Result;
	}

	return nullptr;
}

FVSPEqualFilter::FVSPEqualFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo)
{
}

FVSPEqualFilter::FVSPEqualFilter(FString Name, TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo),
	SearchName(Name)
{
}

bool FVSPEqualFilter::SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::String)
		return false;

	SearchName = JsonValue->AsString();
	return true;
}

const FVSPTimerInfo* FVSPEqualFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	if (Stack.Num() == 0 || SearchName.IsEmpty())
		return nullptr;

	if (Stack.Last().Name == SearchName)
		return &Stack.Last();

	return nullptr;
}

FVSPRegexpFilter::FVSPRegexpFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo)
{
}

bool FVSPRegexpFilter::SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::String)
		return false;

	RegexpPattern = MakeUnique<FRegexPattern>(JsonValue->AsString());
	return true;
}

const FVSPTimerInfo* FVSPRegexpFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	if (Stack.Num() == 0 || !RegexpPattern.IsValid() || !EventInfo.IsValid())
		return nullptr;

	FoundedGroups.Empty();

	FRegexMatcher Matcher(*RegexpPattern.Get(), Stack.Last().Name);
	const TSharedPtr<FVSPEventInfo> PinEvent = EventInfo.Pin();
	if (Matcher.FindNext())
	{
		int32 GroupCounter = 1;
		FString Group = Matcher.GetCaptureGroup(GroupCounter);
		while (!Group.IsEmpty() && GroupCounter < MAX_REGEXP_GROUP_COUNT)
		{
			FoundedGroups.Push(Group);
			FVSPEventInfo* Founded{};
			const TFunction<VSPPerfUtils::EWalkStatus(FVSPEventInfo*)> SearchGroup =
				[ &Group, &Founded ](FVSPEventInfo* Child)
				{
					if (Group == Child->Name)
					{
						Founded = Child;
						return VSPPerfUtils::EWalkStatus::Stop;
					}
					return VSPPerfUtils::EWalkStatus::Continue;
				};
			RecursiveBfsWalk(PinEvent.Get(), &FVSPEventInfo::Children, SearchGroup);

			if (!Founded)
			{
				TSharedPtr<FVSPEventInfo> GroupChild = MakeShared<FVSPEventInfo>(Group);
				Thread.FlatTotalEvents.Push(GroupChild);
				GroupChild->bSyntheticEvent = true;
				PinEvent->Children.Push(GroupChild);
				Founded = GroupChild.Get();
			}
			FVSPEventInfo* FoundedChild{};
			const TFunction<VSPPerfUtils::EWalkStatus(FVSPEventInfo*)> SearchChild =
				[ &FoundedChild, &Stack ](FVSPEventInfo* Child)
				{
					if (Stack.Last().Name == Child->Name)
					{
						FoundedChild = Child;
						return VSPPerfUtils::EWalkStatus::Stop;
					}
					return VSPPerfUtils::EWalkStatus::Continue;
				};
			RecursiveBfsWalk(Founded, &FVSPEventInfo::Children, SearchChild);
			if (!FoundedChild)
			{
				TSharedRef<FVSPEventInfo> NewChild = MakeShared<FVSPEventInfo>(Stack.Last().Name);
				Founded->Children.Push(NewChild);
				Thread.FlatTotalEvents.Push(NewChild);
			}
			Founded->Children.Last()->Value += Stack.Last().Duration;
			Founded->Value = Algo::Accumulate(Founded->Children,
				0.f,
				[](int32 Acc, const TSharedPtr<FVSPEventInfo>& Child) { return Acc + Child->Value; });

			Group = Matcher.GetCaptureGroup(++GroupCounter);
		}
		return &Stack.Last();
	}

	return nullptr;
}

FVSPStartsWithFilter::FVSPStartsWithFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo)
{
}

bool FVSPStartsWithFilter::SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonValue.IsValid() || JsonValue->Type != EJson::String)
		return false;

	Beginning = JsonValue->AsString();
	return true;
}

const FVSPTimerInfo* FVSPStartsWithFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
                                                      FVSPThreadInfo& Thread)
{
	if (Stack.Num() == 0 || Beginning.IsEmpty())
		return nullptr;

	if (Stack.Last().Name.StartsWith(Beginning))
		return &Stack.Last();

	return nullptr;
}

FVSPHaveAnyParentFilter::FVSPHaveAnyParentFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo)
{
}

bool FVSPHaveAnyParentFilter::SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonValue.IsValid() || !EventInfo.IsValid())
		return false;

	ParentFilter = TryCreateFromJson(JsonValue, EventInfo.Pin());
	if (ParentFilter)
		return true;

	return false;
}

const FVSPTimerInfo* FVSPHaveAnyParentFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
                                                         FVSPThreadInfo& Thread)
{
	if (Stack.Num() <= 1 || !ParentFilter)
		return nullptr;

	for (int32 Num = Stack.Num(); Num > 0; --Num)
	{
		if (ParentFilter->ProcessStack(Stack.Slice(0, Num), Thread))
			return &Stack.Last();
	}

	return nullptr;
}

FVSPHaveParentFilter::FVSPHaveParentFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	FVSPHaveAnyParentFilter(EventInfo)
{
}

const FVSPTimerInfo* FVSPHaveParentFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack,
                                                      FVSPThreadInfo& Thread)
{
	if (Stack.Num() <= 1)
		return nullptr;

	if (ParentFilter->ProcessStack(Stack.Slice(0, Stack.Num() - 1), Thread))
		return &Stack.Last();

	return nullptr;
}

FVSPHaveChildFilter::FVSPHaveChildFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo)
{
}

bool FVSPHaveChildFilter::SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonValue.IsValid() || !EventInfo.IsValid())
		return false;

	ChildFilter = TryCreateFromJson(JsonValue, EventInfo.Pin());
	if (ChildFilter)
		return true;

	return false;
}

const FVSPTimerInfo* FVSPHaveChildFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	if (Stack.Num() == 0)
		return nullptr;

	TArray<FVSPTimerInfo> ChildStack(Stack);
	for (const int32& ChildId : Stack.Last().ChildIds)
	{
		ChildStack.SetNum(Stack.Num());
		ChildStack.Push(Thread.FrameChildrenStorage[ChildId]);
		const FVSPTimerInfo* Timer = ChildFilter->ProcessStack(ChildStack, Thread);
		if (Timer)
			return &Stack.Last();
	}

	return nullptr;
}

FString FVSPMetadataFilter::FMetaField::Format(const TArray<TTuple<FString, FString>>& Meta)
{
	if (Formatter.Template.IsEmpty())
	{
		const TTuple<FString, FString>* FieldValue = Meta.FindByPredicate(
			[this](const TTuple<FString, FString>& MetaData)
			{
				return MetaData.Key == Name;
			});

		if (FieldValue)
			return FieldValue->Value;

		return {};
	}

	FStringFormatOrderedArguments Args;
	for (const FString& Field : Formatter.Fields)
	{
		const TTuple<FString, FString>* FieldValue = Meta.FindByPredicate(
			[&Field](const TTuple<FString, FString>& MetaData)
			{
				return MetaData.Key == Field;
			});

		if (FieldValue)
			Args.Add(FieldValue->Value);
	}

	return FString::Format(*Formatter.Template, Args);
}

FVSPMetadataFilter::FVSPMetadataFilter(TSharedPtr<FVSPEventInfo> EventInfo):
	IVSPSyntheticFilter(EventInfo)
{
}

bool FVSPMetadataFilter::SetupFromJson(const TSharedPtr<FJsonValue>& JsonValue)
{
	if (!JsonValue.IsValid() || !EventInfo.IsValid())
		return false;

	const TSharedPtr<FJsonObject>* ObjectPtr = nullptr;
	if (!JsonValue->TryGetObject(ObjectPtr))
		return false;

	const TSharedPtr<FVSPEventInfo> PinnedEvent = EventInfo.Pin();
	for (auto& [MetaName, FilterDataValue]: (*ObjectPtr)->Values)
	{
		const TSharedPtr<FJsonObject>* FilterData = nullptr;
		if (!FilterDataValue->TryGetObject(FilterData))
			return false;

		FMetaField& Field = MetaFields.Emplace_GetRef();
		Field.Name = MetaName;

		if (const TSharedPtr<FJsonValue> Filter = (*FilterData)->TryGetField("Filter"))
			Field.Filter = TryCreateFromJson(Filter, PinnedEvent);

		const TSharedPtr<FJsonObject>* NameFormat = nullptr;
		if ((*FilterData)->TryGetObjectField("NameFormat", NameFormat))
		{
			(*NameFormat)->TryGetStringField("Template", Field.Formatter.Template);
			const TArray<TSharedPtr<FJsonValue>>* Fields;
			if ((*NameFormat)->TryGetArrayField("Fields", Fields))
			{
				for (const TSharedPtr<FJsonValue>& FieldNameJson : *Fields)
				{
					FString& FieldName = Field.Formatter.Fields.Emplace_GetRef();
					if (!FieldNameJson->TryGetString(FieldName))
						return false;
				}
			}
		}
	}

	return true;
}

class FMetadataReader
{
public:
	FMetadataReader(FVSPTimerInfo& Info, ECborEndianness Endianness = ECborEndianness::StandardCompliant):
		MemoryReader(Info.Metadata),
		CborReader(&MemoryReader, Endianness)
	{
		CborReader.ReadNext(Context);
	}

	TTuple<FString, FString> ReadNext()
	{
		// Read key
		if (!CborReader.ReadNext(Context) || !Context.IsString())
			return {};

		TTuple<FString, FString> Meta;
		Meta.Key.Append(Context.AsCString(), Context.AsLength());

		if (!CborReader.ReadNext(Context))
			return {};

		switch (Context.MajorType())
		{
		case ECborCode::Int:
		case ECborCode::Uint:
			Meta.Value.Appendf(TEXT("%llu"), Context.AsUInt());
			return Meta;
		case ECborCode::ByteString:
			Meta.Value.Append(Context.AsCString(), Context.AsLength());
			return Meta;
		case ECborCode::TextString:
			Meta.Value.Append(Context.AsString());
			return Meta;
		default: break;
		}

		if (Context.RawCode() == (ECborCode::Prim|ECborCode::Value_4Bytes))
			Meta.Value.Appendf(TEXT("%f"), Context.AsFloat());
		else if (Context.RawCode() == (ECborCode::Prim|ECborCode::Value_8Bytes))
			Meta.Value.Appendf(TEXT("%g"), Context.AsDouble());
		else if (Context.IsFiniteContainer())
			CborReader.SkipContainer(ECborCode::Array);

		return Meta;
	}

	TArray<TTuple<FString, FString>> ReadAll()
	{
		TArray<TTuple<FString, FString>> MetaFields;
		for (TTuple<FString, FString> Meta = ReadNext(); !Meta.Key.IsEmpty(); Meta = ReadNext())
			MetaFields.Emplace(Meta);
		return MetaFields;
	}

private:
	FMemoryReaderView MemoryReader;
	FCborReader CborReader;
	FCborContext Context;
};

const FVSPTimerInfo* FVSPMetadataFilter::ProcessStack(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread)
{
	if (Stack.Num() == 0 || Stack.Last().Metadata.Num() == 0 || MetaFields.Num() == 0)
		return nullptr;

	const TSharedPtr<FVSPEventInfo> Event = EventInfo.Pin();
	TSharedPtr<FVSPEventInfo> Root = Event;
	FMetadataReader MetaReader(Stack.Last());
	TArray<TTuple<FString, FString>> EventMetaData = MetaReader.ReadAll();
	TArray<FVSPTimerInfo> NewStack(Stack);
	NewStack.Push({});
	const FVSPTimerInfo* LastProcessedEvent = nullptr;
	for (FMetaField& Meta : MetaFields)
	{
		TTuple<FString, FString>* MetaField = EventMetaData.FindByPredicate(
			[&Meta](const TTuple<FString, FString>& Field)
			{
				return Field.Key == Meta.Name;
			});

		if (!MetaField)
			return nullptr;

		if (!Meta.Filter)
		{
			LastProcessedEvent = &Stack.Last();
		}
		else
		{
			NewStack.Last().Name = MetaField->Value;
			if (Meta.Filter->ProcessStack(NewStack, Thread))
				LastProcessedEvent = &Stack.Last();
		}

		if (!LastProcessedEvent)
			return LastProcessedEvent;
	}

	FString GroupName;
	for (FMetaField& Meta : MetaFields)
	{
		GroupName = Meta.Format(EventMetaData);
		if (GroupName.IsEmpty())
			continue;

		TSharedPtr<FVSPEventInfo>* FieldGroup = Root->Children.FindByPredicate(
			[&GroupName](const TSharedPtr<FVSPEventInfo>& Child)
			{
				return Child && Child->Name == GroupName;
			});

		if (FieldGroup)
		{
			Root = *FieldGroup;
			Root->Value += LastProcessedEvent->Duration;
		}
		else
		{
			TSharedPtr<FVSPEventInfo> Child = Root->Children[Root->Children.Emplace(MakeShared<FVSPEventInfo>(GroupName))];
			Child->Value = LastProcessedEvent->Duration;
			Child->Parent = Root;
			Root = Child;
			Thread.FlatTotalEvents.Push(Root);
		}
	}

	return LastProcessedEvent;
}
