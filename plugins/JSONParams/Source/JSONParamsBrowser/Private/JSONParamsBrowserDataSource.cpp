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
#include "JSONParamsBrowserDataSource.h"
#include "JSONParamsBrowserDataPayload.h"

#include "JSONParamsEditorModule.h"
#include "JSONParamsEditorTabManager.h"
#include "ParamsRegistry.h"
#include "ParamsSettings.h"

#include "ContentBrowserDataMenuContexts.h"
#include "ContentBrowserDataSubsystem.h"
#include "ContentBrowserModule.h"
#include "IContentBrowserSingleton.h"
#include "ToolMenus.h"


namespace FJSONParamsBrowserDataSourceLocal
{
	const FName Root = "/";
	const FName Type = "UParam";
	const FString Description = "JSON parameter";
}

void UJSONParamsBrowserDataSource::Initialize(const FName InMountRoot, const bool InAutoRegister)
{
	Super::Initialize(InMountRoot, InAutoRegister);

	// Subscriptions
	FParamsRegistry::Get().OnParamsReloaded.AddUObject(this, &UJSONParamsBrowserDataSource::UpdateParamsData);
	FJSONParamsEditorModule::OnBrowseToParamDelegate.BindUObject(this, &UJSONParamsBrowserDataSource::BrowseToParam);

	// Bind menu extensions
	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AddNewContextMenu"))
	{
		Menu->AddDynamicSection(
			*FString::Printf(TEXT("DynamicSection_DataSource_%s"), *GetName()),
			FNewToolMenuDelegate::CreateLambda(
				[WeakThis = TWeakObjectPtr<UJSONParamsBrowserDataSource>(this)](UToolMenu* InMenu)
				{
					if (UJSONParamsBrowserDataSource* This = WeakThis.Get())
					{
						This->PopulateAddNewContextMenu(InMenu);
					}
				}));
	}

	if (UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("ContentBrowser.AssetViewOptions"))
	{
		FToolMenuSection& Section = Menu->AddSection("Content");
		Section.AddMenuEntry(
			"ShowParams",
			FText::FromString("Show JSON params"),
			FText::FromString("Show JSON params"),
			FSlateIcon(),
			FUIAction(
				FExecuteAction::CreateUObject(this, &UJSONParamsBrowserDataSource::ToggleEnableState),
				FCanExecuteAction::CreateLambda(
					[this]()
					{
						return true;
					}),
				FIsActionChecked::CreateLambda(
					[this]()
					{
						return UParamsSettings::Get()->GetEnableContentBrowserIntegration();
					})),
			EUserInterfaceActionType::ToggleButton);
	}
}

void UJSONParamsBrowserDataSource::EnumerateRootPaths(
	const FContentBrowserDataFilter& InFilter,
	TFunctionRef<void(FName)> InCallback)
{
	InCallback(UParamsSettings::Get()->GetContentBrowserVirtualRoot());
}

