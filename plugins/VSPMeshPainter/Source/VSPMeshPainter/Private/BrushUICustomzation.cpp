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
#include "BrushUICustomzation.h"
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "Engine/Texture2DArray.h"
#include "PropertyCustomizationHelpers.h"
#include "VSPCheck.h"
#include "VSPToolsProperties.h"
#include "SVSPMeshPaintPalette.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Layout/SWrapBox.h"


#define LOCTEXT_NAMESPACE "MeshPaintCustomization"

TSharedRef<ITableRow> FBrushUICustomization::GenerateMaterialTile(
	FTextureArrayPaletteItemModelPtr Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STextureArrayPaletteItemTile, OwnerTable, Item);
}

TSharedRef<ITableRow> FBrushUICustomization::GenerateAlphaTile(
	FTextureArrayAlphaItemModelPtr Item,
	const TSharedRef<STableViewBase>& OwnerTable)
{
	return SNew(STextureArrayAlphaItemTile, OwnerTable, Item);
}

TSharedRef<IDetailCustomization> FMaskBakeUICustomization::MakeInstance()
{
	return MakeShared<FMaskBakeUICustomization>();
}

void FMaskBakeUICustomization::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	TArray<TWeakObjectPtr<UObject>> CustomizationObjects;
	DetailBuilder.GetObjectsBeingCustomized(CustomizationObjects);
	if (CustomizationObjects.Num() != 1)
		return;

	auto Properties = Cast<UMaskBakeProperties>(CustomizationObjects[0].Get());
	if (!Properties)
		return;

	DetailBuilder.EditCategory(TEXT("MaskBakeProperties"), FText(), ECategoryPriority::Uncommon)
		.AddCustomRow(NSLOCTEXT("MaskBakeProperties", "MaskBakeProperties", "MaskBakeProperties"))
		.ValueContent()
		.HAlign(HAlign_Left)
			[
				SNew( SButton )
				.Text( FText::FromString( TEXT("Mask Apply") ) )
				.OnClicked_Lambda( [=]()
				{
					Properties->OnApplyBake.ExecuteIfBound();
					return FReply::Handled();
				} )
			];
}

UTexture2D* FBrushUICustomization::GetTextureFromArray(UTexture2DArray* Array, int32 Idx)
{
	return Array && Array->SourceTextures.IsValidIndex(Idx) ? Array->SourceTextures[Idx] : nullptr;
}

void FBrushUICustomization::RefreshTextureArrayPalette(UVSPBrushToolProperties* BrushProperties)
{
	TextureArrayMaterialItems.Empty();
	TextureArrayAlphaItems.Empty();
	if (BrushProperties && BrushProperties->TextureArray)
	{
		UTexture2DArray* Texture2DArray = BrushProperties->TextureArray;

		for (int32 LayerIdx = 0; LayerIdx < BrushProperties->GetMaterialLayersCount(); ++LayerIdx)
		{
			const int32 TextureIdx = BrushProperties->LayerToTextureIdx[LayerIdx];
			UTexture2D* Texture = GetTextureFromArray(Texture2DArray, TextureIdx);
			TextureArrayMaterialItems.Add(MakeShareable(new FTextureArrayPaletteItemModel(
				Texture,
				LayerIdx,
				TextureIdx,
				ThumbnailPool,
				BrushProperties,
				EMaterialLayerType::MaterialLayer)));
		}

		for (int32 LayerIdx = 0; LayerIdx < BrushProperties->GetAlphaLayersCount(); ++LayerIdx)
		{
			const int32 TextureIdx = BrushProperties->AlphaDataToTexture[LayerIdx].TextureIndex;
			UTexture2D* Texture = GetTextureFromArray(Texture2DArray, TextureIdx);
			TextureArrayAlphaItems.Add(MakeShareable(new FTextureArrayAlphaItemModel(
				Texture,
				LayerIdx,
				TextureIdx,
				ThumbnailPool,
				BrushProperties,
				EMaterialLayerType::AlphaBlendLayer,
				BrushProperties->AlphaDataToTexture[LayerIdx].TintColor,
				BrushProperties->AlphaDataToTexture[LayerIdx].TextureChannel,
				BrushProperties->AlphaDataToTexture[LayerIdx].Roughness)));
		}
	}

	AtlasTilesViewMaterials->RequestListRefresh();
	AtlasTilesViewAlphaLayers->RequestListRefresh();

	if (BrushProperties->GetSelectedLayerType() == EMaterialLayerType::MaterialLayer)
	{
		if (TextureArrayMaterialItems.Num() > 0)
		{
			AtlasTilesViewMaterials->SetSelection(
				TextureArrayMaterialItems[BrushProperties->GetSelectedTileIndex()],
				ESelectInfo::Direct);
			AtlasTilesViewAlphaLayers->SetSelection(
				TSharedPtr<FTextureArrayAlphaItemModel, ESPMode::Fast>(),
				ESelectInfo::Direct);
		}
	}
	else
	{
		if (TextureArrayAlphaItems.Num() > 0)
		{
			AtlasTilesViewMaterials->SetSelection(
				TSharedPtr<FTextureArrayPaletteItemModel, ESPMode::Fast>(),
				ESelectInfo::Direct);
			AtlasTilesViewAlphaLayers->SetSelection(
				TextureArrayAlphaItems[BrushProperties->GetSelectedTileIndex()],
				ESelectInfo::Direct);
		}
	}
}

