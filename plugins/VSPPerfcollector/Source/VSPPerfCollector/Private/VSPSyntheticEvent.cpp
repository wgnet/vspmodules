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
#include "VSPSyntheticEvent.h"

FVSPSyntheticEvent::FVSPSyntheticEvent(TSharedPtr<FVSPEventInfo> Info):
	Info(Info)
{
}

void FVSPSyntheticEvent::SetCalculationStrategy(TSharedPtr<IVSPSyntheticCalculator> NewCalculator)
{
	if (NewCalculator)
		Calculator = NewCalculator;
}

TWeakPtr<FVSPEventInfo> FVSPSyntheticEvent::UpdateValue()
{
	if (!Calculator || !Info.IsValid())
		return Info;

	const TSharedPtr<FVSPEventInfo> PinInfo = Info.Pin();
	PinInfo->Value = Calculator->CalculateAndDumpTo(*PinInfo.Get());

	if (bNeedCleanup)
	{
		for (int32 Id = 0; Id < PinInfo->Children.Num(); ++Id)
		{
			if (!PinInfo->Children[Id].IsValid() || PinInfo->Children[Id]->Value == 0.0)
				PinInfo->Children.RemoveAt(Id);
		}
		bNeedCleanup = false;
	}

	return Info;
}

void FVSPSyntheticEvent::ProcessTimerStack(const TArrayView<FVSPTimerInfo>& Stack, FVSPThreadInfo& Thread) const
{
	if (Calculator)
		Calculator->Search(Stack, Thread);
}

void FVSPSyntheticEvent::PendingCleanup()
{
	if (!Info.IsValid())
		return;

	bNeedCleanup = true;
	for (const TSharedPtr<FVSPEventInfo>& Event : Info.Pin()->Children)
		Event->Value = 0.0;
}