void UJSONParamsBrowserDataSource::CompileFilter(
	const FName InPath,
	const FContentBrowserDataFilter& InFilter,
	FContentBrowserDataCompiledFilter& OutCompiledFilter)
{
	if (!UParamsSettings::Get()->GetEnableContentBrowserIntegration())
		return;

	const FContentBrowserDataObjectFilter* ObjectFilter =
		InFilter.ExtraFilters.FindFilter<FContentBrowserDataObjectFilter>();
	const FContentBrowserDataPackageFilter* PackageFilter =
		InFilter.ExtraFilters.FindFilter<FContentBrowserDataPackageFilter>();
	const FContentBrowserDataClassFilter* ClassFilter =
		InFilter.ExtraFilters.FindFilter<FContentBrowserDataClassFilter>();
	const FContentBrowserDataCollectionFilter* CollectionFilter =
		InFilter.ExtraFilters.FindFilter<FContentBrowserDataCollectionFilter>();

	const bool bIncludeFolders =
		EnumHasAnyFlags(InFilter.ItemTypeFilter, EContentBrowserItemTypeFilter::IncludeFolders);
	const bool bIncludeFiles = EnumHasAnyFlags(InFilter.ItemTypeFilter, EContentBrowserItemTypeFilter::IncludeFiles);

	const bool bIncludeMisc =
		EnumHasAnyFlags(InFilter.ItemCategoryFilter, EContentBrowserItemCategoryFilter::IncludeMisc);

	FContentBrowserDataFilterList& FilterList = OutCompiledFilter.CompiledFilters.FindOrAdd(this);
	FJSONParamsBrowserDataFilter& FileDataFilter = FilterList.FindOrAddFilter<FJSONParamsBrowserDataFilter>();

	// If we aren't including anything, then we can just bail now
	if (!bIncludeMisc || (!bIncludeFolders && !bIncludeFiles))
		return;

	// If we have any inclusive object, package, class or collection filters, then skip
	if ((ObjectFilter
		 && (ObjectFilter->ObjectNamesToInclude.Num() > 0 || ObjectFilter->TagsAndValuesToInclude.Num() > 0))
		|| (PackageFilter
			&& (PackageFilter->PackageNamesToInclude.Num() > 0 || PackageFilter->PackagePathsToInclude.Num() > 0))
		|| (ClassFilter && ClassFilter->ClassNamesToInclude.Num() > 0)
		|| (CollectionFilter && CollectionFilter->SelectedCollections.Num() > 0))
		return;

	//Check path
	FName CorrectedPath;
	if (InPath.IsEqual(FJSONParamsBrowserDataSourceLocal::Root))
	{
		CorrectedPath = UParamsSettings::Get()->GetContentBrowserVirtualRoot();

		if (bIncludeFolders)
			FileDataFilter.MatchingFolders.Add(CorrectedPath);

		if (!InFilter.bRecursivePaths)
			return;
	}
	else
	{
		CorrectedPath = InPath;
	}
	if (CorrectedPath.IsNone())
		return;


	// Find the child folders
	if (bIncludeFolders)
		GetFoldersChildrenRecursive(CorrectedPath, FileDataFilter.MatchingFolders, InFilter.bRecursivePaths);

	// Find the child params
	if (bIncludeFiles)
		GetFilesChildrenRecursive(CorrectedPath, FileDataFilter.MatchingParams, InFilter.bRecursivePaths);
}

void UJSONParamsBrowserDataSource::EnumerateItemsMatchingFilter(
	const FContentBrowserDataCompiledFilter& InFilter,
	TFunctionRef<bool(FContentBrowserItemData&&)> InCallback)
{
	const FContentBrowserDataFilterList* FilterList = InFilter.CompiledFilters.Find(this);
	if (!FilterList)
		return;

	const FJSONParamsBrowserDataFilter* ParamsDataFilter = FilterList->FindFilter<FJSONParamsBrowserDataFilter>();
	if (!ParamsDataFilter)
		return;

	if (EnumHasAnyFlags(InFilter.ItemTypeFilter, EContentBrowserItemTypeFilter::IncludeFolders))
	{
		for (const FName& Folder : ParamsDataFilter->MatchingFolders)
		{
			if (!InCallback(CreateParamsFolderItem(Folder)))
				return;
		}
	}

	if (EnumHasAnyFlags(InFilter.ItemTypeFilter, EContentBrowserItemTypeFilter::IncludeFiles))
	{
		for (const FName& Param : ParamsDataFilter->MatchingParams)
		{
			if (!InCallback(CreateParamItem(Param)))
				return;
		}
	}
}

void UJSONParamsBrowserDataSource::EnumerateItemsAtPath(
	const FName InPath,
	const EContentBrowserItemTypeFilter InItemTypeFilter,
	TFunctionRef<bool(FContentBrowserItemData&&)> InCallback)
{
	if (EnumHasAnyFlags(InItemTypeFilter, EContentBrowserItemTypeFilter::IncludeFolders)
		&& ParamsFolders.Contains(InPath))
		InCallback(CreateParamsFolderItem(InPath));

	if (EnumHasAnyFlags(InItemTypeFilter, EContentBrowserItemTypeFilter::IncludeFiles) && Params.Contains(InPath))
		InCallback(CreateParamItem(InPath));
}

bool UJSONParamsBrowserDataSource::IsFolderVisibleIfHidingEmpty(const FName InPath)
{
	return ParamsFolders.Contains(InPath);
}

