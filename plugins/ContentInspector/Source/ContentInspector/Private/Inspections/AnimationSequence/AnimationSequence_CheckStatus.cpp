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
#include "Inspections/AnimationSequence/AnimationSequence_CheckStatus.h"
#include "ContentInspectorSettings.h"

#include "PropertyPathHelpers.h"

namespace FCheckStatusLocal
{
	const static FName ShortFailedDescription = TEXT("Missing animation status checker");
	const static FName FullFailedDescription = TEXT("Missing animation status checker. Assign and setup as needed");
	const static bool bCriticalInspection = false;
}

void UAnimationSequence_CheckStatus::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FAnimationSequence_CheckStatus;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FAnimationSequence_CheckStatus::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		OutInspectionResult.bInspectionFailed = false;

		FString FullFailedDescription = "Animation status. Unchecked:";
		FString ShortFailedDescription = "Unchecked:";

		if (const auto AnimationStatusUserDataClass =
				Cast<UClass>(UContentInspectorSettings::Get()->GetAnimationStatusUserDataClass().TryLoad()))
		{
			if (const auto AssetUserDataInterface = Cast<IInterface_AssetUserData>(InAssetData.GetAsset()))
			{
				if (UAssetUserData* AssetUserData =
						AssetUserDataInterface->GetAssetUserDataOfClass(AnimationStatusUserDataClass))
				{
					TArray<FString> UncheckedProperties;

					for (TFieldIterator<FProperty> PropertyIterator(AnimationStatusUserDataClass); PropertyIterator;
						 ++PropertyIterator)
					{
						FProperty* CurrentProperty = *PropertyIterator;

						FString PropertyName = CurrentProperty->GetName();
						bool bPropertyValue;
						PropertyPathHelpers::GetPropertyValue(AssetUserData, PropertyName, bPropertyValue);

						if (!bPropertyValue)
						{
							PropertyName.RemoveFromStart("b", ESearchCase::CaseSensitive);
							UncheckedProperties.Add(PropertyName);
						}
					}

					for (int i = 0; i < UncheckedProperties.Num(); ++i)
					{
						const FString& UncheckProperty = UncheckedProperties[i];

						if (i == UncheckedProperties.Num() - 1)
						{
							FullFailedDescription += " " + UncheckProperty;
							ShortFailedDescription += " " + UncheckProperty;
						}
						else
						{
							FullFailedDescription += " " + UncheckProperty + ",";
							ShortFailedDescription += " " + UncheckProperty + ",";
						}
					}

					OutInspectionResult.bInspectionFailed = UncheckedProperties.Num() > 0;
					OutInspectionResult.ShortFailedDescription = *ShortFailedDescription;
					OutInspectionResult.FullFailedDescription = *FullFailedDescription;
				}
				else
				{
					OutInspectionResult.bInspectionFailed = true;
					OutInspectionResult.ShortFailedDescription = FCheckStatusLocal::ShortFailedDescription;
					OutInspectionResult.FullFailedDescription = FCheckStatusLocal::FullFailedDescription;
				}
			}
		}
		OutInspectionResult.bCriticalInspection = FCheckStatusLocal::bCriticalInspection;
		OutInspectionResult.InspectionStatus = FInspectionResult::Completed;

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
