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
#include "SVSPBudgetViewer.h"

#include "Algo/Accumulate.h"
#include "VSPPerfCollector.h"
#include "VSPPerfCollectorSettings.h"
#include "VSPThreadInfo.h"
#include "SlateOptMacros.h"
#include "Widgets/Images/SImage.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSuggestionTextBox.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SScrollBox.h"

#define LOCTEXT_NAMESPACE "SVSPBudgetViewer"


BEGIN_SLATE_FUNCTION_BUILD_OPTIMIZATION


uint32 SVSPBudgetViewer::ViewValueBufferSize = 60;

static TAutoConsoleVariable<FString> CVarBudgetPreset(
	TEXT("BudgetMonitor.SelectBudgetPreset"),
	TEXT(""),
	TEXT("Valid Arguments: Editor, Low, Medium, High"));

static TSharedPtr<FTextBlockStyle> TextStyle = MakeShared<FTextBlockStyle>();

constexpr const TCHAR* FavoriteMetricsRootName = TEXT("Favorites");
constexpr uint32 FavoriteMetricsRootId = 0;

class SBudgetInfoRow : public SMultiColumnTableRow<TSharedPtr<FVSPEventInfo>>
{
public:
	SLATE_BEGIN_ARGS(SBudgetInfoRow)
	{
	}

	SLATE_ATTRIBUTE(FString, Name)
	SLATE_ATTRIBUTE(double, Value)
	SLATE_ATTRIBUTE(double, Budget)
	SLATE_ATTRIBUTE(FSlateColor, Color)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs, const TSharedRef<STableViewBase>& InOwnerTableView)
	{
		Name = InArgs._Name;
		Value = InArgs._Value;
		Budget = InArgs._Budget;
		Color = InArgs._Color;

		SMultiColumnTableRow::Construct(FSuperRowType::FArguments(), InOwnerTableView);
	}

	TSharedRef<SWidget> GenerateWidgetForColumn(const FName& ColumnName) override
	{
		if (ColumnName == "Name")
		{
			return SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.AutoWidth()
				.VAlign(VAlign_Center)
				.Padding(6, 0, 0, 0)
				[
					SNew(SExpanderArrow, SharedThis( this ))
					.IndentAmount(12)
				]
				+ SHorizontalBox::Slot()
				.FillWidth(1.0f)
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.TextStyle(TextStyle.Get())
					.ColorAndOpacity(Color)
					.Text(FText::FromString(Name.Get()))
				];
		}

		if (ColumnName == "Value")
		{
			return SNew(STextBlock)
				.ColorAndOpacity(Color)
				.TextStyle(TextStyle.Get())
				.Text(FText::FromString(FString::Printf(TEXT("%.3f"), Value.Get())));
		}

		if (ColumnName == "Budget")
		{
			if (Budget.Get() >= 0.0)
				return SNew(STextBlock)
					.ColorAndOpacity(Color)
					.TextStyle(TextStyle.Get())
					.Text(FText::FromString(FString::Printf(TEXT("%.3f"), Budget.Get())));

			return SNew(STextBlock)
				.ColorAndOpacity(Color)
				.TextStyle(TextStyle.Get());
		}

		if (ColumnName == "Percentage")
		{
			double Percentage = Budget.Get() > 0.0 ? 100 * Value.Get() / Budget.Get() : 0.0;
			return SNew(STextBlock)
				.ColorAndOpacity(Color)
				.TextStyle(TextStyle.Get())
				.Text(FText::FromString(FString::Printf(TEXT("%.2f"), Percentage)));
		}

		return SNullWidget::NullWidget;
	}


private:
	TAttribute<FString> Name;
	TAttribute<double> Value;
	TAttribute<double> Budget;
	TAttribute<FSlateColor> Color;
};

