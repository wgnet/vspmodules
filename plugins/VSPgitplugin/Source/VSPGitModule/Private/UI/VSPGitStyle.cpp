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
#include "VSPGitStyle.h"
// Engine headers
#include "Framework/Application/SlateApplication.h"
#include "Brushes/SlateImageBrush.h"
#include "Styling/SlateStyleRegistry.h"
#include "Styling/SlateStyle.h"
#include "Interfaces/IPluginManager.h"
#include "EditorStyleSet.h"

TSharedPtr< FSlateStyleSet > FVSPGitStyle::StyleInstance = nullptr;

const ISlateStyle& FVSPGitStyle::Get()
{
	return *StyleInstance;
}

void FVSPGitStyle::Initialize()
{
	if ( !StyleInstance.IsValid() )
	{
		StyleInstance = Create();
		FSlateStyleRegistry::RegisterSlateStyle( *StyleInstance );
	}
}

void FVSPGitStyle::Shutdown()
{
	FSlateStyleRegistry::UnRegisterSlateStyle( *StyleInstance );
	ensure( StyleInstance.IsUnique() );
	StyleInstance.Reset();
}

FName FVSPGitStyle::GetStyleSetName()
{
	static FName StyleSetName( TEXT( "VSPGitStyle" ) );
	return StyleSetName;
}

#define IMAGE_BRUSH( RelativePath, ... ) FSlateImageBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BOX_BRUSH( RelativePath, ... ) FSlateBoxBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define BORDER_BRUSH( RelativePath, ... ) FSlateBorderBrush( Style->RootToContentDir( RelativePath, TEXT(".png") ), __VA_ARGS__ )
#define TTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".ttf") ), __VA_ARGS__ )
#define OTF_FONT( RelativePath, ... ) FSlateFontInfo( Style->RootToContentDir( RelativePath, TEXT(".otf") ), __VA_ARGS__ )

// Note, these sizes are in Slate Units.
// Slate Units do NOT have to map to pixels.
const FVector2D Icon5x16( 5.0f, 16.0f );
const FVector2D Icon8x4( 8.0f, 4.0f );
const FVector2D Icon8x8( 8.0f, 8.0f );
const FVector2D Icon10x10( 10.0f, 10.0f );
const FVector2D Icon12x12( 12.0f, 12.0f );
const FVector2D Icon12x16( 12.0f, 16.0f );
const FVector2D Icon14x14( 14.0f, 14.0f );
const FVector2D Icon16x16( 16.0f, 16.0f );
const FVector2D Icon20x20( 20.0f, 20.0f );
const FVector2D Icon20x32( 20.0f, 32.0f );
const FVector2D Icon22x22( 22.0f, 22.0f );
const FVector2D Icon24x24( 24.0f, 24.0f );
const FVector2D Icon25x25( 25.0f, 25.0f );
const FVector2D Icon32x32( 32.0f, 32.0f );
const FVector2D Icon40x40( 40.0f, 40.0f );
const FVector2D Icon48x48( 48.0f, 48.0f );
const FVector2D Icon64x64( 64.0f, 64.0f );
const FVector2D Icon36x24( 36.0f, 24.0f );
const FVector2D Icon128x128( 128.0f, 128.0f );
const FVector2D Image( 1600.0f, 1200.0f );

