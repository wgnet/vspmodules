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

#include "TerraformerEditorSettings.generated.h"

UCLASS(config = TerraformerEditor, defaultconfig, meta = (DisplayName = "Terraformer Editor"))
class TERRAFORMER_API UTerraformerEditorSettings : public UDeveloperSettings
{
	GENERATED_BODY()
public:
	virtual FName GetCategoryName() const override
	{
		return FName(TEXT("Plugins"));
	}

	int32 GetSelectedSegmentLineThickness() const;

	int32 GetUnselectedSegmentLineThickness() const;

	int32 GetSplineHandleSize() const;

	int32 GetStampLineThickness() const;

private:
	UPROPERTY(EditAnywhere, config, Category = Spline)
	int32 SelectedSegmentLineThickness = 64;

	UPROPERTY(EditAnywhere, config, Category = Spline)
	int32 UnselectedSegmentLineThickness = 32;

	UPROPERTY(EditAnywhere, config, Category = Spline)
	int32 SplineHandleSize = 45;

	UPROPERTY(EditAnywhere, config, Category = Stamp)
	int32 StampLineThickness = 50;
};
