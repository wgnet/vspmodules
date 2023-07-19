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
#include "Engine/DataTable.h"
#include "LODDIstancesTable.generated.h"


USTRUCT(BlueprintType)
struct FLodDistancesStruct : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere)
	float BoundingSphereSize;

	UPROPERTY(EditAnywhere, DisplayName = "LOD 0 Screen Size")
	float LOD0ScreenSize;

	UPROPERTY(EditAnywhere, DisplayName = "LOD 1 Screen Size")
	float LOD1ScreenSize;

	UPROPERTY(EditAnywhere, DisplayName = "LOD 2 Screen Size")
	float LOD2ScreenSize;

	UPROPERTY(EditAnywhere, DisplayName = "LOD 3 Screen Size")
	float LOD3ScreenSize;

	UPROPERTY(EditAnywhere, DisplayName = "LOD 4 Screen Size")
	float LOD4ScreenSize;

	UPROPERTY(EditAnywhere, DisplayName = "LOD 5 Screen Size")
	float LOD5ScreenSize;

	UPROPERTY(EditAnywhere, DisplayName = "LOD 6 Screen Size")
	float LOD6ScreenSize;

	UPROPERTY(EditAnywhere, DisplayName = "LOD 7 Screen Size")
	float LOD7ScreenSize;
};
