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

#include "IDetailCustomization.h"
#include "VSPToolsProperties.h"
#include "Slate/Public/Widgets/Views/STileView.h"

class IDetailCategoryBuilder;

class FTextureArrayPaletteItemModel : public TSharedFromThis<FTextureArrayPaletteItemModel>
{
public:
	virtual ~FTextureArrayPaletteItemModel() = default;

	FSimpleDelegate OnUpdate;

	FTextureArrayPaletteItemModel(
		UTexture2D* InTexture2D,
		int32 InLayerIdx,
		int32 InTextureIdx,
		TSharedPtr<class FAssetThumbnailPool> InThumbnailPool,
		UVSPBrushToolProperties* InBrushProperties,
		EMaterialLayerType InLayerType);

	virtual TSharedRef<SWidget> GetThumbnailWidget() const;
	virtual int32 GetLayerIdx() const;
	virtual EMaterialLayerType GetLayerType() const;
	virtual int32 GetTextureIdx() const;
	virtual bool IsSelected() const;
	virtual UTexture2D* GetTexture() const;
	virtual void SetTexture(UTexture2D* Texture);
	virtual void ShiftTextureLeft();
	virtual void ShiftTextureRight();
	virtual void SetTexture(int Index);
	virtual void ShowTextureArrayPalette();
	virtual TSharedPtr<class FAssetThumbnailPool> GetThumbnailPool();

protected:
	TSharedPtr<SWidget> ThumbnailWidget;
	TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;
	UTexture2D* Texture2D;
	int32 LayerIdx;
	int32 TextureIdx;
	EMaterialLayerType LayerType;
	UVSPBrushToolProperties* BrushProperties;
};

class FTextureArrayAlphaItemModel : public FTextureArrayPaletteItemModel
{
public:
	FTextureArrayAlphaItemModel(
		UTexture2D* InTexture2D,
		int32 InLayerIdx,
		int32 InTextureIdx,
		TSharedPtr<class FAssetThumbnailPool> InThumbnailPool,
		UVSPBrushToolProperties* InBrushProperties,
		EMaterialLayerType,
		FLinearColor InTintColor,
		FLinearColor InTextureChannel,
		float InRoughness);

	FLinearColor GetTintColor() const;
	FLinearColor GetTextureChannel() const;
	float GetRoughness() const;

	void ShiftTexture(int32 Offset);
	void ShiftTint(int32 Offset);
	void SetTextureChannel(FLinearColor InChannelColor);
	void UpdateRoughness(float InRoughness);

private:
	FLinearColor TintColor;
	FLinearColor TextureChannel;
	float Roughness;
};

class STextureArrayTextureSelector : public SCompoundWidget
{
	SLATE_BEGIN_ARGS(STextureArrayTextureSelector)
	{
	}
	SLATE_ATTRIBUTE(TSharedPtr<SWidget>, ThumbnailWidget)
	SLATE_ATTRIBUTE(int32, Layer)
	SLATE_ATTRIBUTE(bool, bShowPaletteButton)
	SLATE_EVENT(FSimpleDelegate, OnShowTextureArrayPalette)
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	void Rebuild();

private:
	FSimpleDelegate OnShowTextureArrayPalette;
	TAttribute<int32> Layer;
	TAttribute<TSharedPtr<SWidget>> ThumbnailWidget;
};

typedef TSharedPtr<FTextureArrayPaletteItemModel> FTextureArrayPaletteItemModelPtr;
typedef STileView<FTextureArrayPaletteItemModelPtr> STextureArrayMaterialTypeTileView;

typedef TSharedPtr<FTextureArrayAlphaItemModel> FTextureArrayAlphaItemModelPtr;
typedef STileView<FTextureArrayAlphaItemModelPtr> STextureArrayAlphaTypeTileView;

class STextureArrayPaletteItemTile : public STableRow<TSharedPtr<FTextureArrayPaletteItemModel>>
{
public:
	SLATE_BEGIN_ARGS(STextureArrayPaletteItemTile)
	{
	}
	SLATE_END_ARGS()

	virtual void Construct(
		const FArguments& InArgs,
		TSharedRef<STableViewBase> InOwnerTableView,
		TSharedPtr<FTextureArrayPaletteItemModel>& InModel,
		bool bShowPaletteButton = true);
	virtual void Rebuild();

private:
	TSharedPtr<FTextureArrayPaletteItemModel> Model;
	TSharedPtr<STextureArrayTextureSelector> SelectorWidget;
	bool bShowPaletteButton = false;
};

class STextureArrayAlphaItemTile : public STableRow<TSharedPtr<FTextureArrayAlphaItemModel>>
{
	SLATE_BEGIN_ARGS(STextureArrayAlphaItemTile)
	{
	}
	SLATE_END_ARGS()

	void Construct(
		const FArguments& InArgs,
		TSharedRef<STableViewBase> InOwnerTableView,
		TSharedPtr<FTextureArrayAlphaItemModel>& InModel);
	void Rebuild();

private:
	TSharedPtr<FTextureArrayAlphaItemModel> Model;
	TSharedPtr<STextureArrayTextureSelector> SelectorWidget;
};

class FBrushUICustomization : public IDetailCustomization
{
public:
	UFUNCTION()
	void RefreshTextureArrayPalette(UVSPBrushToolProperties* BrushProperties);

	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailLayout) override;
	static UTexture2D* GetTextureFromArray(UTexture2DArray* Array, int32 Idx);

private:
	TArray<FTextureArrayPaletteItemModelPtr> TextureArrayMaterialItems;
	TArray<FTextureArrayAlphaItemModelPtr> TextureArrayAlphaItems;
	TSharedPtr<class FAssetThumbnailPool> ThumbnailPool;
	TSharedPtr<STextureArrayMaterialTypeTileView> AtlasTilesViewMaterials;
	TSharedPtr<STextureArrayAlphaTypeTileView> AtlasTilesViewAlphaLayers;

	int32 MaterialLayersCount = -1;
	int32 AlphaLayersCount = -1;

public:
	TSharedRef<ITableRow> GenerateMaterialTile(
		FTextureArrayPaletteItemModelPtr Item,
		const TSharedRef<STableViewBase>& OwnerTable);
	TSharedRef<ITableRow> GenerateAlphaTile(
		FTextureArrayAlphaItemModelPtr Item,
		const TSharedRef<STableViewBase>& OwnerTable);

	void AddMaterialsRow(IDetailCategoryBuilder& DetailCategory, UVSPBrushToolProperties* BrushProperties);
	void AddAlphaBlendsRow(IDetailCategoryBuilder& DetailCategory, UVSPBrushToolProperties* BrushProperties);

	static void SetupPaletteWindow(
		UVSPBrushToolProperties* BrushProperties,
		FTextureArrayPaletteItemModelPtr PaletteItem);
};

class FMaskBakeUICustomization : public IDetailCustomization
{
public:
	static TSharedRef<IDetailCustomization> MakeInstance();
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;
};