void SVSPBudgetViewer::Construct(const FArguments& InArgs)
{
	constexpr uint32 MinFramesValue = 1;
	constexpr uint32 MaxFramesValue = 200;
	constexpr FColor BackgroundColor = FColor(30, 30, 30);
	constexpr uint32 FontSize = 12;

	ViewValueBufferSize = UVSPPerfCollectorSettings::Get().TableViewAvgFramesCount;
	FavoriteMetrics = UVSPPerfCollectorSettings::Get().FavoriteMetrics;
	FreshEvents.Push(MakeShared<FVSPEventInfo>(FavoriteMetricsRootName));

	TextStyle->SetColorAndOpacity({ FColor::White });
	TextStyle->SetFont(FCoreStyle::GetDefaultFontStyle("Regular", FontSize));

	ChildSlot.Padding(5)
	[
		SNew(SOverlay)
		+ SOverlay::Slot()
		[
			SNew(SImage)
			.Image(FStyleDefaults::GetNoBrush())
			.ColorAndOpacity(BackgroundColor)
		]
		+ SOverlay::Slot()
		[
			SNew(SVerticalBox)
			+SVerticalBox::Slot()
			[
				SNew(SScrollBox)
				+ SScrollBox::Slot()
				[
					SAssignNew(BudgetTreeView, STreeView<TSharedPtr<FVSPEventInfo>>)
					.TreeItemsSource(&EventsData)
					.OnGetChildren(this, &SVSPBudgetViewer::OnGetChildren)
					.OnGenerateRow(this, &SVSPBudgetViewer::OnGenerateEventRow)
					.SelectionMode(ESelectionMode::None)
					.OnMouseButtonDoubleClick_Lambda([this](TSharedPtr<FVSPEventInfo> Event)
					{
						FavoriteMetrics.Add(Event->Alias.IsEmpty() ? Event->Name : Event->Alias);
					})
					.HeaderRow(
						SNew(SHeaderRow)
						+SHeaderRow::Column("Name")
						.DefaultLabel(FText::FromString("Name"))
						.HAlignHeader(HAlign_Center)
						.FillWidth(3.f)
						+SHeaderRow::Column("Value")
						.DefaultLabel(FText::FromString("Value (ms)"))
						.HAlignHeader(HAlign_Center)
						.HAlignCell(HAlign_Center)
						.FillWidth(1.f)
						+SHeaderRow::Column("Budget")
						.DefaultLabel(FText::FromString("Budget (ms)"))
						.HAlignHeader(HAlign_Center)
						.HAlignCell(HAlign_Center)
						.FillWidth(1.f)
						+SHeaderRow::Column("Percentage")
						.DefaultLabel(FText::FromString("Percentage (%)"))
						.HAlignHeader(HAlign_Center)
						.HAlignCell(HAlign_Center)
						.FillWidth(1.f)
					)
				]
			]
			+SVerticalBox::Slot()
			.AutoHeight()
			[
				SNew(SGridPanel)
				.FillColumn(0, 1)
				.FillColumn(1, 1)
				.FillColumn(2, 1)
				+SGridPanel::Slot(0, 0)
				.Padding(10, 10, 10, 5)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Update performance config")))
				]
				+SGridPanel::Slot(0, 1)
				.Padding(10, 5, 10, 10)
				[
					SNew(SButton)
					.Text(FText::FromString(TEXT("Update")))
					.OnClicked_Lambda([this]()->FReply
					{
						FScopeLock ScopeLock(&Section);
						FModuleManager& ModuleManager = FModuleManager::Get();
						auto* PerfCollector = ModuleManager.GetModulePtr<FVSPPerfCollectorModule>("VSPPerfCollector");
						if (PerfCollector)
							PerfCollector->UpdateDefaultConfig();
						return FReply::Handled();
					})
				]
				+SGridPanel::Slot(1, 0)
				.Padding(10, 10, 10, 5)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Averaging frames count")))
				]
				+SGridPanel::Slot(1, 1)
				.Padding(10, 5, 10, 10)
				[
					SNew(SSpinBox<uint32>)
					.MinValue(MinFramesValue)
					.MaxValue(MaxFramesValue)
					.Delta(1)
					.Value(ViewValueBufferSize)
					.LinearDeltaSensitivity(10)
					.OnValueChanged_Lambda([this](uint32 InValue) { ViewValueBufferSize = InValue; })
				]
				+SGridPanel::Slot(2, 0)
				.Padding(10, 10, 10, 5)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Add metrics to Favorite")))
				]
				+SGridPanel::Slot(2, 1)
				.Padding(10, 5, 10, 10)
				[
					SAssignNew(FavoriteMetricsSearch, SSuggestionTextBox)
					.OnShowingSuggestions(this, &SVSPBudgetViewer::FavoriteSuggestions)
					.ClearKeyboardFocusOnCommit(false)
					.OnTextCommitted(this, &SVSPBudgetViewer::AddToFavoriteList)
				]
			]
		]
	];
}

