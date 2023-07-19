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

#include "Inspections/Texture/Texture_CorrectSettingsPerNaming.h"

namespace FCorrectSettingsPerNamingLocal
{
	const static FName ShortFailedDescription = TEXT("Incorrect Texture Import Settings");
	const static FName FullFailedDescription = TEXT(
		"Incorrect Texture Import Settings for this texture's naming.");
	const static bool bCriticalInspection = false;

	const static FString BaseColorPostfix = TEXT("_A");
	const static FString BaseColorHeightPostfix = TEXT("_AH");
	const static FString BaseColorMaskPostfix = TEXT("_AM");
	const static FString NormalRoughnessPostfix = TEXT("_NR");
	const static FString NormalRoughnessMetallicPostfix = TEXT("_NRM");
	const static FString NormalRoughnessMaskPostfix = TEXT("_NRMX");
	const static FString NormalEmptyAlphaPostfix = TEXT("_N");
	const static FString MaskPostfix = TEXT("_M");
	const static FVector4 ZeroAlphaCoverageThreshold = FVector4(0.f, 0.f, 0.f, 0.f);
	const static FVector4 NonZeroAlphaCoverageThreshold = FVector4(0.f, 0.f, 0.f, 1.f);
}

void UTexture_CorrectSettingsPerNaming::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_CorrectSettingsPerNaming;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_CorrectSettingsPerNaming::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (const auto TextureAsset = Cast<UTexture2D>(InAssetData.GetAsset()))
		{
			OutInspectionResult.bInspectionFailed = false;
			FString AssetName = TextureAsset->GetName();
			if (AssetName.EndsWith(FCorrectSettingsPerNamingLocal::BaseColorPostfix))
			{
				OutInspectionResult.bInspectionFailed = Validation_A(TextureAsset);
			}
			else if (AssetName.EndsWith(FCorrectSettingsPerNamingLocal::BaseColorHeightPostfix))
			{
				OutInspectionResult.bInspectionFailed = Validation_AH(TextureAsset);
			}
			else if (AssetName.EndsWith(FCorrectSettingsPerNamingLocal::BaseColorMaskPostfix))
			{
				OutInspectionResult.bInspectionFailed = Validation_AM(TextureAsset);
			}
			else if (AssetName.EndsWith(FCorrectSettingsPerNamingLocal::NormalRoughnessPostfix))
			{
				OutInspectionResult.bInspectionFailed = Validation_NRMX(TextureAsset);
			}
			else if (AssetName.EndsWith(FCorrectSettingsPerNamingLocal::NormalRoughnessMetallicPostfix))
			{
				OutInspectionResult.bInspectionFailed = Validation_NRMX(TextureAsset);
			}
			else if (AssetName.EndsWith(FCorrectSettingsPerNamingLocal::NormalRoughnessMaskPostfix))
			{
				OutInspectionResult.bInspectionFailed = Validation_NRMX(TextureAsset);
			}
			else if (AssetName.EndsWith(FCorrectSettingsPerNamingLocal::NormalEmptyAlphaPostfix))
			{
				OutInspectionResult.bInspectionFailed = Validation_N(TextureAsset);
			}
			else if (AssetName.EndsWith(FCorrectSettingsPerNamingLocal::MaskPostfix))
			{
				OutInspectionResult.bInspectionFailed = Validation_M(TextureAsset);
			}


			OutInspectionResult.bCriticalInspection = FCorrectSettingsPerNamingLocal::bCriticalInspection;
			OutInspectionResult.ShortFailedDescription = FCorrectSettingsPerNamingLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FCorrectSettingsPerNamingLocal::FullFailedDescription;

			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}

		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}

bool FInspectionTask_CorrectSettingsPerNaming::InCorrectMipGenSettings(
	TEnumAsByte<TextureMipGenSettings> MipGenSettings)
{
	return MipGenSettings == TMGS_NoMipmaps || MipGenSettings == TMGS_MAX || MipGenSettings == TMGS_Unfiltered
		|| MipGenSettings == TMGS_LeaveExistingMips;
}