FTextureArrayPaletteItemModel::FTextureArrayPaletteItemModel(
	UTexture2D* InTexture2D,
	int32 InLayerIdx,
	int32 InTextureIdx,
	TSharedPtr<class FAssetThumbnailPool> InThumbnailPool,
	UVSPBrushToolProperties* InBrushProperties,
	EMaterialLayerType InLayerType)
	: ThumbnailPool(InThumbnailPool)
	, Texture2D(InTexture2D)
	, LayerIdx(InLayerIdx)
	, TextureIdx(InTextureIdx)
	, LayerType(InLayerType)
	, BrushProperties(InBrushProperties)
{
}

FTextureArrayAlphaItemModel::FTextureArrayAlphaItemModel(
	UTexture2D* InTexture2D,
	int32 InLayerIdx,
	int32 InTextureIdx,
	TSharedPtr<class FAssetThumbnailPool> InThumbnailPool,
	UVSPBrushToolProperties* InBrushProperties,
	EMaterialLayerType InLayerType,
	FLinearColor InTintColor,
	FLinearColor InTextureChannel,
	float InRoughness)
	: FTextureArrayPaletteItemModel(
		InTexture2D,
		InLayerIdx,
		InTextureIdx,
		InThumbnailPool,
		InBrushProperties,
		InLayerType)
	, TintColor(InTintColor)
	, TextureChannel(InTextureChannel)
	, Roughness(InRoughness)
{
}


FLinearColor FTextureArrayAlphaItemModel::GetTintColor() const
{
	return TintColor;
}

FLinearColor FTextureArrayAlphaItemModel::GetTextureChannel() const
{
	return TextureChannel;
}

float FTextureArrayAlphaItemModel::GetRoughness() const
{
	return Roughness;
}

void FTextureArrayAlphaItemModel::ShiftTexture(int32 Offset)
{
	if (BrushProperties)
	{
		TextureIdx = BrushProperties->SetTextureIdxForLayer(LayerIdx, TextureIdx + Offset, LayerType);
		Texture2D = BrushProperties->TextureArray->SourceTextures[TextureIdx];

		OnUpdate.ExecuteIfBound();
	}
}

void FTextureArrayAlphaItemModel::ShiftTint(int32 Offset)
{
	if (BrushProperties)
	{
		TintColor = BrushProperties->ShiftLayerTintColor(GetLayerIdx(), Offset);
	}
}

void FTextureArrayAlphaItemModel::SetTextureChannel(FLinearColor InChannelColor)
{
	if (BrushProperties)
	{
		TextureChannel = BrushProperties->SetLayerTextureChannel(GetLayerIdx(), InChannelColor);
	}
}

void FTextureArrayAlphaItemModel::UpdateRoughness(float InRoughness)
{
	if (BrushProperties)
	{
		Roughness = BrushProperties->SetLayerRoughness(GetLayerIdx(), InRoughness);
	}
}

