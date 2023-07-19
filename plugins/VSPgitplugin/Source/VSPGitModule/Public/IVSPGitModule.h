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

#include "Modules/ModuleInterface.h"

class VSPGITMODULE_API IVSPGitModule : public IModuleInterface
{
public:
	virtual TSharedPtr< FTabManager > GetTabManager() { return nullptr; }

	// git info about user
	virtual FString GetGitUserName() const { return {}; }
	virtual FString GetGitUserEmail() const { return {}; }

	// methods for accessing git lfs locking capabilities
	virtual bool IsFileLocked( const FString& InContentPath, FString& OutLockedUser, int32& OutLockId ) { return false; }

	virtual void LockMaps( const TArray< FString >& InContentPathList )	{}
	virtual void UnlockMaps( const TArray< FString >& InContentPathList, bool bIsForce = false ) {}
};
