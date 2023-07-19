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
#include "WaterPlaneInterface.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UWaterPlaneInterface : public UInterface
{
	GENERATED_BODY()
};

class FLUIDSURFACES_API IWaterPlaneInterface
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FlowMap")
	bool UpdateMaterialsTrigger();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FlowMap")
	bool StartPaint();

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "FlowMap")
	bool EndPaint();

	virtual UPrimitiveComponent* GetSurface();

	virtual UPrimitiveComponent* GetTargetSurface();

	virtual UTexture2D* GetOutputTexture();

	virtual void UpdateMaterials();
};