TSharedRef<SWidget> FTextureArrayPaletteItemModel::GetThumbnailWidget() const
{
	FAssetData AssetData = FAssetData(Texture2D);

	int32 MaxThumbnailSize = 64;
	TSharedPtr<FAssetThumbnail> Thumbnail =
		MakeShareable(new FAssetThumbnail(AssetData, MaxThumbnailSize, MaxThumbnailSize, ThumbnailPool));
	FAssetThumbnailConfig ThumbnailConfig;
	ThumbnailConfig.ThumbnailLabel = EThumbnailLabel::AssetName;
	ThumbnailConfig.AssetTypeColorOverride = FLinearColor(0, 0.7, 0, 1);

	TSharedRef<SWidget> Widget = Thumbnail->MakeThumbnailWidget(ThumbnailConfig);
	return Widget;
}

int32 FTextureArrayPaletteItemModel::GetLayerIdx() const
{
	return LayerIdx;
}

EMaterialLayerType FTextureArrayPaletteItemModel::GetLayerType() const
{
	return LayerType;
}

int32 FTextureArrayPaletteItemModel::GetTextureIdx() const
{
	return TextureIdx;
}

bool FTextureArrayPaletteItemModel::IsSelected() const
{
	return BrushProperties->GetSelectedTileIndex() == LayerIdx;
}

UTexture2D* FTextureArrayPaletteItemModel::GetTexture() const
{
	return Texture2D;
}

void FTextureArrayPaletteItemModel::SetTexture(UTexture2D* Texture)
{
	Texture2D = Texture;
}

void FTextureArrayPaletteItemModel::ShiftTextureLeft()
{
	if (BrushProperties)
	{
		TextureIdx = BrushProperties->SetTextureIdxForLayer(LayerIdx, TextureIdx - 1, LayerType);
		Texture2D = BrushProperties->TextureArray->SourceTextures[TextureIdx];

		OnUpdate.ExecuteIfBound();
	}
}

void FTextureArrayPaletteItemModel::ShiftTextureRight()
{
	if (BrushProperties)
	{
		TextureIdx = BrushProperties->SetTextureIdxForLayer(LayerIdx, TextureIdx + 1, LayerType);
		Texture2D = BrushProperties->TextureArray->SourceTextures[TextureIdx];

		OnUpdate.ExecuteIfBound();
	}
}

void FTextureArrayPaletteItemModel::SetTexture(int Index)
{
	if (!BrushProperties)
		return;

	TextureIdx = BrushProperties->SetTextureIdxForLayer(LayerIdx, Index, LayerType);
	Texture2D = BrushProperties->TextureArray->SourceTextures[TextureIdx];

	OnUpdate.ExecuteIfBound();
}

void FTextureArrayPaletteItemModel::ShowTextureArrayPalette()
{
	if (!BrushProperties)
		return;

	FBrushUICustomization::SetupPaletteWindow(BrushProperties, TSharedPtr<FTextureArrayPaletteItemModel>(this));
}

TSharedPtr<FAssetThumbnailPool> FTextureArrayPaletteItemModel::GetThumbnailPool()
{
	return ThumbnailPool;
}

void STextureArrayTextureSelector::Construct(const FArguments& InArgs)
{
	OnShowTextureArrayPalette = InArgs._OnShowTextureArrayPalette;
	ThumbnailWidget = InArgs._ThumbnailWidget;
	Layer = InArgs._Layer;
	bool bBShowPaletteButton = InArgs._bShowPaletteButton.Get();

	ChildSlot[
		SNew(SOverlay)
			+ SOverlay::Slot()
			.Padding(4)
			[
				SNew(SHorizontalBox)
				+ SHorizontalBox::Slot()
				.FillWidth(0.68f)
				[
					SNew(SBorder)
					.BorderImage(FEditorStyle::GetBrush("ContentBrowser.ThumbnailShadow"))
					.ForegroundColor(FLinearColor::White)
					[
						ThumbnailWidget.Get().ToSharedRef()
					]
				]				
			]
			+ SOverlay::Slot()
			.HAlign(HAlign_Left)
			.VAlign(VAlign_Bottom)
			.Padding(FMargin(25.f, 10.f))
			[
				SNew(STextBlock)
				.Text(FText::AsNumber(Layer.Get()))
				.ShadowOffset(FVector2D(1.f, 1.f))
				.ColorAndOpacity(FLinearColor(.85f, .85f, .85f, 1.f))
			]
			+ SOverlay::Slot()
            .HAlign(HAlign_Right)
            .VAlign(VAlign_Bottom)            
            .Padding(FMargin(5.0f, 10.f))            
            [
             	SNew(SButton)
				.HAlign( HAlign_Right )
				.VAlign( VAlign_Bottom )
				.ContentPadding( FMargin(FVector2D(10,10)) )
				.ButtonColorAndOpacity( FSlateColor(FLinearColor(0.5,1,0.5, 0.25f)) )
				.OnClicked(FOnClicked::CreateLambda([this]() -> FReply
				{
					OnShowTextureArrayPalette.ExecuteIfBound();
					return FReply::Handled();
				}))
				.Visibility(  bBShowPaletteButton ? EVisibility::Visible : EVisibility::Hidden )
            ]
			
	];
}