void SVSPBudgetViewer::Tick(const FGeometry& AllottedGeometry, const double InCurrentTime, const float InDeltaTime)
{
	FScopeLock ScopeLock(&Section);
	++UpdateCounter;
	if (UpdateCounter > UVSPPerfCollectorSettings::Get().UIUpdateMissingFrames && bNeedUpdate)
	{
		bNeedUpdate = false;
		EventsData = FreshEvents;
		EventBufferedData = FreshNamedEvents;

		SCOPED_NAMED_EVENT(VSPBudgetViewer_Tick, FColor::Orange);
		BudgetTreeView->RebuildList();
		UpdateCounter = 0;
	}
	SCompoundWidget::Tick(AllottedGeometry, InCurrentTime, InDeltaTime);
}

SVSPBudgetViewer::FEventBufferedData& SVSPBudgetViewer::UpdateBufferedData(const TSharedPtr<FVSPEventInfo>& Item)
{
	FScopeLock ScopeLock(&Section);
	FEventBufferedData* ItemData = FreshNamedEvents.Find(Item->Name);
	if (!ItemData)
		ItemData = &FreshNamedEvents.Add(Item->Name);

	ItemData->Event = Item;

	++ItemData->FrameCounter;
	if (ItemData->ValuesBuffer.Num() != ViewValueBufferSize)
		ItemData->ValuesBuffer.SetNum(ViewValueBufferSize);

	ItemData->ValuesBuffer[ItemData->FrameCounter % ItemData->ValuesBuffer.Num()] = Item->Value;

	ItemData->SmoothValue =
		FMath::Max(Algo::Accumulate(ItemData->ValuesBuffer, 0.0) / ItemData->ValuesBuffer.Num(), 0.0);

	return *ItemData;
}

void SVSPBudgetViewer::SetMode(EBudgetViewMode Mode)
{
	ShowMode = Mode;
}

void SVSPBudgetViewer::AddToFavoriteList(const FText& NewText, ETextCommit::Type CommitType)
{
	const FString Text = NewText.ToString();
	if (CommitType == ETextCommit::Type::OnEnter && !Text.IsEmpty())
	{
		FavoriteMetrics.Add(Text);
		FavoriteMetricsSearch->SetText(FText());
	}
}

void SVSPBudgetViewer::FavoriteSuggestions(const FString& Text, TArray<FString>& OutSuggestions)
{
	FScopeLock ScopeLock(&Section);
	for (const TTuple<FString, FEventBufferedData>& Metric : FreshNamedEvents)
	{
		if (Metric.Key.StartsWith(Text))
			OutSuggestions.Push(Metric.Key);
	}
}

