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
#include "IDetailCustomization.h"
#include "Input/Reply.h"
#include "TerrainMaterialProxy.h"

struct FTerrainMaterialInfo;

class FTerrainMaterialProxyDetails : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();

	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	TSharedPtr<FSlateBrush> PreviewBrush;
	TArray<TSharedPtr<FSlateBrush> > PreviewBrushes;
	TSharedPtr<SWidget> TexturesWidget;

	FReply ClickedOnLeftButton(FTerrainMaterialInfo MaterialInfo);
	FReply ClickedOnRightButton(FTerrainMaterialInfo MaterialInfo);

private:
	TArray<TWeakObjectPtr<UObject> > SelectedObjects;
	TSharedRef<SWidget> MakeWidget(TMap<FName, FTerrainMaterialInfo> InMaterialsInfo);
	TSharedRef<SWidget> MakeIDSelectorWidget(FTerrainMaterialInfo& MaterialInfo, int32 ID);
	TMap<FName, FTerrainMaterialInfo> TerrainMaterialData;
};
