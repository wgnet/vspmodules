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
#include "TerrainMaterialProxyData.generated.h"

USTRUCT()
struct FTerrainMaterialInfo
{
	friend bool operator==(const FTerrainMaterialInfo& Lhs, const FTerrainMaterialInfo& RHS)
	{
		return Lhs.ParameterName == RHS.ParameterName && Lhs.ParameterIndex == RHS.ParameterIndex
			&& Lhs.Texture == RHS.Texture;
	}

	friend bool operator!=(const FTerrainMaterialInfo& Lhs, const FTerrainMaterialInfo& RHS)
	{
		return !(Lhs == RHS);
	}

	GENERATED_BODY()

	UPROPERTY(VisibleAnywhere)
	FName ParameterName;

	UPROPERTY(VisibleAnywhere)
	int32 ParameterIndex;

	UPROPERTY(VisibleAnywhere)
	int32 MaxIndex;

	UPROPERTY(VisibleAnywhere)
	UTexture2D* Texture;
};