void STextureArrayPaletteItemTile::Construct(
	const FArguments& InArgs,
	TSharedRef<STableViewBase> InOwnerTableView,
	TSharedPtr<FTextureArrayPaletteItemModel>& InModel,
	bool ShowPaletteButton)
{
	this->bShowPaletteButton = ShowPaletteButton;
	ConstructInternal({}, InOwnerTableView);

	Model = InModel;

	Model->OnUpdate.BindSP(this, &STextureArrayPaletteItemTile::Rebuild);
	Rebuild();
}

void STextureArrayPaletteItemTile::Rebuild()
{
	ChildSlot.DetachWidget();
	ChildSlot [
			SNew(STextureArrayTextureSelector)
			.OnShowTextureArrayPalette_Lambda( [this]{	Model->ShowTextureArrayPalette();} )
			.bShowPaletteButton( bShowPaletteButton )
			.ThumbnailWidget_Lambda([this]() { return Model->GetThumbnailWidget(); })
			.Layer( Model->GetLayerIdx() )
		];
}

void STextureArrayAlphaItemTile::Construct(
	const FArguments& InArgs,
	TSharedRef<STableViewBase> InOwnerTableView,
	TSharedPtr<FTextureArrayAlphaItemModel>& InModel)
{
	ConstructInternal({}, InOwnerTableView);

	Model = InModel;

	Model->OnUpdate.BindSP(this, &STextureArrayAlphaItemTile::Rebuild);
	Rebuild();
}

