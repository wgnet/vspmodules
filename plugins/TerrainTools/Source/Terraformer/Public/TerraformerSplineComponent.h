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

#include "Components/SplineComponent.h"
#include "TerraformerSplineComponent.generated.h"

UCLASS(HideCategories = (Navigation, "Tags", Activation, Cooking, HLOD, "AssetUserData"))
class TERRAFORMER_API UTerraformerSplineComponent : public USplineComponent
{
	GENERATED_BODY()
public:
	UTerraformerSplineComponent();

	virtual bool CanEditChange(const FProperty* InProperty) const override;

	virtual TArray<ESplinePointType::Type> GetEnabledSplinePointTypes() const override;

	virtual bool AllowsSplinePointScaleEditing() const override;

	virtual FPrimitiveSceneProxy* CreateSceneProxy() override;

	DECLARE_EVENT(UTerraformerSplineComponent, FOnSplineDataChanged);
	FOnSplineDataChanged& OnSplineDataChanged()
	{
		return SplineDataChangedEvent;
	}

private:
	FOnSplineDataChanged SplineDataChangedEvent;
};
