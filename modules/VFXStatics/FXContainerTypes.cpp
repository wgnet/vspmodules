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

#include "FXContainerTypes.h"
#include "FXSpawnLibrary.h"

#include "Particles/ParticleSystemComponent.h"

FFXComponentContainer::FFXComponentContainer(UFXSystemComponent* FXComponent)
{
	Add(FXComponent);
}

FFXComponentContainer::FFXComponentContainer(const TArray<UFXSystemComponent*>::SizeType& InSize)
{
	FXComponents.Reserve(InSize);
}

void FFXComponentContainer::Add(UFXSystemComponent* FXComponent)
{
	if (IsValid(FXComponent))
	{
		FXComponents.Add(FXComponent);
	}
}

void FFXComponentContainer::Append(const FFXComponentContainer& Other)
{
	FXComponents.Append(Other.FXComponents);
}

void FFXComponentContainer::ReleaseToPool()
{
	for (auto It = FXComponents.CreateIterator(); It; ++It)
	{
		UFXSystemComponent*& FXComponent = *It;
		if (IsValid(FXComponent))
		{
			FXComponent->ReleaseToPool();
		}
	}

	FXComponents.Reset();
}

void FFXComponentContainer::Deactivate()
{
	for (auto It = FXComponents.CreateIterator(); It; ++It)
	{
		UFXSystemComponent*& FXComponent = *It;
		if (IsValid(FXComponent))
		{
			FXComponent->Deactivate();
		}
	}

	FXComponents.Reset();
}

void FFXComponentContainer::Destroy()
{
	for (auto It = FXComponents.CreateIterator(); It; ++It)
	{
		UFXSystemComponent*& FXComponent = *It;
		if (IsValid(FXComponent))
		{
			FXComponent->DestroyComponent();
		}
	}

	FXComponents.Reset();
}

int32 FFXComponentContainer::Num() const
{
	return FXComponents.Num();
}

bool FFXComponentContainer::IsEmpty() const
{
	return FXComponents.Num() == 0;
}

void FFXComponentContainer::CleanUp()
{
	for (auto It = FXComponents.CreateIterator(); It; ++It)
	{
		UFXSystemComponent const* FXComponent = *It;
		if (!(IsValid(FXComponent) && FXComponent->IsActive()))
		{
			It.RemoveCurrent();
		}
	}
}

void FFXComponentContainer::UpdateFXIntParameter(const FFXIntParam& InParam)
{
	for (auto& FXComponent : FXComponents)
	{
		FFXSpawnLibrary::UpdateFXIntParameter(FXComponent, InParam);
	}
}

void FFXComponentContainer::UpdateFXFloatParameter(const FFXFloatParam& InParam)
{
	for (auto& FXComponent : FXComponents)
	{
		FFXSpawnLibrary::UpdateFXFloatParameter(FXComponent, InParam);
	}
}

void FFXComponentContainer::UpdateFXVectorParameter(const FFXVectorParam& InParam)
{
	for (auto& FXComponent : FXComponents)
	{
		FFXSpawnLibrary::UpdateFXVectorParameter(FXComponent, InParam);
	}
}

void FFXComponentContainer::UpdateFXParameters(const FFXParametersParams& InParametersData)
{
	for (auto& FXComponent : FXComponents)
	{
		FFXSpawnLibrary::UpdateFXParameters(FXComponent, InParametersData);
	}
}

void FFXComponentContainer::UpdateRotationAndLocation(const FName& LocationParamName, const FName& RotationParamName)
{
	for (const auto& FXComponent : FXComponents)
	{
		if (IsValid(FXComponent) && IsValid(FXComponent->GetAttachParent()))
		{
			const FName AttachSocketName = FXComponent->GetAttachSocketName();
			const FTransform SocketTransform =
				FXComponent->GetAttachParent()->GetSocketTransform(AttachSocketName, RTS_World);

			if (LocationParamName != NAME_None)
			{
				FFXSpawnLibrary::UpdateFXVectorParameter(
					FXComponent,
					{ LocationParamName, SocketTransform.GetLocation() });
			}

			if (RotationParamName != NAME_None)
			{
				FFXSpawnLibrary::UpdateFXVectorParameter(
					FXComponent,
					{ RotationParamName, SocketTransform.Rotator().Vector() });
			}
		}
	}
}

