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
#include "AssetRegisterData.h"

UAssetRegisterMark* UAssetRegisterMark::GetAssetRegisterMark(UObject* InObject)
{
	UAssetRegisterMark* AssetRegisterMarkResult {};

	if (IInterface_AssetUserData* const AssetUserDataInterface = Cast<IInterface_AssetUserData>(InObject))
	{
		if (UAssetRegisterMark* const AssetRegisterMark = Cast<UAssetRegisterMark>(
				AssetUserDataInterface->GetAssetUserDataOfClass(UAssetRegisterMark::StaticClass())))
		{
			AssetRegisterMarkResult = AssetRegisterMark;
		}
	}

	return AssetRegisterMarkResult;
}

FName UAssetRegisterMark::GetAssetRegisterMarkTag(UObject* InObject)
{
	FName NameResult = NAME_None;

	if (IInterface_AssetUserData* const AssetUserDataInterface = Cast<IInterface_AssetUserData>(InObject))
	{
		if (const UAssetRegisterMark* AssetRegisterMark = Cast<UAssetRegisterMark>(
				AssetUserDataInterface->GetAssetUserDataOfClass(UAssetRegisterMark::StaticClass())))
		{
			NameResult = AssetRegisterMark->PrimaryTag.GetTagName();
		}
	}

	return NameResult;
}

UObject* UAssetRegisterMark::GetOwner() const
{
	return GetOuter();
}

EAssetType UAssetRegisterMark::GetType() const
{
	EAssetType OutType = EAssetType::Undefined;
	UObject* Owner = GetOwner();

	if (Owner->IsA<UStaticMesh>())
	{
		OutType = EAssetType::StaticMesh;
	}
	else if (Owner->IsA<UTexture>())
	{
		OutType = EAssetType::Texture;
	}
	else if (Owner->IsA<UMaterial>())
	{
		OutType = EAssetType::Material;
	}
	else if (Owner->IsA<UMaterialInstance>())
	{
		OutType = EAssetType::MaterialInstance;
	}

	return OutType;
}