void STextureArrayAlphaItemTile::Rebuild()
{
	ChildSlot.DetachWidget();
	ChildSlot[
		SNew(SVerticalBox)
		+SVerticalBox::Slot()
		.MaxHeight( 74.f )
		.FillHeight( 74.f )
		[
			SNew(STextureArrayTextureSelector)
			.bShowPaletteButton( true )
			.OnShowTextureArrayPalette_Lambda( [this]
			{
				Model->ShowTextureArrayPalette();
			} )			
			.ThumbnailWidget_Lambda([this]() { return Model->GetThumbnailWidget(); })
			.Layer(Model->GetLayerIdx())
		]		
		+ SVerticalBox::Slot()
		.MaxHeight(30.f)
		.FillHeight(30.f)
		.Padding(4.0f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			.FillWidth(0.16f)
			.HAlign(HAlign_Left)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT("<")))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.OnClicked(FOnClicked::CreateLambda([this]() -> FReply
				{
					Model->ShiftTint(-1);
					return FReply::Handled();
				}))
			]
			+ SHorizontalBox::Slot()
			.FillWidth(0.68f)
			[
				SNew(SOverlay)
				+ SOverlay::Slot()
				[
					SNew(SImage)
					.Image(FEditorStyle::GetBrush("WhiteBrush"))
					.ColorAndOpacity_Lambda( [this]() ->FSlateColor
					{
						return FSlateColor(Model->GetTintColor());
					})
				]
				+ SOverlay::Slot()
				.VAlign(VAlign_Center)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("Tint Color")))
					.Justification(ETextJustify::Center)
					.ShadowOffset(FVector2D(1.f, 1.f))
					.ColorAndOpacity(FLinearColor(.85f, .85f, .85f, 1.f))
				]
			]
			+SHorizontalBox::Slot()
			.FillWidth(0.16f)
			[
				SNew(SButton)
				.Text(FText::FromString(TEXT(">")))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.OnClicked(FOnClicked::CreateLambda([this]() -> FReply
				{
					Model->ShiftTint(1);
					return FReply::Handled();
				}))
			]
		]
		+SVerticalBox::Slot()
		.MaxHeight(20.f)
		.FillHeight(20.f)
		.Padding(4.0f, 0.f)
		[
			SNew(SHorizontalBox)
			+ SHorizontalBox::Slot()
			[
				SNew(SButton)
				.ButtonColorAndOpacity_Lambda( [this]()->FSlateColor
				{
					return FSlateColor(FMath::IsNearlyEqual(Model->GetTextureChannel().R, 1) ? FLinearColor(1.0f, .0f, .0f, 1.f) : FLinearColor(.45f, .27f, 0.27f, 1.f));
				})
				.Text(FText::FromString(TEXT("R")))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.ForegroundColor(FSlateColor(FLinearColor(.85f, .85f, .85f, 1.f)))
				.DesiredSizeScale(FVector2D(18.f, 18.f))
				.OnClicked(FOnClicked::CreateLambda([this]() -> FReply
				{
					Model->SetTextureChannel(FLinearColor(1.0f, .0f, .0f, .0f));
					return FReply::Handled();
				}))
			]
			+SHorizontalBox::Slot()
			[
				SNew(SButton)
				.ButtonColorAndOpacity_Lambda([this]()->FSlateColor
				{
					return FSlateColor(FMath::IsNearlyEqual(Model->GetTextureChannel().G, 1) ? FLinearColor(.0f, 1.0f, .0f, 1.f) : FLinearColor(.27f, .45f, 0.27f, 1.f));
				})
				.Text(FText::FromString(TEXT("G")))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.ForegroundColor(FSlateColor(FLinearColor(.85f, .85f, .85f, 1.f)))
				.DesiredSizeScale(FVector2D(18.f, 18.f))
				.OnClicked(FOnClicked::CreateLambda([this]() -> FReply
				{
					Model->SetTextureChannel(FLinearColor(.0f, 1.0f, .0f, .0f));
					return FReply::Handled();
				}))
			]
			+SHorizontalBox::Slot()
			[
				SNew(SButton)
				.ButtonColorAndOpacity_Lambda([this]()->FSlateColor
				{
					return FSlateColor(FMath::IsNearlyEqual(Model->GetTextureChannel().B, 1) ? FLinearColor(.0f, .0f, 1.0f, 1.f) : FLinearColor(.27f, .27f, 0.45f, 1.f));
				})
				.Text(FText::FromString(TEXT("B")))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.ForegroundColor(FSlateColor(FLinearColor(.85f, .85f, .85f, 1.f)))
				.DesiredSizeScale(FVector2D(18.f, 18.f))
				.OnClicked(FOnClicked::CreateLambda([this]() -> FReply
				{
					Model->SetTextureChannel(FLinearColor(0.0f, .0f, 1.0f, .0f));
					return FReply::Handled();
				}))
			]
			+SHorizontalBox::Slot()
			[
				SNew(SButton)
				.ButtonColorAndOpacity_Lambda([this]()->FSlateColor
				{
					return FSlateColor(FMath::IsNearlyEqual(Model->GetTextureChannel().A, 1) ? FLinearColor(1.0f, 1.0f, 1.0f, 1.f) : FLinearColor(.3f, .3f, 0.3f, 1.0f));
				})
				.Text(FText::FromString(TEXT("A")))
				.VAlign(VAlign_Center)
				.HAlign(HAlign_Center)
				.ForegroundColor(FSlateColor(FMath::IsNearlyEqual(Model->GetTextureChannel().B, 1) ? FLinearColor(.05f, .05f, .05f, 1.f) : FLinearColor(.05f, .05f, 0.05f, 1.f)))
				.DesiredSizeScale(FVector2D(18.f, 18.f))
				.OnClicked(FOnClicked::CreateLambda([this]() -> FReply
				{
					Model->SetTextureChannel(FLinearColor(.0f, .0f, .0f, 1.0f));
					return FReply::Handled();
				}))
			]
		]
		+SVerticalBox::Slot()
			.MaxHeight(21.f)
			.FillHeight(21.f)
			.Padding(4.0f, 0.f)
			[
				SNew(SHorizontalBox)
				+SHorizontalBox::Slot()
				.FillWidth(0.5f)
				[
					SNew(STextBlock)
					.Text(FText::FromString(TEXT("ROUGH")))
					.Justification(ETextJustify::Center)
					.ShadowOffset(FVector2D(1.f, 1.f))
					.ColorAndOpacity(FLinearColor(.85f, .85f, .85f, 1.f))
				]
				+SHorizontalBox::Slot()
				.FillWidth(0.5f)
				[
					SNew(SSpinBox<float>)
					.MaxValue(1.f)
					.MinValue(-1.f)
					.MaxSliderValue(1.f)
					.MinSliderValue(-1.f)
					.Value_Lambda([this]() {return Model->GetRoughness(); })
					.OnValueChanged_Lambda([this](float InRoughness) {Model->UpdateRoughness(InRoughness); })
				]
			]
		];
}

