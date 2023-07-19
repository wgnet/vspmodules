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

//Engine headers
#include "Core.h"
// Module headers
#include "IVSPGitModule.h"
#include "Core/VSPGitProvider.h"

DECLARE_LOG_CATEGORY_EXTERN( VSPGitLog, Log, All );

class VSPGITMODULE_API FVSPGitModule : public IVSPGitModule
{
public:
	// Module quick access methods
	static FVSPGitModule& Get();
	static FName GetModuleName();
	// Provider quick access methods
	FVSPGitProvider& GetProvider();
	FVSPGitProvider& AccessProvider();

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

	/** IVSPGitModule implementation **/
	virtual TSharedPtr< FTabManager > GetTabManager() override;
	// git info about user
	virtual FString GetGitUserName() const override;
	virtual FString GetGitUserEmail() const override;

	// methods for accessing git lfs locking capabilities
	virtual bool IsFileLocked( const FString& InContentPath, FString& OutLockedUser, int32& OutLockId ) override;

	virtual void LockMaps( const TArray< FString >& InContentPathList ) override;
	virtual void UnlockMaps( const TArray< FString >& InContentPathList, bool bIsForce ) override;

private:
	FVSPGitProvider Provider;
};
