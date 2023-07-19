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

#include "ContentBrowserDataSource.h"

#include "JSONParamsBrowserDataSource.generated.h"


USTRUCT()
struct JSONPARAMSBROWSER_API FJSONParamsBrowserDataFilter
{
	GENERATED_BODY()

	UPROPERTY()
	TArray<FName> MatchingFolders;

	UPROPERTY()
	TArray<FName> MatchingParams;
};


struct FJSONParamsFolderChildrenData
{
	TArray<FName> Folders;

	TArray<FName> Params;
};

UCLASS()
class JSONPARAMSBROWSER_API UJSONParamsBrowserDataSource : public UContentBrowserDataSource
{
	GENERATED_BODY()

public:
	void Initialize(const FName InMountRoot, const bool InAutoRegister = true);

	// Filtering
	virtual void CompileFilter(
		const FName InPath,
		const FContentBrowserDataFilter& InFilter,
		FContentBrowserDataCompiledFilter& OutCompiledFilter) override;
	virtual void EnumerateItemsMatchingFilter(
		const FContentBrowserDataCompiledFilter& InFilter,
		TFunctionRef<bool(FContentBrowserItemData&&)> InCallback) override;
	virtual void EnumerateItemsAtPath(
		const FName InPath,
		const EContentBrowserItemTypeFilter InItemTypeFilter,
		TFunctionRef<bool(FContentBrowserItemData&&)> InCallback) override;
	virtual bool IsFolderVisibleIfHidingEmpty(const FName InPath) override;
	virtual bool DoesItemPassFilter(
		const FContentBrowserItemData& InItem,
		const FContentBrowserDataCompiledFilter& InFilter) override;

	// Item processing
	virtual bool GetItemPhysicalPath(const FContentBrowserItemData& InItem, FString& OutDiskPath) override;
	virtual bool IsItemDirty(const FContentBrowserItemData& InItem) override;
	virtual bool CanEditItem(const FContentBrowserItemData& InItem, FText* OutErrorMsg) override;
	virtual bool EditItem(const FContentBrowserItemData& InItem) override;
	virtual bool BulkEditItems(TArrayView<const FContentBrowserItemData> InItems) override;
	virtual bool AppendItemReference(const FContentBrowserItemData& InItem, FString& InOutStr) override;
	virtual bool GetItemAttribute(
		const FContentBrowserItemData& InItem,
		const bool InIncludeMetaData,
		const FName InAttributeKey,
		FContentBrowserItemDataAttributeValue& OutAttributeValue) override;

	virtual bool UpdateThumbnail(const FContentBrowserItemData& InItem, FAssetThumbnail& InThumbnail) override;

protected:
	virtual void EnumerateRootPaths(const FContentBrowserDataFilter& InFilter, TFunctionRef<void(FName)> InCallback)
		override;

private:
	TMap<FName, FJSONParamsFolderChildrenData> ParamsFolders {};
	TMap<FName, UScriptStruct*> Params {};

	void ToggleEnableState();
	void PopulateAddNewContextMenu(UToolMenu* InMenu);
	void UpdateParamsData();
	void GetFoldersChildrenRecursive(FName Path, TArray<FName>& OutFolders, const bool bRecurse);
	void GetFilesChildrenRecursive(FName Path, TArray<FName>& OutFiles, const bool bRecurse);
	FContentBrowserItemData CreateParamsFolderItem(const FName InFolderPath);
	FContentBrowserItemData CreateParamItem(const FName InParamPath);

	void BrowseToParam(const UScriptStruct* Type, const FName& Name);
};
