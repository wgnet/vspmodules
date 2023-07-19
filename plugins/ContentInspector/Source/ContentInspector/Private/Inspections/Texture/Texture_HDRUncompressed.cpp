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

#include "Inspections/Texture/Texture_HDRUncompressed.h"

namespace FHDRUncompressedLocal
{
	const static FName ShortFailedDescription = TEXT("Large HDR Texture uncompressed");
	const static FName FullFailedDescription = TEXT("Large HDR Texture uncompressed. Change format to compressed HDR");
	const static bool bCriticalInspection = true;

	const static int32 MaximumTextureSize = 512 * 512;
}

void UTexture_HDRUncompressed::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_HDRUncompressed;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_HDRUncompressed::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto TextureAsset = Cast<UTexture>(InAssetData.GetAsset()))
		{
			if (TextureAsset->CompressionSettings == TC_HDR
				&& TextureAsset->CalcTextureMemorySizeEnum(TMC_MAX) > FHDRUncompressedLocal::MaximumTextureSize)
			{
				OutInspectionResult.bInspectionFailed = true;
			}

			OutInspectionResult.bCriticalInspection = FHDRUncompressedLocal::bCriticalInspection;
			OutInspectionResult.ShortFailedDescription = FHDRUncompressedLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FHDRUncompressedLocal::FullFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
