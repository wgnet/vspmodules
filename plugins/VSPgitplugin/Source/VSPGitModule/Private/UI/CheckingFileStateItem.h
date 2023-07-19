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
#include "ISourceControlState.h"

class FCheckingTreeItemBase
{
public:
	FCheckingTreeItemBase( const TWeakPtr< FCheckingTreeItemBase >& InParentItem )
		: ParentItem( InParentItem )
		, CheckBoxState( ECheckBoxState::Checked )
	{}

	virtual ~FCheckingTreeItemBase() = default;
public:
	TSharedPtr< FCheckingTreeItemBase > GetParentItem();
	virtual FString GetDisplayName() = 0;

	bool IsChecked();
	virtual ECheckBoxState GetCheckBoxState() const = 0;
	virtual void OnCheckedStateChanged( ECheckBoxState InCheckBoxState ) = 0;

	virtual FString GetStringPath() = 0;
	virtual FText GetTextPath() = 0;

	virtual const FSlateBrush* GetItemIcon() const = 0;

	virtual void GetCheckedFiles( TArray< FString >& OutFilesList ) = 0;
	virtual void GetAllFiles( TArray< FString >& OutFilesList ) = 0;

	virtual const FSlateBrush* GetStateImage() const = 0;
	virtual FText GetStateToolTip() const = 0;

	virtual bool IsFile() = 0;

protected:
	TWeakPtr< FCheckingTreeItemBase > ParentItem;
	ECheckBoxState CheckBoxState;
};

class FCheckingDirectoryItem : public FCheckingTreeItemBase
{
public:
	explicit FCheckingDirectoryItem(
		const TWeakPtr< FCheckingTreeItemBase >& InParentItem,
		const FString& InDirectoryName )
		: FCheckingTreeItemBase( InParentItem )
		, DirectoryName( InDirectoryName )
		, bIsExpanded( false )
	{}

	virtual FString GetDisplayName() override;

	bool IsExpanded() const;
	void SetIsExpanded( bool bInIsExpanded );

	TArray< TSharedPtr< FCheckingTreeItemBase > >& AccessSubItems();
	const TArray< TSharedPtr< FCheckingTreeItemBase > >& GetSubItems();
	void AddSubItem( const TSharedPtr< FCheckingTreeItemBase >& NewSubItemPtr );

	virtual ECheckBoxState GetCheckBoxState() const override;
	virtual void OnCheckedStateChanged( ECheckBoxState InCheckBoxState ) override;

	virtual FString GetStringPath() override;
	virtual FText GetTextPath() override;

	virtual const FSlateBrush* GetItemIcon() const override;
	virtual void GetCheckedFiles( TArray< FString >& OutFilesList ) override;
	virtual void GetAllFiles( TArray< FString >& OutFilesList ) override;

	virtual const FSlateBrush* GetStateImage() const override;
	virtual FText GetStateToolTip() const override;

	virtual bool IsFile() override { return false; }

protected:
	TArray< TSharedPtr< FCheckingTreeItemBase > > SubItems;
	FString DirectoryName;
	bool bIsExpanded;
};

class FCheckingFileStateItem : public FCheckingTreeItemBase
{
public:
	FCheckingFileStateItem(
		const TWeakPtr< FCheckingTreeItemBase >& InParentItem,
		const TSharedRef< ISourceControlState, ESPMode::ThreadSafe >& InFileState )
		: FCheckingTreeItemBase( InParentItem )
		, FileState( InFileState )
	{}

	virtual FString GetDisplayName() override;

	virtual ECheckBoxState GetCheckBoxState() const override;
	virtual void OnCheckedStateChanged( ECheckBoxState InCheckBoxState ) override;

	virtual FString GetStringPath() override;
	virtual FText GetTextPath() override;

	virtual const FSlateBrush* GetItemIcon() const override;
	virtual void GetCheckedFiles( TArray< FString >& OutFilesList ) override;
	virtual void GetAllFiles( TArray< FString >& OutFilesList ) override;

	virtual bool IsFile() override { return true; }

	virtual const FSlateBrush* GetStateImage() const override;
	virtual FText GetStateToolTip() const override;

	const TSharedRef< ISourceControlState, ESPMode::ThreadSafe >& GetState() const;
	void UpdateState( const TSharedRef< ISourceControlState, ESPMode::ThreadSafe >& InState );

protected:
	TSharedRef< ISourceControlState, ESPMode::ThreadSafe > FileState;
};
