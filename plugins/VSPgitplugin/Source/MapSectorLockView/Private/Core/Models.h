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

enum class EMapLockState
{
	NotLocked,
	LockedByMe,
	LockedOther
};

struct FMap
{
	FName Name { NAME_None };
	FString ContentPath;
	FString DefaultImage;
	FVector2D DefaultImageSize { FVector2D::ZeroVector };
};

struct FMapChildren
{
	int32 RelationshipId { 0 };
	FString ParentMapContentPath;
	FString ChildMapContentPath;
};

struct FLayer
{
	FGuid LayerGuid { };
	FName Name { NAME_None };
	FString ImageBrushName;
	FVector2D Size { FVector2D::ZeroVector };
	FVector2D TopLeft { FVector2D::ZeroVector };
	FVector2D BottomRight { FVector2D::ZeroVector };
	FString ParentMapContentPath;
};

struct FSector
{
	FGuid SectorGuid { };
	FName Name { NAME_None };
	FGuid LayerGuid { };
	FString LinkedMapContentPath;
	FColor DefaultColor { FColor::White.WithAlpha( 50 ) };
	FColor HoveredColor { FColor::White.WithAlpha( 127 ) };
	EMapLockState LockState { EMapLockState::NotLocked };
	FString LockUserName;
};

struct FSectorPoint
{
	FGuid PointGuid { };
	FVector2D Point { FVector2D::ZeroVector };
	FGuid SectorGuid { };
};
