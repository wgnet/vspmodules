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

#include "Inspections/MaterialInstance/MaterialInstance_ParamOverrideWithDefault.h"

#include "ContentInspectorSettings.h"
#include "Materials/MaterialInstance.h"

namespace FParamOverrideWithDefaultLocal
{
	const static FName ShortFailedDescription = TEXT("Parameter overriden using default value");
	const static FName FullFailedDescription =
		TEXT("Parameter overriden using default value. Deactivate parameter before saving");
	// We need this exception atm cuz Epics left this built-in-engine parameter as active override in any MatInstance by default
	const static FName ExceptionParamName = TEXT("RefractionDepthBias");
	const static bool bCriticalInspection = false;
}

void UMaterialInstance_ParamOverrideWithDefault::CreateInspectionTask(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionTask = new FInspectionTask_ParamOverrideWithDefault;
	InspectionTask->DoInspection(InAssetData, OutInspectionResult, bAsynchronous);
}

void FInspectionTask_ParamOverrideWithDefault::DoInspection(
	const FAssetData& InAssetData,
	FInspectionResult& OutInspectionResult,
	bool bAsynchronous,
	bool bCommandLetMode)
{
	auto InspectionBody = [InAssetData, &OutInspectionResult, this]()
	{
		if (UMaterialInstance* MaterialInstanceAsset = Cast<UMaterialInstance>(InAssetData.GetAsset()))
		{
			OutInspectionResult.bInspectionFailed = false;

			if (const UMaterial* MeshPainterMasterShader =
					Cast<UMaterial>(UContentInspectorSettings::Get()->GetPainterMasterShader().TryLoad()))
			{
				if (MaterialInstanceAsset->GetMaterial() != MeshPainterMasterShader)
				{
					if (UMaterialInstance* ParentInstance = Cast<UMaterialInstance>(MaterialInstanceAsset->Parent))
					{
						CompareWithParentInstance(
							MaterialInstanceAsset,
							ParentInstance,
							OutInspectionResult.bInspectionFailed);
					}
					else
					{
						CompareWithParentMaterial(MaterialInstanceAsset, OutInspectionResult.bInspectionFailed);
					}
				}
			}
			else if (UMaterialInstance* ParentInstance = Cast<UMaterialInstance>(MaterialInstanceAsset->Parent))
			{
				CompareWithParentInstance(MaterialInstanceAsset, ParentInstance, OutInspectionResult.bInspectionFailed);
			}
			else
			{
				CompareWithParentMaterial(MaterialInstanceAsset, OutInspectionResult.bInspectionFailed);
			}

			OutInspectionResult.ShortFailedDescription = FParamOverrideWithDefaultLocal::ShortFailedDescription;
			OutInspectionResult.FullFailedDescription = FParamOverrideWithDefaultLocal::FullFailedDescription;
			OutInspectionResult.bCriticalInspection = FParamOverrideWithDefaultLocal::bCriticalInspection;
			OutInspectionResult.InspectionStatus = FInspectionResult::Completed;
		}
		delete this;
	};

	if (bAsynchronous)
		Async(EAsyncExecution::TaskGraphMainThread, MoveTemp(InspectionBody));
	else
		InspectionBody();
}

void FInspectionTask_ParamOverrideWithDefault::CompareWithParentMaterial(
	UMaterialInstance* MaterialInstanceAsset,
	bool& Result)
{
	for (FScalarParameterValue Param : MaterialInstanceAsset->ScalarParameterValues)
	{
		float Value;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);

		if (MaterialInstanceAsset->GetScalarParameterValue(HashedInfo, Value, true)
			&& MaterialInstanceAsset->GetScalarParameterDefaultValue(HashedInfo, Value))
		{
			if (Param.ParameterValue == Value)
				Result = true;
		}
	}
	for (FVectorParameterValue Param : MaterialInstanceAsset->VectorParameterValues)
	{
		FLinearColor Value;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);

		if (MaterialInstanceAsset->GetVectorParameterValue(HashedInfo, Value, true)
			&& MaterialInstanceAsset->GetVectorParameterDefaultValue(HashedInfo, Value))
		{
			if (Param.ParameterValue == Value)
				Result = true;
		}
	}
	for (FTextureParameterValue Param : MaterialInstanceAsset->TextureParameterValues)
	{
		UTexture* Value;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);

		if (MaterialInstanceAsset->GetTextureParameterValue(HashedInfo, Value, true)
			&& MaterialInstanceAsset->GetTextureParameterDefaultValue(HashedInfo, Value))
		{
			if (Param.ParameterValue == Value)
				Result = true;
		}
	}
	for (FRuntimeVirtualTextureParameterValue Param : MaterialInstanceAsset->RuntimeVirtualTextureParameterValues)
	{
		URuntimeVirtualTexture* Value;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);

		if (MaterialInstanceAsset->GetRuntimeVirtualTextureParameterValue(HashedInfo, Value, true)
			&& MaterialInstanceAsset->GetRuntimeVirtualTextureParameterDefaultValue(HashedInfo, Value))
		{
			if (Param.ParameterValue == Value)
				Result = true;
		}
	}
	for (FFontParameterValue Param : MaterialInstanceAsset->FontParameterValues)
	{
		UFont* Value;
		int32 FontPageValue;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);

		if (MaterialInstanceAsset->GetFontParameterValue(HashedInfo, Value, FontPageValue, true)
			&& MaterialInstanceAsset->GetFontParameterDefaultValue(HashedInfo, Value, FontPageValue))
		{
			if (Param.FontValue == Value && Param.FontPage == FontPageValue)
				Result = true;
		}
	}
	TArray<FMaterialParameterInfo> ParamInfos;
	TArray<FGuid> SwitchParamIDs;
	MaterialInstanceAsset->GetAllStaticSwitchParameterInfo(ParamInfos, SwitchParamIDs);
	for (FMaterialParameterInfo ParamInfo : ParamInfos)
	{
		bool Value;
		bool DefaultValue;
		FGuid Guid;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(ParamInfo);
		if (MaterialInstanceAsset->GetStaticSwitchParameterValue(HashedInfo, Value, Guid, true)
			&& MaterialInstanceAsset->GetStaticSwitchParameterDefaultValue(HashedInfo, DefaultValue, Guid))
		{
			if (DefaultValue == Value)
				Result = true;
		}
	}
	MaterialInstanceAsset->GetAllStaticComponentMaskParameterInfo(ParamInfos, SwitchParamIDs);
	for (FMaterialParameterInfo ParamInfo : ParamInfos)
	{
		bool R;
		bool G;
		bool B;
		bool A;
		bool ParR;
		bool ParG;
		bool ParB;
		bool ParA;
		FGuid Guid;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(ParamInfo);
		if (MaterialInstanceAsset->GetStaticComponentMaskParameterValue(HashedInfo, R, G, B, A, Guid, true)
			&& MaterialInstanceAsset->GetStaticComponentMaskParameterDefaultValue(
				HashedInfo,
				ParR,
				ParG,
				ParB,
				ParA,
				Guid))
		{
			if (R == ParR && G == ParG && B == ParB && A == ParA)
				Result = true;
		}
	}
}

