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
#include "Engine/DeveloperSettings.h"

#include "Inspections/InspectionBase.h"
#include "ContentInspectorSettings.generated.h"

USTRUCT()
struct FInspectionsForClass
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	FSoftClassPath ClassForInspection;

	UPROPERTY(EditAnywhere)
	TArray<TSubclassOf<UInspectionBase>> Inspections;
};

UCLASS(Config = VSPEditor, DefaultConfig, meta = (DisplayName = "Content Inspector Settings"))
class CONTENTINSPECTOR_API UContentInspectorSettings : public UDeveloperSettings
{
	GENERATED_BODY()

public:
	static const UContentInspectorSettings* Get();
	TArray<FInspectionsForClass> GetClassesAndInspections() const;
	const FName& GetConfluenceURL(const TSoftClassPtr<UInspectionBase>& InInspectionClass) const;
	bool CanRejectSavingAsset() const;
	TMap<FSoftObjectPath, FName> GetAssetPrefixDictionary() const;
	FSoftObjectPath GetAssetRegisterMarkClass() const;
	FSoftObjectPath GetPainterMasterShader() const;
	FSoftObjectPath GetAnimationStatusUserDataClass() const;
	TArray<FDirectoryPath> GetRetainOnLoadIncludePathsArray() const;
	TArray<FDirectoryPath> GetInspectExcludeFolders() const;
	float GetMinimumPolyCountDifferenceValue() const;

private:
	FName EmptyURL = NAME_None;

	//	Can reject saving assets that have not passed the inspections
	UPROPERTY(EditAnywhere, Category = General)
	bool bRejectSavingAssets = true;

	UPROPERTY(EditAnywhere, Config, Category = General, meta = (TitleProperty = ClassForInspection))
	TArray<FInspectionsForClass> ClassesAndInspections;

	UPROPERTY(EditAnywhere, Config, Category = General)
	TMap<TSubclassOf<UInspectionBase>, FName> ConfluenceURLs;

	//	Common_IncorrectNaming Inspection
	UPROPERTY(EditAnywhere, Config, Category = "Inspection Settings|Asset Names", meta = (DisplayName = "Asset Prefix Dictionary"))
	TMap<FSoftObjectPath, FName> AssetPrefixDictionary;
	//	Common_IncorrectNaming Inspection

	// UCommon_NoAssetRegisterMark
	UPROPERTY(EditAnywhere, Config, Category = "Inspection Settings|Asset Register", meta = (DisplayName = "Asset Register Mark Class"))
	FSoftObjectPath AssetRegisterMarkClass;
	// UCommon_NoAssetRegisterMark

	// UMaterialinstance_ParamOverrideWithDefault
	UPROPERTY(EditAnywhere, Config, Category = "Inspection Settings|Asset Register", meta = (DisplayName = "VSPMeshPainter Master Shader"))
	FSoftObjectPath MeshPainterMasterShader;
	// UCommon_NoAssetRegisterMark

	//	UAnimationSequence_CheckStatus
	UPROPERTY(EditAnywhere, Config, Category = "Inspection Settings|Asset Register", meta = (DisplayName = "Animation Status Asset User Data Class"))
	FSoftObjectPath AnimationStatusUserDataClass;
	//	UAnimationSequence_CheckStatus

	//	Sound_LoadingBehaviorOverride
	UPROPERTY(EditAnywhere, Config, Category = "Inspection Settings Settings|Sound", meta = (DisplayName = "Retain On Load Include Paths Array", RelativeToGameContentDir, LongPackageName))
	TArray<FDirectoryPath> RetainOnLoadIncludePathsArray;

	//	Sound_LoadingBehaviorOverride

	UPROPERTY(EditAnywhere, Config, Category = "Settings|Global Settings", meta = (DisplayName = "In these folders Content Inspector is not inspect"))
	TArray<FDirectoryPath> InspectExcludeFolders;

	UPROPERTY(EditAnywhere, Config, Category = "Settings|Global Settings", meta = (DisplayName = "Minimum Polygon Count Difference with preveiw LOD (in %)"))
	float MinimumPolyCountDifference;

	virtual FName GetCategoryName() const override;
	virtual FName GetSectionName() const override;
};
