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

#include "ContentInspectorStyle.h"

#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"

TSharedPtr<FSlateStyleSet> FContentInspectorStyle::StyleInstance = nullptr;

void FContentInspectorStyle::Initialize()
{
	if (!StyleInstance.IsValid())
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle(*StyleInstance);
	}
}

void FContentInspectorStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle(*StyleInstance);
	ensure(StyleInstance.IsUnique());
	StyleInstance.Reset();
}

void FContentInspectorStyle::ReloadTextures()
{
	if (FSlateApplication::IsInitialized())
	{
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
	}
}

FName FContentInspectorStyle::GetStyleSetName()
{
	static FName StyleSetName(TEXT("ContentInspectorStyle"));
	return StyleSetName;
}
#define IMAGE_BRUSH(RelativePath, ...) \
	FSlateImageBrush(Style->RootToContentDir(RelativePath, TEXT(".png")), __VA_ARGS__)
const FVector2D Icon40x40(40.0f, 40.0f);

TSharedRef<FSlateStyleSet> FContentInspectorStyle::Create()
{
	TSharedRef<FSlateStyleSet> Style = MakeShareable(new FSlateStyleSet("ContentInspectorStyle"));
	Style->SetContentRoot(IPluginManager::Get().FindPlugin("ContentInspector")->GetBaseDir() / TEXT("Resources"));

	Style->Set("ContentInspector.BookmarkGreen", new IMAGE_BRUSH(TEXT("BookmarkGreen_40x"), Icon40x40));
	Style->Set("ContentInspector.BookmarkOrange", new IMAGE_BRUSH(TEXT("BookmarkOrange_40x"), Icon40x40));
	Style->Set("ContentInspector.BookmarkRed", new IMAGE_BRUSH(TEXT("BookmarkRed_40x"), Icon40x40));
	Style->Set("ContentInspector.BookmarkGray", new IMAGE_BRUSH(TEXT("BookmarkGray_40x"), Icon40x40));
	Style->Set("ContentInspector.OrangeColor", new IMAGE_BRUSH(TEXT("OrangeColor_16x"), Icon40x40));
	Style->Set("ContentInspector.RedColor", new IMAGE_BRUSH(TEXT("RedColor_16x"), Icon40x40));

	return Style;
}

#undef IMAGE_BRUSH

const ISlateStyle& FContentInspectorStyle::Get()
{
	return *StyleInstance;
}
