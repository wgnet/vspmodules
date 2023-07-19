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

#include "GameFramework/Actor.h"
#include "FluidSurfaceShape.generated.h"

class USplineComponent;

UCLASS()
class FLUIDSURFACES_API AFluidSurfaceShape : public AActor
{
	GENERATED_BODY()

public:
	AFluidSurfaceShape();

	UPROPERTY(EditInstanceOnly)
	USplineComponent* Shape;

	UPROPERTY(EditInstanceOnly, BlueprintReadOnly)
	float SegmentSize = 2048.f;

	void GetPoints(TArray<FVector>& Positions, TArray<int32>& Indices) const;
};