void FInspectionTask_ParamOverrideWithDefault::CompareWithParentInstance(
	UMaterialInstance* MaterialInstanceAsset,
	UMaterialInstance* ParentInstance,
	bool& Result)
{
	for (FScalarParameterValue Param : MaterialInstanceAsset->ScalarParameterValues)
	{
		float Value;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);
		float ParentValue;
		if (MaterialInstanceAsset->GetScalarParameterValue(HashedInfo, Value, true)
			&& ParentInstance->GetScalarParameterValue(HashedInfo, ParentValue))
		{
			if (ParentValue == Value && Param.ParameterInfo.Name != FParamOverrideWithDefaultLocal::ExceptionParamName)
				Result = true;
		}
	}
	for (FVectorParameterValue Param : MaterialInstanceAsset->VectorParameterValues)
	{
		FLinearColor Value;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);
		FLinearColor ParentValue;
		if (MaterialInstanceAsset->GetVectorParameterValue(HashedInfo, Value, true)
			&& ParentInstance->GetVectorParameterValue(HashedInfo, ParentValue))
		{
			if (ParentValue == Value)
				Result = true;
		}
	}
	for (FTextureParameterValue Param : MaterialInstanceAsset->TextureParameterValues)
	{
		UTexture* Value;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);
		UTexture* ParentValue;
		if (MaterialInstanceAsset->GetTextureParameterValue(HashedInfo, Value, true)
			&& ParentInstance->GetTextureParameterValue(HashedInfo, ParentValue))
		{
			if (ParentValue == Value)
				Result = true;
		}
	}
	for (FRuntimeVirtualTextureParameterValue Param : MaterialInstanceAsset->RuntimeVirtualTextureParameterValues)
	{
		URuntimeVirtualTexture* Value;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);
		URuntimeVirtualTexture* ParentValue;
		if (MaterialInstanceAsset->GetRuntimeVirtualTextureParameterValue(HashedInfo, Value, true)
			&& ParentInstance->GetRuntimeVirtualTextureParameterValue(HashedInfo, ParentValue))
		{
			if (ParentValue == Value)
				Result = true;
		}
	}
	for (FFontParameterValue Param : MaterialInstanceAsset->FontParameterValues)
	{
		UFont* Value;
		int32 FontPageValue;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(Param.ParameterInfo);

		UFont* ParentValue;
		int32 FontPageParentValue;
		if (MaterialInstanceAsset->GetFontParameterValue(HashedInfo, Value, FontPageValue, true)
			&& ParentInstance->GetFontParameterValue(HashedInfo, ParentValue, FontPageParentValue))
		{
			if (ParentValue == Value && FontPageValue == FontPageParentValue)
				Result = true;
		}
	}
	TArray<FMaterialParameterInfo> ParamInfos;
	TArray<FGuid> SwitchParamIDs;
	MaterialInstanceAsset->GetAllStaticSwitchParameterInfo(ParamInfos, SwitchParamIDs);
	for (FMaterialParameterInfo ParamInfo : ParamInfos)
	{
		bool Value;
		bool ParentValue;
		FGuid Guid;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(ParamInfo);
		if (MaterialInstanceAsset->GetStaticSwitchParameterValue(HashedInfo, Value, Guid, true)
			&& ParentInstance->GetStaticSwitchParameterValue(HashedInfo, ParentValue, Guid))
		{
			if (ParentValue == Value)
				Result = true;
		}
	}
	MaterialInstanceAsset->GetAllStaticComponentMaskParameterInfo(ParamInfos, SwitchParamIDs);
	for (FMaterialParameterInfo ParamInfo : ParamInfos)
	{
		bool R;
		bool G;
		bool B;
		bool A;
		bool ParR;
		bool ParG;
		bool ParB;
		bool ParA;
		FGuid Guid;
		FHashedMaterialParameterInfo HashedInfo = FHashedMaterialParameterInfo(ParamInfo);
		if (MaterialInstanceAsset->GetStaticComponentMaskParameterValue(HashedInfo, R, G, B, A, Guid, true)
			&& ParentInstance->GetStaticComponentMaskParameterValue(HashedInfo, ParR, ParG, ParB, ParA, Guid))
		{
			if (R == ParR && G == ParG && B == ParB && A == ParA)
				Result = true;
		}
	}
}