TSharedRef<ITableRow> SVSPBudgetViewer::OnGenerateEventRow(
	TSharedPtr<FVSPEventInfo> Item,
	const TSharedRef<STableViewBase>& Owner)
{
	if (!Item.IsValid())
		return SNew(SBudgetInfoRow, Owner);

	const FEventBufferedData& BufferedData = UpdateBufferedData(Item);

	FSlateColor Color;
	const double Budget = Item->GetBudget();
	if (Budget > 0.0)
	{
		if (BufferedData.SmoothValue > Budget)
			Color = FSlateColor(FLinearColor::Red);
		else
			Color = FMath::Lerp(FLinearColor::White, FLinearColor(FColor::Orange), BufferedData.SmoothValue / Budget);
	}
	else
	{
		Color = FSlateColor(FLinearColor::White);
	}

	return SNew(SBudgetInfoRow, Owner)
		.Color(Color)
		.Name(Item->Alias.IsEmpty() ? Item->Name : Item->Alias)
		.Value(BufferedData.SmoothValue)
		.Budget(Budget);
}

void SVSPBudgetViewer::OnGetChildren(TSharedPtr<FVSPEventInfo> ParentItem, TArray<TSharedPtr<FVSPEventInfo>>& ItemChildren)
	const
{
	for (const TSharedPtr<FVSPEventInfo>& Child : ParentItem->Children)
	{
		if (ParentItem != EventsData[FavoriteMetricsRootId] && FavoriteMetrics.Contains(Child->Name))
		{
			continue;
		}

		if (ShowMode == EBudgetViewMode::ShowAll)
			ItemChildren.Push(Child);
		else if (Child->GetBudget() <= Child->Value)
			ItemChildren.Push(Child);
	}
}

void SVSPBudgetViewer::UpdateTree(TArray<TSharedPtr<FVSPEventInfo>>& NewEvents, FVSPThreadInfo& ThreadInfo, double, double)
{
	FScopeLock ScopeLock(&Section);
	bNeedUpdate = true;

	SCOPED_NAMED_EVENT(VSPBudgetViewer_UpdateTree, FColor::Orange);
	TSet<FString> RootEventNames;
	for (int32 Idx = ThreadInfo.EventRootStart; Idx < ThreadInfo.EventRootEnd; ++Idx)
	{
		const TSharedPtr<FVSPEventInfo> NewRootEvent = NewEvents[Idx];
		if (!NewRootEvent)
			continue;
		RootEventNames.Add(NewRootEvent->Name);
	}
	for (TWeakPtr<FVSPEventInfo>& WeakEvent : ThreadInfo.FlatTotalEvents)
	{
		if (const TSharedPtr<FVSPEventInfo> Event = WeakEvent.Pin())
		{
			FEventBufferedData* BufferedEvent = FreshNamedEvents.Find(Event->Name);
			if (!BufferedEvent)
				BufferedEvent = &FreshNamedEvents.Add(Event->Name, { MakeShared<FVSPEventInfo>(Event->Name) });

			BufferedEvent->Event->DeepCopy(*Event.Get());
			if (RootEventNames.Contains(Event->Name))
			{
				TSharedPtr<FVSPEventInfo>* ExistsEvent = FreshEvents.FindByPredicate(
					[Event](const TSharedPtr<FVSPEventInfo>& Ev)
					{
						return Ev && Event->Name == Ev->Name;
					});

				if (!ExistsEvent)
					ExistsEvent = &FreshEvents.Emplace_GetRef(MakeShared<FVSPEventInfo>(BufferedEvent->Event->Name));
				*ExistsEvent = BufferedEvent->Event;
			}
			else if (FavoriteMetrics.Contains(Event->Name))
			{
				TSharedPtr<FVSPEventInfo>* ExistsEvent = FreshEvents[0]->Children.FindByPredicate(
					[Event](const TSharedPtr<FVSPEventInfo>& Ev)
					{
						return Ev && Event->Name == Ev->Name;
					});

				if (!ExistsEvent)
					ExistsEvent =
						&FreshEvents[0]->Children.Emplace_GetRef(MakeShared<FVSPEventInfo>(BufferedEvent->Event->Name));
				*ExistsEvent = BufferedEvent->Event;
			}
		}
	}
}
END_SLATE_FUNCTION_BUILD_OPTIMIZATION

#undef LOCTEXT_NAMESPACE