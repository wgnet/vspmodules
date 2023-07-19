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
#include "MapSectorLockViewStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "Framework/Application/SlateApplication.h"
#include "Slate/SlateGameResources.h"
#include "Interfaces/IPluginManager.h"

TSharedPtr< FSlateStyleSet > FMapSectorLockViewStyle::StyleInstance = nullptr;

void FMapSectorLockViewStyle::Initialize()
{
	if ( !StyleInstance.IsValid() )
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *StyleInstance );
	}
}

void FMapSectorLockViewStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle( *StyleInstance );
	ensure( StyleInstance.IsUnique() );
	StyleInstance.Reset();
}

FName FMapSectorLockViewStyle::GetStyleSetName()
{
	static FName StyleSetName( TEXT( "MapSectorLockViewStyle" ) );
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

const FVector2D Icon16x16( 16.0f, 16.0f );
const FVector2D Icon20x20( 20.0f, 20.0f );
const FVector2D Icon24x24( 24.0f, 24.0f );
const FVector2D Icon32x32( 32.0f, 32.0f );
const FVector2D Icon40x40( 40.0f, 40.0f );
const FVector2D Icon1024x1024( 1024.0f, 1024.0f );

TSharedRef< FSlateStyleSet > FMapSectorLockViewStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable( new FSlateStyleSet( "MapSectorLockViewStyle" ) );
	Style->SetContentRoot( IPluginManager::Get().FindPlugin( "VSPGitPlugin" )->GetBaseDir() / TEXT( "Resources" ) );

	Style->Set( "MSLV.UserLock", new IMAGE_BRUSH( TEXT("MapSectorViewLockStyle/icon_MyBlock"), Icon24x24 ) );
	Style->Set( "MSLV.MyLock", new IMAGE_BRUSH( TEXT("MapSectorViewLockStyle/icon_UserBlock"), Icon24x24 ) );
	Style->Set( "MSLV.LockedGrid", new IMAGE_BRUSH( TEXT("MapSectorViewLockStyle/Lines_45_Big"), Icon1024x1024 ) );
	Style->Set( "MSLV.LockedGrid.Small", new IMAGE_BRUSH( TEXT("MapSectorViewLockStyle/Lines_45_Smallx2"), Icon1024x1024 ) );

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FMapSectorLockViewStyle::ReloadTextures()
{
	if ( FSlateApplication::IsInitialized() )
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}

const ISlateStyle& FMapSectorLockViewStyle::Get()
{
	return *StyleInstance;
}