TSharedRef<IDetailCustomization> FBrushUICustomization::MakeInstance()
{
	return MakeShared<FBrushUICustomization>();
}

void FBrushUICustomization::CustomizeDetails(IDetailLayoutBuilder& DetailLayout)
{
	TArray<TWeakObjectPtr<UObject>> CustomizationObjects;
	DetailLayout.GetObjectsBeingCustomized(CustomizationObjects);
	if (CustomizationObjects.Num() != 1)
		return;

	auto BrushProperties = Cast<UVSPBrushToolProperties>(CustomizationObjects[0].Get());
	if (!BrushProperties)
		return;

	ThumbnailPool = DetailLayout.GetThumbnailPool();

	IDetailCategoryBuilder& TextureCategory =
		DetailLayout.EditCategory(TEXT("TexturePainting"), FText(), ECategoryPriority::Uncommon);

	AddMaterialsRow(TextureCategory, BrushProperties);
	AddAlphaBlendsRow(TextureCategory, BrushProperties);

	BrushProperties->OnLayerCountChanged.AddLambda(
		[this, BrushProperties](int32 MaterialLayerCount, int32 AlphaLayerCount)
		{
			RefreshTextureArrayPalette(BrushProperties);
		});

	RefreshTextureArrayPalette(BrushProperties);
}

TSharedPtr<SVSPMeshPaintPalette> MeshPaintPalette;

void FBrushUICustomization::SetupPaletteWindow(
	UVSPBrushToolProperties* BrushProperties,
	FTextureArrayPaletteItemModelPtr PaletteItem)
{
	if (!BrushProperties)
		return;

	if (MeshPaintPalette != nullptr)
		MeshPaintPalette->CloseWindow();

	auto ThumbnailPool = MakeShared<FAssetThumbnailPool>(128, true); // todo  is there a better way?

	UTexture2DArray* Texture2DArray = BrushProperties->TextureArray;
	TArray<FTextureArrayPaletteItemModelPtr> TextureArrayMaterialItems_Ptr;

	for (int32 Index = 0; Index < Texture2DArray->SourceTextures.Num(); ++Index)
	{
		UTexture2D* Texture = FBrushUICustomization::GetTextureFromArray(Texture2DArray, Index);
		auto Item = MakeShared<FTextureArrayPaletteItemModel>(
			Texture,
			Index,
			Index,
			ThumbnailPool,
			BrushProperties,
			EMaterialLayerType::MaterialLayer);
		TextureArrayMaterialItems_Ptr.Add(Item);
	}

	TSharedRef<SWindow> Window = SNew(SWindow)
	.Title(LOCTEXT("WindowTitle", "Texture array palette"))
	.SizingRule(ESizingRule::Autosized)
	.MinWidth( 1200 )
	.MinHeight( 1200 )
	.AutoCenter( EAutoCenter::None )
	.ScreenPosition( FVector2D(0,0) )
	.IsTopmostWindow( true );

	Window->SetContent(
		SAssignNew(MeshPaintPalette, SVSPMeshPaintPalette)
		.WidgetWindow(Window)
		.TextureArrayMaterialItems( TextureArrayMaterialItems_Ptr )
		.BrushProperties( BrushProperties )
		.PaletteItem( PaletteItem )
	);

	//close palette on painter close
	BrushProperties->ShutdownLambdaHandleForPalette = BrushProperties->OnShutdownDelegate.AddLambda(
		[]()
		{
			if (MeshPaintPalette != nullptr)
				MeshPaintPalette->CloseWindow();
		});

	FSlateApplication::Get().AddWindow(Window, true);
}


