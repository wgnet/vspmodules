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
#include "FXContainerTypes.generated.h"

struct FFXIntParam;
struct FFXVectorParam;
struct FFXParametersParams;
struct FFXFloatParam;
class UFXSystemComponent;

// TODO: consider making this struct templated
USTRUCT()
struct FFXComponentContainer
{
	GENERATED_BODY()

	FFXComponentContainer() = default;
	explicit FFXComponentContainer(UFXSystemComponent* FXComponent);
	explicit FFXComponentContainer(const TArray<UFXSystemComponent*>::SizeType& InSize);

	void Add(UFXSystemComponent* FXComponent);
	void Append(const FFXComponentContainer& Other);
	void ReleaseToPool();
	void Deactivate();
	void Destroy();
	int32 Num() const;
	bool IsEmpty() const;
	void CleanUp();

	template<typename TComparisonType>
	bool Contains(const TComparisonType& Item);

	template<typename TRemoveType>
	void RemoveFXComponents(TRemoveType const& InFXData);

	void UpdateFXIntParameter(const FFXIntParam& InParam);
	void UpdateFXFloatParameter(const FFXFloatParam& InParam);
	void UpdateFXVectorParameter(const FFXVectorParam& InParam);
	void UpdateFXParameters(const FFXParametersParams& InParametersData);
	void UpdateRotationAndLocation(const FName& LocationParamName, const FName& RotationParamName);

	TArray<UFXSystemComponent*>& GetComponents();
	const TArray<UFXSystemComponent*>& GetComponents() const;

	FFXComponentContainer& operator+=(const FFXComponentContainer& Rhs);
	FFXComponentContainer& operator+=(FFXComponentContainer&& Rhs);
	FORCEINLINE UFXSystemComponent*& operator[](int32 Index)
	{
		return FXComponents[Index];
	};
	FORCEINLINE UFXSystemComponent* const& operator[](int32 Index) const
	{
		return FXComponents[Index];
	};

	void SetEmitterEnable(FName const& EmitterName, bool bEnabled);

private:
	// TODO: maybe consider adding a custom iterator, which will be able to remove invalid FXs as we cycle throw them
	UPROPERTY()
	TArray<UFXSystemComponent*> FXComponents {};
};

USTRUCT()
struct FFXBeamEndPoint
{
	GENERATED_BODY()

	UPROPERTY()
	USceneComponent* PointComponent = nullptr;

	FName PointSocketName = NAME_None;
	FName PointBeamParameterName = NAME_None;
	FVector PointLocation = FVector::ZeroVector;

	FVector GetUpdatedLocation() const;
	void SetNewLocation(const FVector& InNewLocation);
};

USTRUCT()
struct FFXBeamContext
{
	GENERATED_BODY()

	UPROPERTY()
	UFXSystemComponent* FXComponent = nullptr;

	UPROPERTY()
	FFXBeamEndPoint SourcePoint {};

	UPROPERTY()
	FFXBeamEndPoint TargetPoint {};

	bool IsValid() const;
	UFXSystemComponent* operator->();
};

USTRUCT()
struct FFXBeamContainer
{
	GENERATED_BODY()

	FFXBeamContainer() = default;
	explicit FFXBeamContainer(const FFXBeamContext& BeamContext);
	explicit FFXBeamContainer(const TArray<FFXBeamContext>::SizeType& InSize);

	void Add(const FFXBeamContext& BeamContext);
	void Append(const FFXBeamContainer& Other);
	void ReleaseToPool();
	int32 Num() const;

	void UpdateBeams();
	void UpdateBeams(const FVector& NewSourceLocation, const FVector NewTargetLocation);

	void UpdateFXFloatParameter(const FFXFloatParam& InParam);
	void UpdateFXVectorParameter(const FFXVectorParam& InParam);
	void UpdateFXParameters(const FFXParametersParams& InParametersData);

private:
	UPROPERTY()
	TArray<FFXBeamContext> BeamContexts {};
};

#if CPP
	#include "FXContainerTypes.inl"
#endif
