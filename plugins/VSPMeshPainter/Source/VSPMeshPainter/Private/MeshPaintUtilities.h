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
#include "MeshPaintUtilities.generated.h"

struct FToolBuilderState;

UCLASS()
class UMeshPaintUtilities : public UObject
{
	GENERATED_BODY()

public:
	static void CopyTextureToRT(UTexture* Texture, UTextureRenderTarget2D* RT);
	static void WriteRTDataToTexture(UTextureRenderTarget2D* RT, UTexture2D* Texture);
	static UTextureRenderTarget2D* CreateInternalRenderTarget(
		const FIntPoint& Size,
		TextureAddress TextureAddressX,
		TextureAddress TextureAddressY);
	static void ClearRT(UTextureRenderTarget2D* RT, const FLinearColor& ClearColor = FLinearColor::Transparent);
	static UStaticMeshComponent* GetSingleSelectedStaticMeshComp(const FToolBuilderState& SceneState);
};
