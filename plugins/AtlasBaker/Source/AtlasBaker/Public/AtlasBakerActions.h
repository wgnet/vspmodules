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

#include "AssetTypeActions_Base.h"
#include "SlateAtlasTile.h"
#include "TextureAtlasData.h"

class FAtlasBakerActions : public FAssetTypeActions_Base
{
public:
	FAtlasBakerActions(EAssetTypeCategories::Type InAssetCategory);

	virtual FColor GetTypeColor() const override;
	virtual void OpenAssetEditor(
		const TArray<UObject*>& InObjects,
		TSharedPtr<class IToolkitHost> EditWithinLevelEditor = TSharedPtr<IToolkitHost>()) override;

	// IAssetTypeActions Implementation
	virtual FText GetName() const override
	{
		return FText::FromString(TEXT("TextureAtlasData"));
	}
	virtual UClass* GetSupportedClass() const override
	{
		return UTextureAtlasData::StaticClass();
	}
	virtual uint32 GetCategories() override
	{
		return AssetCategory;
	}

private:
	EAssetTypeCategories::Type AssetCategory;
};

class FSlateAtlasTileActions : public FAssetTypeActions_Base
{
public:
	virtual FText GetName() const override
	{
		return FText::FromString(TEXT("Slate Atlas Tile"));
	}
	virtual UClass* GetSupportedClass() const override
	{
		return USlateAtlasTile::StaticClass();
	}
	virtual uint32 GetCategories() override
	{
		return EAssetTypeCategories::UI;
	}
	virtual FColor GetTypeColor() const override
	{
		return FColor::Purple;
	}
};