void FBrushUICustomization::AddMaterialsRow(
	IDetailCategoryBuilder& DetailCategory,
	UVSPBrushToolProperties* BrushProperties)
{
	VSPCheck(DetailCategory.IsParentLayoutValid() && BrushProperties);

	DetailCategory.AddCustomRow(NSLOCTEXT("TexturePaintSetting", "TextureSearchString", "Materials"))
		.NameContent()
	[
		SNew(STextBlock)
		.Text(NSLOCTEXT("TexturePaintSettings", "MaterialPaintLabel", "Materials"))
		.ToolTipText(NSLOCTEXT("TexturePaintSettings", "MaterialPaintToolTip", "Array materials to paint with."))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.HAlign(HAlign_Fill)
	[
		SAssignNew(AtlasTilesViewMaterials, STextureArrayMaterialTypeTileView)
		.ListItemsSource(&TextureArrayMaterialItems)
		.SelectionMode(ESelectionMode::Single)
		.Visibility_Lambda([BrushProperties]()
		{
			return BrushProperties
				&& BrushProperties->TextureArray
				&& BrushProperties->TextureArray->SourceTextures.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.OnGenerateTile(this, &FBrushUICustomization::GenerateMaterialTile)
		.OnSelectionChanged_Lambda([BrushProperties, this](FTextureArrayPaletteItemModelPtr Item, ESelectInfo::Type SelectInfo)
		{
			if ( SelectInfo != ESelectInfo::Direct && BrushProperties && Item )
			{
				AtlasTilesViewAlphaLayers.Get()->ClearSelection();
				BrushProperties->SelectTile(Item->GetLayerIdx(), Item->GetLayerType());
			}
		})
		.AllowOverscroll(EAllowOverscroll::No)
		.ItemHeight(100)
		.ItemWidth(100)
		.ItemAlignment(EListItemAlignment::LeftAligned)
	];
}

void FBrushUICustomization::AddAlphaBlendsRow(
	IDetailCategoryBuilder& DetailCategory,
	UVSPBrushToolProperties* BrushProperties)
{
	VSPCheck(DetailCategory.IsParentLayoutValid() && BrushProperties);

	DetailCategory.AddCustomRow(NSLOCTEXT("TexturePaintSetting", "TextureSearchString", "Alpha blend layers"))
		.NameContent()
	[
		SNew(STextBlock)
		.Text(NSLOCTEXT("TexturePaintSettings", "AlphaPaintLabel", "Alpha blend layers"))
		.ToolTipText(NSLOCTEXT("TexturePaintSettings", "AlphaPaintToolTip", "Array materials to use in alpha blend mode."))
		.Font(IDetailLayoutBuilder::GetDetailFont())
	]
	.ValueContent()
	.HAlign(HAlign_Fill)
	[
		SAssignNew(AtlasTilesViewAlphaLayers, STextureArrayAlphaTypeTileView)
		.ListItemsSource(&TextureArrayAlphaItems)
		.SelectionMode(ESelectionMode::Single)
		.Visibility_Lambda([BrushProperties]()
		{
			return BrushProperties
				&& BrushProperties->TextureArray
				&& BrushProperties->TextureArray->SourceTextures.Num() > 0 ? EVisibility::Visible : EVisibility::Collapsed;
		})
		.OnGenerateTile(this, &FBrushUICustomization::GenerateAlphaTile)
		.OnSelectionChanged_Lambda([BrushProperties, this](FTextureArrayAlphaItemModelPtr Item, ESelectInfo::Type SelectInfo)
		{
			if ( SelectInfo != ESelectInfo::Direct && BrushProperties && Item )
			{
				AtlasTilesViewMaterials.Get()->ClearSelection();
				BrushProperties->SelectTile(Item->GetLayerIdx(), Item->GetLayerType());
			}
		})
		.AllowOverscroll(EAllowOverscroll::No)
		.ItemHeight(169)
		.ItemWidth(106)
		.ItemAlignment(EListItemAlignment::LeftAligned)
	];
}

#undef LOCTEXT_NAMESPACE