bool UJSONParamsBrowserDataSource::DoesItemPassFilter(
	const FContentBrowserItemData& InItem,
	const FContentBrowserDataCompiledFilter& InFilter)
{
	const FContentBrowserDataFilterList* FilterList = InFilter.CompiledFilters.Find(this);
	if (!FilterList)
		return false;


	const FJSONParamsBrowserDataFilter* ParamsDataFilter = FilterList->FindFilter<FJSONParamsBrowserDataFilter>();
	if (!ParamsDataFilter)
		return false;

	switch (InItem.GetItemType())
	{
	case EContentBrowserItemFlags::Type_Folder:
		if (EnumHasAnyFlags(InFilter.ItemTypeFilter, EContentBrowserItemTypeFilter::IncludeFolders)
			&& ParamsDataFilter->MatchingFolders.Num() > 0)
			return ParamsDataFilter->MatchingFolders.Contains(InItem.GetVirtualPath());

		break;

	case EContentBrowserItemFlags::Type_File:
		if (EnumHasAnyFlags(InFilter.ItemTypeFilter, EContentBrowserItemTypeFilter::IncludeFiles)
			&& ParamsDataFilter->MatchingParams.Num() > 0)
			return ParamsDataFilter->MatchingParams.Contains(InItem.GetVirtualPath());

		break;

	default:
		break;
	}

	return false;
}

bool UJSONParamsBrowserDataSource::GetItemPhysicalPath(const FContentBrowserItemData& InItem, FString& OutDiskPath)
{
	const TSharedPtr<const FJSONParamsBrowserDataPayload> Payload =
		StaticCastSharedPtr<const FJSONParamsBrowserDataPayload>(InItem.GetPayload());
	if (!Params.Contains(InItem.GetVirtualPath()) || !Payload->IsParamValid())
		return false;

	OutDiskPath = Payload->GetParamPhysicalPath();
	return !OutDiskPath.IsEmpty();
}

bool UJSONParamsBrowserDataSource::IsItemDirty(const FContentBrowserItemData& InItem)
{
	if (!Params.Contains(InItem.GetVirtualPath()))
		return false;

	const TSharedPtr<const FJSONParamsBrowserDataPayload> Payload =
		StaticCastSharedPtr<const FJSONParamsBrowserDataPayload>(InItem.GetPayload());
	return Payload->IsParamChanged();
}

bool UJSONParamsBrowserDataSource::CanEditItem(const FContentBrowserItemData& InItem, FText* OutErrorMsg)
{
	const TSharedPtr<const FJSONParamsBrowserDataPayload> Payload =
		StaticCastSharedPtr<const FJSONParamsBrowserDataPayload>(InItem.GetPayload());
	const FName ItemVirtualPath = InItem.GetVirtualPath();

	if (Params.Contains(ItemVirtualPath) && Payload->IsParamValid())
		return true;

	if (OutErrorMsg != nullptr)
	{
		*OutErrorMsg = FParamsRegistry::Get().ParamsInitialization
			? FText::FromString("Params Registry is initializing. Please try again later.")
			: FText::FromString(
				FString::Format(TEXT("Param \"{0}\" not found in registry"), { ItemVirtualPath.ToString() }));
	}
	return false;
}

bool UJSONParamsBrowserDataSource::EditItem(const FContentBrowserItemData& InItem)
{
	const TSharedPtr<const FJSONParamsBrowserDataPayload> Payload =
		StaticCastSharedPtr<const FJSONParamsBrowserDataPayload>(InItem.GetPayload());
	if (!Params.Contains(InItem.GetVirtualPath()) || !Payload->IsParamValid())
		return false;

	FJSONParamsEditorTabManager::Get()->OpenJSONParamsEditorTab(Payload->GetParamType(), Payload->GetParamName());
	return true;
}

bool UJSONParamsBrowserDataSource::BulkEditItems(TArrayView<const FContentBrowserItemData> InItems)
{
	for (const FContentBrowserItemData& InItem : InItems)
	{
		if (!EditItem(InItem))
			return false;
	}
	return true;
}

bool UJSONParamsBrowserDataSource::AppendItemReference(const FContentBrowserItemData& InItem, FString& InOutStr)
{
	const TSharedPtr<const FJSONParamsBrowserDataPayload> Payload =
		StaticCastSharedPtr<const FJSONParamsBrowserDataPayload>(InItem.GetPayload());
	if (!Params.Contains(InItem.GetVirtualPath()) || !Payload->IsParamValid())
		return false;

	InOutStr = FString::Format(
		TEXT("(Type=ScriptStruct'\"{0}\"',Name=\"{1}\")"),
		{ Payload->GetParamType()->GetPathName(), Payload->GetParamName().ToString() });
	return true;
}

