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

#include "Inspections/Texture/Texture_NoPowerOfTwo.h"

namespace FNoPowerOfTwoLocal
{
	const static FName ShortFailedDescription = TEXT("Missing mips");
	const static FName FullFailedDescription = TEXT("Missing mips. Texture sizes should be powers of two on the sides");
	const static bool bCriticalInspection = true;

	const static TextureGroup UITextureGroup = TEXTUREGROUP_UI;
	const static TextureMipGenSettings NoMipmapsSetting = TMGS_NoMipmaps;
	const static int32 NoMips = 1;
	const static int32 MinimumPixelOnSide = 4;
}

void UTexture_NoPowerOfTwo::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_NoPowerOfTwo;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_NoPowerOfTwo::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto TextureAsset = Cast<UTexture2D>(InAssetData.GetAsset()))
		{
			OutInspectionResult.bInspectionFailed = TextureAsset->LODGroup != FNoPowerOfTwoLocal::UITextureGroup
				&& TextureAsset->MipGenSettings != FNoPowerOfTwoLocal::NoMipmapsSetting
				&& TextureAsset->GetNumMips() == FNoPowerOfTwoLocal::NoMips
				&& TextureAsset->GetSizeX() > FNoPowerOfTwoLocal::MinimumPixelOnSide
				&& TextureAsset->GetSizeY() > FNoPowerOfTwoLocal::MinimumPixelOnSide;

			OutInspectionResult.bCriticalInspection = FNoPowerOfTwoLocal::bCriticalInspection;
			OutInspectionResult.ShortFailedDescription = FNoPowerOfTwoLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FNoPowerOfTwoLocal::FullFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}