TArray<UFXSystemComponent*>& FFXComponentContainer::GetComponents()
{
	return FXComponents;
}

const TArray<UFXSystemComponent*>& FFXComponentContainer::GetComponents() const
{
	return FXComponents;
}

FFXComponentContainer& FFXComponentContainer::operator+=(const FFXComponentContainer& Rhs)
{
	FXComponents.Append(Rhs.FXComponents);

	return *this;
}

FFXComponentContainer& FFXComponentContainer::operator+=(FFXComponentContainer&& Rhs)
{
	FXComponents.Append(MoveTemp(Rhs.FXComponents));

	return *this;
}

void FFXComponentContainer::SetEmitterEnable(FName const& EmitterName, bool bEnabled)
{
	for (UFXSystemComponent* FXComponent : FXComponents)
	{
		if (IsValid(FXComponent))
		{
			FXComponent->SetEmitterEnable(EmitterName, bEnabled);
		}
	}
}

FVector FFXBeamEndPoint::GetUpdatedLocation() const
{
	if (IsValid(PointComponent))
	{
		const FName SocketName = PointSocketName;
		return PointComponent->GetSocketLocation(SocketName);
	}

	return PointLocation;
}

void FFXBeamEndPoint::SetNewLocation(const FVector& InNewLocation)
{
	PointComponent = nullptr;
	PointSocketName = NAME_None;
	PointLocation = InNewLocation;
}

bool FFXBeamContext::IsValid() const
{
	return true;
}

UFXSystemComponent* FFXBeamContext::operator->()
{
	return FXComponent;
}

FFXBeamContainer::FFXBeamContainer(const FFXBeamContext& BeamContext)
{
	Add(BeamContext);
}

FFXBeamContainer::FFXBeamContainer(const TArray<FFXBeamContext>::SizeType& InSize)
{
	BeamContexts.Reserve(InSize);
}

void FFXBeamContainer::Add(const FFXBeamContext& BeamContext)
{
	if (IsValid(BeamContext.FXComponent))
	{
		BeamContexts.Add(BeamContext);
	}
}

void FFXBeamContainer::Append(const FFXBeamContainer& Other)
{
	BeamContexts.Append(Other.BeamContexts);
}

void FFXBeamContainer::ReleaseToPool()
{
	for (const auto& BeamContext : BeamContexts)
	{
		if (BeamContext.FXComponent)
		{
			BeamContext.FXComponent->ReleaseToPool();
		}
	}
	BeamContexts.Empty();
}

int32 FFXBeamContainer::Num() const
{
	return BeamContexts.Num();
}

void FFXBeamContainer::UpdateBeams()
{
	for (const auto& BeamContext : BeamContexts)
	{
		FFXSpawnLibrary::UpdateFXBeam(BeamContext);
	}
}

void FFXBeamContainer::UpdateBeams(const FVector& NewSourceLocation, const FVector NewTargetLocation)
{
	for (auto& BeamContext : BeamContexts)
	{
		BeamContext.SourcePoint.SetNewLocation(NewSourceLocation);
		BeamContext.TargetPoint.SetNewLocation(NewTargetLocation);
		FFXSpawnLibrary::UpdateFXBeam(BeamContext);
	}
}

void FFXBeamContainer::UpdateFXFloatParameter(const FFXFloatParam& InParam)
{
	for (auto& BeamContext : BeamContexts)
	{
		FFXSpawnLibrary::UpdateFXFloatParameter(BeamContext.FXComponent, InParam);
	}
}

void FFXBeamContainer::UpdateFXVectorParameter(const FFXVectorParam& InParam)
{
	for (auto& BeamContext : BeamContexts)
	{
		FFXSpawnLibrary::UpdateFXVectorParameter(BeamContext.FXComponent, InParam);
	}
}

void FFXBeamContainer::UpdateFXParameters(const FFXParametersParams& InParametersData)
{
	for (auto& BeamContext : BeamContexts)
	{
		FFXSpawnLibrary::UpdateFXParameters(BeamContext.FXComponent, InParametersData);
	}
}
