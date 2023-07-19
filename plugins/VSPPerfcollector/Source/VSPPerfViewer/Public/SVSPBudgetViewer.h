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
#include "Misc/DelegateFilter.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Views/STreeView.h"

struct FVSPEventInfo;
struct FVSPThreadInfo;
struct FHeaderRowStyle;
class SSuggestionTextBox;

UENUM(BlueprintType)
enum class EBudgetViewMode : uint8
{
	ShowAll,
	ShowOverloaded
};


class VSPPERFVIEWER_API SVSPBudgetViewer: public SCompoundWidget
{
	struct FEventBufferedData
	{
		TSharedPtr<FVSPEventInfo> Event;
		TArray<double> ValuesBuffer;
		uint64 FrameCounter = 0;
		int32 OverloadCounter = 0;
		float SmoothValue = 0.f;
	};

public:
	SLATE_BEGIN_ARGS(SVSPBudgetViewer)
	{
	}

	SLATE_END_ARGS()

	/** Constructs this widget with InArgs */
	void Construct(const FArguments& InArgs);

	void Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime) override;

	TSharedRef<ITableRow> OnGenerateEventRow(TSharedPtr<FVSPEventInfo> Item, const TSharedRef<STableViewBase>& Owner);
	void OnGetChildren(TSharedPtr<FVSPEventInfo> ParentItem, TArray<TSharedPtr<FVSPEventInfo>>& Children) const;

	void UpdateTree(TArray<TSharedPtr<FVSPEventInfo>>& Events, FVSPThreadInfo& ThreadInfo, double, double);

	FEventBufferedData& UpdateBufferedData(const TSharedPtr<FVSPEventInfo>& Item);

	void SetMode(EBudgetViewMode Mode);
	FORCEINLINE EBudgetViewMode Mode() const
	{
		return ShowMode;
	}

	static uint32 ViewValueBufferSize;

protected:
	void AddToFavoriteList(const FText& NewText, ETextCommit::Type CommitType);
	void FavoriteSuggestions(const FString& Text, TArray<FString>& OutSuggestions);

private:
	FCriticalSection Section;
	bool bNeedUpdate = true;

	uint32 UpdateCounter = 0;
	EBudgetViewMode ShowMode = EBudgetViewMode::ShowAll;

	TSet<FString> FavoriteMetrics;

	TArray<TSharedPtr<FVSPEventInfo>> EventsData;
	TArray<TSharedPtr<FVSPEventInfo>> FreshEvents;
	TMap<FString, FEventBufferedData> FreshNamedEvents;

	TMap<FString, FEventBufferedData> EventBufferedData;
	TSharedPtr<STreeView<TSharedPtr<FVSPEventInfo>>> BudgetTreeView;

	TSharedPtr<SSuggestionTextBox> FavoriteMetricsSearch;
};