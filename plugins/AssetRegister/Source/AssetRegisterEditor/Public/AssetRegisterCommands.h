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
#include "AssetRegisterStyle.h"
#include "Framework/Commands/Commands.h"

class ASSETREGISTEREDITOR_API FAssetRegisterCommands : public TCommands<FAssetRegisterCommands>
{
public:
	FAssetRegisterCommands()
		: TCommands<FAssetRegisterCommands>(
			TEXT("AssetRegister"),
			NSLOCTEXT("Contexts", "AssetRegister", "Asset Register Plugin"),
			NAME_None,
			FAssetRegisterStyle::GetStyleSetName())
	{
	}

	virtual void RegisterCommands() override;

	TSharedPtr<FUICommandInfo> OpenPluginWindow;
	TSharedPtr<FUICommandInfo> RemoveAssetsFromList;
	TSharedPtr<FUICommandInfo> ViewReferences;
	TSharedPtr<FUICommandInfo> ViewSizeMap;
};