TSharedRef< FSlateStyleSet > FVSPGitStyle::Create()
{
	TSharedRef< FSlateStyleSet > Style = MakeShareable( new FSlateStyleSet( "VSPGitStyle" ) );
#if WITH_EDITOR
	Style->SetContentRoot( IPluginManager::Get().FindPlugin( "VSPGitPlugin" )->GetBaseDir() / TEXT( "Resources" ) );
#elif IS_PROGRAM
	Style->SetContentRoot(FPaths::ProjectDir() / TEXT("Resources"));
#endif

	Style->Set( "VSPGit.OpenMainWindow", new IMAGE_BRUSH( TEXT("GitPluginIcon"), Icon40x40 ) );
	Style->Set( "VSPGit.OpenMainWindow.Small", new IMAGE_BRUSH( TEXT("GitPluginIcon"), Icon20x20 ) );
	Style->Set( "VSPGit.GitBranch", new IMAGE_BRUSH( TEXT("GitBranchOrigin"), Icon40x40 ) );
	Style->Set( "VSPGit.Holder", new IMAGE_BRUSH( TEXT("GitPluginIcon"), Image ) );
	Style->Set( "VSPGit.GitCommitIco", new IMAGE_BRUSH( TEXT("MenuCommit"), Icon48x48 ) );
	Style->Set( "VSPGit.GitFetchIco", new IMAGE_BRUSH( TEXT("MenuFetch"), Icon48x48 ) );
	Style->Set( "VSPGit.LogoIco", new IMAGE_BRUSH( TEXT("Logo"), Icon48x48 ) );
	Style->Set( "VSPGit.PullIco", new IMAGE_BRUSH( TEXT("MenuPull"), Icon48x48 ) );
	Style->Set( "VSPGit.RevertIco", new IMAGE_BRUSH( TEXT("MenuRevert"), Icon48x48 ) );
	Style->Set( "VSPGit.GitBranch", new IMAGE_BRUSH( TEXT("GitBranchOrigin"), Icon40x40 ) );
	Style->Set( "VSPGit.GitBranchIco", new IMAGE_BRUSH( TEXT("GitBranch"), Icon40x40 ) );
	Style->Set( "VSPGit.GitBranchSmallIco", new IMAGE_BRUSH( TEXT("GitBranch.Small"), Icon20x20 ) );
	Style->Set( "VSPGit.GitBranchWhiteIco", new IMAGE_BRUSH( TEXT("GitBranchWhite"), Icon20x32 ) );
	Style->Set( "VSPGit.FetchSpinner", new IMAGE_BRUSH( TEXT("FetchSpinner"), Icon24x24 ) );
	Style->Set( "VSPGit.FolderIco", new IMAGE_BRUSH( TEXT("Folder"), Icon20x20 ) );
	Style->Set( "VSPGit.PushIco", new IMAGE_BRUSH( TEXT("MenuPush"), Icon48x48 ) );
	Style->Set( "VSPGit.Warning", new IMAGE_BRUSH( TEXT("StatusMarks/SCC_NotAtHeadRevision"), Icon16x16 ) );
	Style->Set( "VSPGit.Changes", new IMAGE_BRUSH( TEXT("StatusMarks/SCC_CheckedOut"), Icon16x16 ) );
	Style->Set( "VSPGit.Dirty", new IMAGE_BRUSH( TEXT("StatusMarks/ContentDirty"), Icon16x16 ) );

	// MessageBox icons
	Style->Set( "MessageBox.Info", new IMAGE_BRUSH( TEXT("Common/T_InfoIcon"), Icon64x64 ) );
	Style->Set( "MessageBox.Warning", new IMAGE_BRUSH( TEXT("Common/T_WarningIcon"), Icon64x64 ) );
	Style->Set( "MessageBox.Error", new IMAGE_BRUSH( TEXT("Common/T_ErrorIcon"), Icon64x64 ) );
	Style->Set( "MessageBox.Question", new IMAGE_BRUSH( TEXT("Common/T_QuestionIcon"), Icon64x64 ) );
	Style->Set( "MessageBox.Info.Small", new IMAGE_BRUSH( TEXT("Common/T_InfoIcon"), Icon40x40 ) );
	Style->Set( "MessageBox.Warning.Small", new IMAGE_BRUSH( TEXT("Common/T_WarningIcon"), Icon40x40 ) );
	Style->Set( "MessageBox.Error.Small", new IMAGE_BRUSH( TEXT("Common/T_ErrorIcon"), Icon40x40 ) );
	Style->Set( "MessageBox.Question.Small", new IMAGE_BRUSH( TEXT("Common/T_QuestionIcon"), Icon40x40 ) );

	return Style;
}

#undef IMAGE_BRUSH
#undef BOX_BRUSH
#undef BORDER_BRUSH
#undef TTF_FONT
#undef OTF_FONT

void FVSPGitStyle::ReloadTextures()
{
	if ( FSlateApplication::IsInitialized() )
		FSlateApplication::Get().GetRenderer()->ReloadTextureResources();
}
