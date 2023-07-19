﻿/*
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
#include "Common_RedirectorExist.generated.h"

UCLASS()
class UCommon_RedirectorExist : public UInspectionBase
{
	GENERATED_BODY()

	UCommon_RedirectorExist();
	virtual void CreateInspectionTask(
		const FAssetData& InAssetData,
		FInspectionResult& OutInspectionResult,
		bool bAsynchronous = false,
		bool bCommandLetMode = false) override;
};

class FInspectionTask_RedirectorExist final : public FInspectionTask
{
public:
	virtual void DoInspection(
		const FAssetData& InAssetData,
		FInspectionResult& OutInspectionResult,
		bool bAsynchronous = false,
		bool bCommandLetMode = false) override;
};