bool UJSONParamsBrowserDataSource::GetItemAttribute(
	const FContentBrowserItemData& InItem,
	const bool InIncludeMetaData,
	const FName InAttributeKey,
	FContentBrowserItemDataAttributeValue& OutAttributeValue)
{
	using namespace FJSONParamsBrowserDataSourceLocal;

	if (InItem.IsFolder())
	{
		const FName Path = InItem.GetVirtualPath();
		if (InAttributeKey == ContentBrowserItemAttributes::ItemColor && ParamsFolders.Contains(Path)
			&& ParamsFolders.Find(Path)->Params.Num() != 0)
		{
			OutAttributeValue.SetValue(FColor::Transparent.ToString());
			return true;
		}
		return false;
	}
	if (InItem.IsFile())
	{
		if (!Params.Contains(InItem.GetVirtualPath()))
			return false;

		const TSharedPtr<const FJSONParamsBrowserDataPayload> Payload =
			StaticCastSharedPtr<const FJSONParamsBrowserDataPayload>(InItem.GetPayload());

		if (InAttributeKey == ContentBrowserItemAttributes::ItemTypeName)
		{
			OutAttributeValue.SetValue(Type);
			return true;
		}
		if (InAttributeKey == ContentBrowserItemAttributes::ItemTypeDisplayName)
		{
			OutAttributeValue.SetValue(Payload->GetParamType()->GetName());
			return true;
		}
		if (InAttributeKey == ContentBrowserItemAttributes::ItemDescription)
		{
			OutAttributeValue.SetValue(Description);
			return true;
		}
	}
	return false;
}

bool UJSONParamsBrowserDataSource::UpdateThumbnail(const FContentBrowserItemData& InItem, FAssetThumbnail& InThumbnail)
{
	const TSharedPtr<const FJSONParamsBrowserDataPayload> Payload =
		StaticCastSharedPtr<const FJSONParamsBrowserDataPayload>(InItem.GetPayload());
	Payload->UpdateThumbnail(InThumbnail);
	return Super::UpdateThumbnail(InItem, InThumbnail);
}

void UJSONParamsBrowserDataSource::ToggleEnableState()
{
	const bool bIsEnabled = !UParamsSettings::Get()->GetEnableContentBrowserIntegration();
	UParamsSettings::SetEnableContentBrowserIntegration(bIsEnabled);
	if (bIsEnabled)
	{
		UpdateParamsData();
	}
	else
	{
		NotifyItemDataRefreshed();
	}
}

void UJSONParamsBrowserDataSource::PopulateAddNewContextMenu(UToolMenu* InMenu)
{
	const UContentBrowserDataMenuContext_AddNewMenu* ContextObject =
		InMenu->FindContext<UContentBrowserDataMenuContext_AddNewMenu>();
	if (ContextObject == nullptr || ContextObject->SelectedPaths.Num() != 1
		|| !ParamsFolders.Contains(ContextObject->SelectedPaths[0]))
		return;

	FToolMenuSection& Section = InMenu->AddSection("UParamsBrowser", FText::FromString("UParams"));
	Section.AddMenuEntry(
		"Create UParam",
		FText::FromString("Create UParam"),
		FText::FromString("Create NEW UParam"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda(
				[this]()
				{
					FGlobalTabmanager::Get()->TryInvokeTab(FName("CreateJSONParams"));
				}),
			FCanExecuteAction::CreateLambda(
				[this]()
				{
					return true;
				})),
		EUserInterfaceActionType::Button);
	Section.AddMenuEntry(
		"Reinit",
		FText::FromString("Reinit UParams"),
		FText::FromString("Reinit UParams registry"),
		FSlateIcon(),
		FUIAction(
			FExecuteAction::CreateLambda(
				[this]()
				{
					FParamsRegistry::Get().Init();
				}),
			FCanExecuteAction::CreateLambda(
				[this]()
				{
					return true;
				})),
		EUserInterfaceActionType::Button);
}

