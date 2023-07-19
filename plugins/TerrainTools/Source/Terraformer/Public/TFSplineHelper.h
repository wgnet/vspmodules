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

#include "TFSplineHelper.generated.h"

USTRUCT(BlueprintType)
struct FStartEndScale
{
	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "Start", Category = "TFSplineHelper")
	FVector2D Start;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, DisplayName = "End", Category = "TFSplineHelper")
	FVector2D End;

	FStartEndScale()
	{
		Start = FVector2D(1, 1);
		End = FVector2D(1, 1);
	}

	GENERATED_BODY()
};

class USplineComponent;

UCLASS()
class TERRAFORMER_API UTFSplineHelper : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()
public:
	/** Input a spline and ideal subdivision length to get number of subdivisions and calculated length. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Calculate Subdivided Spline"), Category = "SplineHelper")
	static void CalcRailLength(
		const USplineComponent* Spline,
		int32& Number,
		float& Length,
		const float IdealLength = 10);

	/** Input a spline with index and section length to calculate start and end locations and tangents for a spline mesh */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Calculate Start End"), Category = "SplineHelper")
	static void CalcStartEnd(
		const USplineComponent* Spline,
		FVector& LocStart,
		FVector& TanStart,
		FVector& LocEnd,
		FVector& TanEnd,
		const int32 Index,
		const float Length = 10);

	/** Input a spline to calculate the rotation of a spline mesh from the current 'Up Vector' to its end location. */
	UFUNCTION(BlueprintPure, meta = (DisplayName = "Calc Rot from Up Vector"), Category = "SplineHelper")
	static void CalcRotFromUp(
		float& Rotation,
		const USplineComponent* Spline,
		const int32 Index,
		const float Length = 10);

	/** Input a spline and SplineMesh to set its start and end along with twisting */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Configure SplineMesh"), Category = "SplineHelper")
	static void ConfigSplineMesh(
		const int32& Index,
		const float& Length,
		const USplineComponent* SplineFinal,
		class USplineMeshComponent* SplineMesh,
		const AActor* Actor,
		UStaticMesh* StaticMesh,
		const FStartEndScale StartEndScale,
		const float Roll = 0);

	/** Input a Spline to offset with offset distance and rotation from the spline's up vector at each point */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Build Offset Spline"), Category = "SplineHelper")
	static void BuildOffsetSpline(
		const USplineComponent* SplineUser,
		USplineComponent* SplineOffset,
		const float RotFromUp = 0,
		const float OffsetDist = 30);

	/** Input an offset spline to fix the tangents by comparing length of user made spline */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Fix Tangents of Offset Spline"), Category = "SplineHelper")
	static void FixTangents(const USplineComponent* SplineUser, USplineComponent* SplineOffset);

	/** Builds a final spline using a user spline and offset spline. Final spline should be heavily subdivided to avoid twisting */
	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Build Corrected Spline"), Category = "SplineHelper")
	static void BuildCorrectedSpline(
		const USplineComponent* SplineUser,
		const USplineComponent* SplineOffset,
		USplineComponent* SplineFinal,
		const float IdealLength = 100);

	UFUNCTION(BlueprintCallable, meta = (DisplayName = "Get Spline Mesh Vertex Position"), Category = "SplineHelper")
	static void GetSplineMeshVertexPositionWorldSpace(
		const USplineMeshComponent* SplineMeshComponent,
		TArray<FVector>& OutVertexPosition);
};
