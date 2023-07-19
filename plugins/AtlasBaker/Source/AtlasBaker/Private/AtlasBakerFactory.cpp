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

#include "AtlasBakerFactory.h"
#include "SlateAtlasTile.h"
#include "TextureAtlasData.h"
#if WITH_EDITOR
	#include "AssetTypeCategories.h"
#endif
UAtlasBakerFactory::UAtlasBakerFactory() : Super()
{
	bCreateNew = true;
	bEditAfterNew = true;
	SupportedClass = UTextureAtlasData::StaticClass();
}
#if WITH_EDITOR
uint32 UAtlasBakerFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::MaterialsAndTextures;
}

UObject* UAtlasBakerFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn,
	FName CallingContext)
{
	UTextureAtlasData* TextureAtlasData = NewObject<UTextureAtlasData>(InParent, InClass, InName, Flags);
	return TextureAtlasData;
}

USlateAtlasTileFactory::USlateAtlasTileFactory()
{
	SupportedClass = USlateAtlasTile::StaticClass();
	bCreateNew = true;
}

uint32 USlateAtlasTileFactory::GetMenuCategories() const
{
	return EAssetTypeCategories::UI;
}

UObject* USlateAtlasTileFactory::FactoryCreateNew(
	UClass* InClass,
	UObject* InParent,
	FName InName,
	EObjectFlags Flags,
	UObject* Context,
	FFeedbackContext* Warn,
	FName CallingContext)
{
	return NewObject<USlateAtlasTile>(InParent, InClass, InName, Flags);
}
#endif
