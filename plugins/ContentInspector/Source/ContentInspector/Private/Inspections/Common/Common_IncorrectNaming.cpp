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

#include "Inspections/Common/Common_IncorrectNaming.h"

#include "ContentInspectorSettings.h"

namespace FIncorrectNamingLocal
{
	const static FName ShortFailedDescription = TEXT("Incorrect naming");
	const static bool bCriticalInspection = true;
}

UCommon_IncorrectNaming::UCommon_IncorrectNaming()
{
	bAssetLoadingRequired = false;
}

void UCommon_IncorrectNaming::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_IncorrectNaming;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_IncorrectNaming::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (UClass* AssetClass = InAssetData.GetClass())
			OutInspectionResult.bInspectionFailed =
				IsNamingIncorrect(AssetClass, InAssetData.AssetName, OutInspectionResult.FullFailedDescription);

		OutInspectionResult.ShortFailedDescription = FIncorrectNamingLocal::ShortFailedDescription;
		OutInspectionResult.bCriticalInspection = FIncorrectNamingLocal::bCriticalInspection;
		OutInspectionResult.InspectionStatus = FInspectionResult::Completed;

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}

bool FInspectionTask_IncorrectNaming::IsNamingIncorrect(
	const UClass* InAssetClass,
	const FName& InAssetName,
	FName& OutFullFailedDescription)
{
	bool bNamingIncorrect = false;
	FString DescriptionResult;
	const FString AssetName = InAssetName.ToString();

	const TMap<FSoftObjectPath, FName>& AssetPrefixDictionary =
		UContentInspectorSettings::Get()->GetAssetPrefixDictionary();

	TArray<FString> NamingParts;

	//	Find prefix for asset
	const FName AssetPrefix = AssetPrefixDictionary[InAssetClass];
	if (AssetPrefix != NAME_None)
	{
		FString RightSide = AssetName;
		FString LeftSide;

		//	Cut the name into parts
		while (RightSide.Split("_", &LeftSide, &RightSide, ESearchCase::IgnoreCase))
		{
			NamingParts.Add(LeftSide);
		}
		if (NamingParts.Num() > 0)
		{
			FString PrefixForCheck = NamingParts[0];
			PrefixForCheck += "_";

			//	Prefix not match
			if (AssetPrefix.ToString() != PrefixForCheck)
			{
				bNamingIncorrect = true;
			}
		}
		//	No prefix and suffix
		else
		{
			bNamingIncorrect = true;
		}

		if (bNamingIncorrect)
		{
			DescriptionResult = FString::Printf(
				TEXT("Incorrect naming. Asset type '%s' requires a '%s' prefix"),
				*InAssetClass->GetName(),
				*AssetPrefix.ToString());
		}
	}

	//	Check empty suffix
	const FString& EmptySuffix = AssetName.Right(1);
	if (EmptySuffix == "_")
	{
		bNamingIncorrect = true;

		if (!DescriptionResult.IsEmpty())
			DescriptionResult += " / ";

		DescriptionResult += FString::Printf(TEXT("Need to remove the '_' suffix"));
	}

	//	Suffixes will be checked here
	// if (NamingParts.Num() > 1)
	// {
	// 	FString Suffix = "_";
	// 	Suffix += NamingParts.Last();
	// }

	OutFullFailedDescription = *DescriptionResult;
	return bNamingIncorrect;
}
