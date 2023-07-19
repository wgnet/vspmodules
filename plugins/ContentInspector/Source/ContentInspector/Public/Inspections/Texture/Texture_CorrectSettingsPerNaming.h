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

#include "Inspections/InspectionBase.h"
#include "Texture_CorrectSettingsPerNaming.generated.h"

UCLASS()
class CONTENTINSPECTOR_API UTexture_CorrectSettingsPerNaming : public UInspectionBase
{
	virtual void CreateInspectionTask(
		const FAssetData& InAssetData,
		FInspectionResult& OutInspectionResult,
		bool bAsynchronous = false,
		bool bCommandLetMode = false) override;

	GENERATED_BODY()
};

class FInspectionTask_CorrectSettingsPerNaming final : public FInspectionTask
{
public:
	virtual void DoInspection(
		const FAssetData& InAssetData,
		FInspectionResult& OutInspectionResult,
		bool bAsynchronous = false,
		bool bCommandLetMode = false) override;

	bool InCorrectMipGenSettings(TEnumAsByte<TextureMipGenSettings> MipGenSettings);

	bool Validation_A(const UTexture2D* TextureAsset);
	bool Validation_AH(const UTexture2D* TextureAsset);
	bool Validation_AM(const UTexture2D* TextureAsset);
	bool Validation_NRMX(const UTexture2D* TextureAsset);
	bool Validation_N(const UTexture2D* TextureAsset);
	bool Validation_M(const UTexture2D* TextureAsset);
};
