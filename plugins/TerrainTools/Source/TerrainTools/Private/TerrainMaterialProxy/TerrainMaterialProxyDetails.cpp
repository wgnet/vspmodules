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


#include "TerrainMaterialProxyDetails.h"

#include "Brushes/SlateImageBrush.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/Texture2DArray.h"
#include "LandscapeLayerInfoObject.h"
#include "TerrainMaterialProxyData.h"
#include "Widgets/Layout/SGridPanel.h"
#include "Widgets/Layout/SUniformGridPanel.h"

TSharedRef<IDetailCustomization> FTerrainMaterialProxyDetails::MakeInstance()
{
	return MakeShareable(new FTerrainMaterialProxyDetails);
}

void FTerrainMaterialProxyDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& DetailCategoryBuilder = DetailBuilder.EditCategory("Layers");
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

	const TSharedRef<IPropertyHandle> LandscapeComponentHandle =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ATerrainMaterialProxy, LandscapeComponent));
	UObject* PropertyValue;
	LandscapeComponentHandle->GetValue(PropertyValue);

	TSharedRef<IPropertyHandle> MaterialsInfoHandle =
		DetailBuilder.GetProperty(GET_MEMBER_NAME_CHECKED(ATerrainMaterialProxy, MaterialsInfo));
	DetailBuilder.HideProperty(MaterialsInfoHandle);
	TArray<void*> DynamicIDRawData;
	MaterialsInfoHandle->AccessRawData(DynamicIDRawData);
	ensure(DynamicIDRawData.Num() == 1);
	if (DynamicIDRawData.Num() == 1)
	{
		TerrainMaterialData = *static_cast<TMap<FName, FTerrainMaterialInfo>*>(DynamicIDRawData[0]);
	}

	DetailCategoryBuilder.AddCustomRow(FText::FromString("Options"))
		.WholeRowContent()[MakeWidget(TerrainMaterialData)->AsShared()];
}

FReply FTerrainMaterialProxyDetails::ClickedOnLeftButton(FTerrainMaterialInfo MaterialInfo)
{
	if (GEngine)
	{
		for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
		{
			ATerrainMaterialProxy* TerrainMaterialProxy = Cast<ATerrainMaterialProxy>(Object.Get());
			if (TerrainMaterialProxy)
			{
				const int32 NewIndex = FMath::Max(MaterialInfo.ParameterIndex - 1, 0);
				if (const FName* Key = TerrainMaterialProxy->MaterialsInfo.FindKey(MaterialInfo))
				{
					TerrainMaterialProxy->MaterialsInfo.Find(*Key)->ParameterIndex = NewIndex;
				}
				TerrainMaterialProxy->PostEditChange();
			}
		}

		GLog->Log(TEXT("Left Button"));
	}
	return FReply::Handled();
}

FReply FTerrainMaterialProxyDetails::ClickedOnRightButton(FTerrainMaterialInfo MaterialInfo)
{
	if (GEngine)
	{
		for (const TWeakObjectPtr<UObject>& Object : SelectedObjects)
		{
			ATerrainMaterialProxy* TerrainMaterialProxy = Cast<ATerrainMaterialProxy>(Object.Get());
			if (TerrainMaterialProxy)
			{
				const int32 NewIndex =
					FMath::Clamp(MaterialInfo.ParameterIndex + 1, 0, FMath::Max(0, MaterialInfo.MaxIndex));
				if (const FName* Key = TerrainMaterialProxy->MaterialsInfo.FindKey(MaterialInfo))
				{
					TerrainMaterialProxy->MaterialsInfo.Find(*Key)->ParameterIndex = NewIndex;
				}
				TerrainMaterialProxy->PostEditChange();
			}
		}
		GLog->Log(TEXT("Right Button"));
	}
	return FReply::Handled();
}

TSharedRef<SWidget> FTerrainMaterialProxyDetails::MakeWidget(TMap<FName, FTerrainMaterialInfo> InMaterialsInfo)
{
	auto GridPanel = SNew( SGridPanel );
	TArray<FName> Keys;
	InMaterialsInfo.GenerateKeyArray(Keys);
	int32 Column = 0;
	int32 Row = 0;

	for (int32 Iter = 0; Iter < InMaterialsInfo.Num(); Iter++)
	{
		if (Row >= 2)
		{
			Column++;
			Row = 0;
		}

		GridPanel->AddSlot(
			Row,
			Column)
		[
			SNew( SBox )
			[
				MakeIDSelectorWidget( *InMaterialsInfo.Find( Keys[ Iter ] ), Iter )
			]
		];
		Row++;
	}

	return SNew( SBorder )[ GridPanel ];
}

TSharedRef<SWidget> FTerrainMaterialProxyDetails::MakeIDSelectorWidget(FTerrainMaterialInfo& MaterialInfo, int32 ID)
{
	const FText Index =
		FText::FromString(FString::FromInt(ID) + "{" + FString::FromInt(MaterialInfo.ParameterIndex) + "}");
	const FSlateBrush* Brush = new FSlateImageBrush(MaterialInfo.Texture, FVector2D(128.f, 128.f));
	return
		SNew( SHorizontalBox )
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( SBox )
			.WidthOverride( 32 )
			.HeightOverride( 128 )
			[
				SNew( SButton )
				.OnClicked( this, &FTerrainMaterialProxyDetails::ClickedOnLeftButton, MaterialInfo )
				.Text( FText::FromString( TEXT( "<" ) ) )
				.VAlign( VAlign_Center )
				.HAlign( HAlign_Center )
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( SBox )
			.WidthOverride( 128 )
			.HeightOverride( 128 )
			[
				SNew( SBorder )
				.BorderImage( Brush )
				.VAlign( VAlign_Center )
				.HAlign( HAlign_Center )
				[
					SNew( STextBlock )
					.Text( Index )
					.Font( FSlateFontInfo( FPaths::EngineContentDir() / TEXT( "Slate/Fonts/Roboto-Bold.ttf" ), 16 ) )
				]
			]
		]
		+ SHorizontalBox::Slot()
		.AutoWidth()
		[
			SNew( SBox )
			.WidthOverride( 32 )
			.HeightOverride( 128 )
			[
				SNew( SButton )
				.OnClicked( this, &FTerrainMaterialProxyDetails::ClickedOnRightButton, MaterialInfo )
				.Text( FText::FromString( TEXT( ">" ) ) )
				.VAlign( VAlign_Center )
				.HAlign( HAlign_Center )
			]
		];
}