void UJSONParamsBrowserDataSource::UpdateParamsData()
{
	if (!UParamsSettings::Get()->GetEnableContentBrowserIntegration())
		return;

	ParamsFolders.Empty();
	Params.Empty();

	const TArray<FString>& ParamsRootPaths = UParamsSettings::Get()->GetParamsRootPaths(true);

	for (UScriptStruct* ParamType : FParamsRegistry::Get().GetRegisteredTypes())
	{
		for (FName ParamName : FParamsRegistry::Get().GetRegisteredNames(ParamType))
		{
			FName ParamVirtualPath = FParamsRegistry::GetParamVirtualPath(ParamType, ParamName, ParamsRootPaths);

			if (ParamVirtualPath == NAME_None || Params.Contains(ParamVirtualPath))
				continue;

			Params.Add(ParamVirtualPath, ParamType);

			FString ParamFolderPath = FPaths::GetPath(ParamVirtualPath.ToString());
			FJSONParamsFolderChildrenData& ParamFolder = ParamsFolders.FindOrAdd(FName(ParamFolderPath));
			ParamFolder.Params.AddUnique(ParamVirtualPath);

			FString FolderPath = ParamFolderPath;
			FString ParentPath = FPaths::GetPath(FolderPath);
			while (!ParentPath.IsEmpty())
			{
				FJSONParamsFolderChildrenData& ParentFolder = ParamsFolders.FindOrAdd(FName(ParentPath));
				ParentFolder.Folders.AddUnique(FName(FolderPath));
				FolderPath = ParentPath;
				ParentPath = FPaths::GetPath(FolderPath);
			}
		}
	}

	NotifyItemDataRefreshed();
}

void UJSONParamsBrowserDataSource::GetFoldersChildrenRecursive(
	FName Path,
	TArray<FName>& OutFolders,
	const bool bRecurse)
{
	if (!ParamsFolders.Contains(Path))
		return;

	const TArray<FName>& FoldersChildren = ParamsFolders.Find(Path)->Folders;
	OutFolders.Append(FoldersChildren);

	if (bRecurse)
	{
		for (const FName ChildFolder : FoldersChildren)
			GetFoldersChildrenRecursive(ChildFolder, OutFolders, bRecurse);
	}
}

void UJSONParamsBrowserDataSource::GetFilesChildrenRecursive(FName Path, TArray<FName>& OutFiles, const bool bRecurse)
{
	if (!ParamsFolders.Contains(Path))
		return;

	const TArray<FName>& ParamsChildren = ParamsFolders.Find(Path)->Params;
	OutFiles.Append(ParamsChildren);

	if (bRecurse)
	{
		const TArray<FName>& FoldersChildren = ParamsFolders.Find(Path)->Folders;
		for (const FName ChildFolder : FoldersChildren)
			GetFilesChildrenRecursive(ChildFolder, OutFiles, bRecurse);
	}
}

FContentBrowserItemData UJSONParamsBrowserDataSource::CreateParamsFolderItem(const FName InFolderPath)
{
	constexpr EContentBrowserItemFlags Flags =
		EContentBrowserItemFlags::Type_Folder | EContentBrowserItemFlags::Category_Misc;
	const FString ItemName = FPaths::GetPathLeaf(InFolderPath.ToString());
	return FContentBrowserItemData(this, Flags, InFolderPath, *ItemName, FText(), nullptr);
}

FContentBrowserItemData UJSONParamsBrowserDataSource::CreateParamItem(const FName InParamPath)
{
	constexpr EContentBrowserItemFlags Flags =
		EContentBrowserItemFlags::Type_File | EContentBrowserItemFlags::Category_Misc;
	const FString ItemName = FPaths::GetPathLeaf(InParamPath.ToString());
	UScriptStruct* ParamType = Params.Contains(InParamPath) ? *Params.Find(InParamPath) : nullptr;
	return FContentBrowserItemData(
		this,
		Flags,
		InParamPath,
		*ItemName,
		FText(),
		MakeShared<FJSONParamsBrowserDataPayload>(*ItemName, ParamType));
}

void UJSONParamsBrowserDataSource::BrowseToParam(const UScriptStruct* Type, const FName& Name)
{
	const TArray<FString>& ParamsRootPaths = UParamsSettings::Get()->GetParamsRootPaths(true);
	const FName ParamVirtualPath = FParamsRegistry::GetParamVirtualPath(Type, Name, ParamsRootPaths);
	if (ParamVirtualPath == NAME_None)
		return;

	constexpr EContentBrowserItemFlags Flags =
		EContentBrowserItemFlags::Type_File | EContentBrowserItemFlags::Category_Misc;
	const FContentBrowserItemData ItemData =
		FContentBrowserItemData(this, Flags, ParamVirtualPath, Name, FText(), nullptr);

	FGlobalTabmanager::Get()->SetActiveTab(nullptr);
	const FContentBrowserModule& ContentBrowserModule =
		FModuleManager::Get().LoadModuleChecked<FContentBrowserModule>("ContentBrowser");
	ContentBrowserModule.Get().SyncBrowserToItems(TArray<FContentBrowserItem> { FContentBrowserItem(ItemData) });
}