bool FInspectionTask_CorrectSettingsPerNaming::Validation_A(const UTexture2D* TextureAsset)
{
	return TextureAsset->CompressionNoAlpha != 1 || TextureAsset->CompressionSettings != TC_Default
		|| TextureAsset->SRGB != true
		|| TextureAsset->AlphaCoverageThresholds != FCorrectSettingsPerNamingLocal::ZeroAlphaCoverageThreshold
		|| InCorrectMipGenSettings(TextureAsset->MipGenSettings)
		|| !(TextureAsset->LODGroup == TEXTUREGROUP_World
			 || TextureAsset->LODGroup == TextureGroup::TEXTUREGROUP_Project02);
}

bool FInspectionTask_CorrectSettingsPerNaming::Validation_AH(const UTexture2D* TextureAsset)
{
	return TextureAsset->CompressionNoAlpha != 0 || TextureAsset->CompressionSettings != TC_Default
		|| TextureAsset->SRGB != true
		|| TextureAsset->AlphaCoverageThresholds != FCorrectSettingsPerNamingLocal::ZeroAlphaCoverageThreshold
		|| InCorrectMipGenSettings(TextureAsset->MipGenSettings)
		|| !(TextureAsset->LODGroup == TEXTUREGROUP_World || TextureAsset->LODGroup == TEXTUREGROUP_Project02);
}

bool FInspectionTask_CorrectSettingsPerNaming::Validation_AM(const UTexture2D* TextureAsset)
{
	return TextureAsset->CompressionNoAlpha != 0 || TextureAsset->CompressionSettings != TC_Default
		|| TextureAsset->SRGB != true
		|| TextureAsset->AlphaCoverageThresholds != FCorrectSettingsPerNamingLocal::NonZeroAlphaCoverageThreshold
		|| InCorrectMipGenSettings(TextureAsset->MipGenSettings)
		|| !(TextureAsset->LODGroup == TEXTUREGROUP_World || TextureAsset->LODGroup == TEXTUREGROUP_Project02);
}

bool FInspectionTask_CorrectSettingsPerNaming::Validation_NRMX(const UTexture2D* TextureAsset)
{
	return TextureAsset->CompressionNoAlpha != 0 || TextureAsset->CompressionSettings != TC_BC7
		|| TextureAsset->SRGB != false
		|| TextureAsset->AlphaCoverageThresholds != FCorrectSettingsPerNamingLocal::ZeroAlphaCoverageThreshold
		|| InCorrectMipGenSettings(TextureAsset->MipGenSettings)
		|| !(TextureAsset->LODGroup == TEXTUREGROUP_World || TextureAsset->LODGroup == TEXTUREGROUP_Project02);
}

bool FInspectionTask_CorrectSettingsPerNaming::Validation_N(const UTexture2D* TextureAsset)
{
	return TextureAsset->CompressionNoAlpha != 0 || TextureAsset->CompressionSettings != TC_Normalmap
		|| TextureAsset->SRGB != false
		|| TextureAsset->AlphaCoverageThresholds != FCorrectSettingsPerNamingLocal::ZeroAlphaCoverageThreshold
		|| InCorrectMipGenSettings(TextureAsset->MipGenSettings)
		|| TextureAsset->LODGroup != TEXTUREGROUP_WorldNormalMap;
}

bool FInspectionTask_CorrectSettingsPerNaming::Validation_M(const UTexture2D* TextureAsset)
{
	return TextureAsset->CompressionNoAlpha != 0 || TextureAsset->CompressionSettings != TC_BC7
		|| TextureAsset->SRGB != false
		|| TextureAsset->AlphaCoverageThresholds != FCorrectSettingsPerNamingLocal::ZeroAlphaCoverageThreshold
		|| InCorrectMipGenSettings(TextureAsset->MipGenSettings) || TextureAsset->LODGroup != TEXTUREGROUP_World;
}
