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

#include "UObject/StrongObjectPtr.h"
#include "Utils/ParamsUtils.h"

#include "ParamsRegistry.generated.h"

class UParamsSettings;
class UJSONParamsBrowserDataSource;
class FJSONParamsEditorModule;
class FJSONParamsModule;
struct FRequestParamsFromServerTask;
struct FParamRegistryData;

using FParamRegistryDataPtr = TSharedPtr<FParamRegistryData>;

template<typename T_ParamType>
struct FParamSharedPtr
{
	FParamSharedPtr() = delete;

	FParamSharedPtr(const FParamRegistryDataPtr InDataPtr) : DataPtr(InDataPtr)
	{
	}

	FParamSharedPtr(std::nullptr_t)
	{
		DataPtr.Reset();
	}

	operator bool() const
	{
		return DataPtr.IsValid();
	}

	// We need to remove const modifier for reference type because T_ParamType could be const.
	bool CopyTo(typename TRemoveConst<T_ParamType>::Type& OutParam) const;

	const T_ParamType* operator->() const;

private:
	FParamRegistryDataPtr DataPtr = nullptr;
};


USTRUCT(BlueprintType)
struct JSONPARAMS_API FParamRegistryInfo
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	UScriptStruct* Type { nullptr };

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName Name = NAME_None;

	template<typename T_ParamType>
	FParamSharedPtr<const T_ParamType> Get() const;

	bool IsValid() const;

	bool operator==(const FParamRegistryInfo& Other) const;
	bool operator!=(const FParamRegistryInfo& Other) const;
	bool operator<(const FParamRegistryInfo& Other) const;
};

uint32 JSONPARAMS_API GetTypeHash(const FParamRegistryInfo& ParamRegistryInfo);

USTRUCT()
struct JSONPARAMS_API FParamRegistryMeta
{
	GENERATED_BODY()

	UPROPERTY()
	FString FilePath = FString();

	UPROPERTY()
	int32 ParamIndex = INDEX_NONE;

	UPROPERTY()
	bool bIsSavable = false;

	UPROPERTY()
	bool bIsChanged = false;
};

USTRUCT()
struct JSONPARAMS_API FParamRegistryData
{
	GENERATED_BODY()

	UPROPERTY()
	FParamRegistryInfo Info {};

#if WITH_EDITORONLY_DATA

	UPROPERTY()
	FParamRegistryMeta Meta {};
#endif

	UPROPERTY()
	TArray<uint8> Data;
};

struct JSONPARAMS_API FParamsRegistry
{
	friend FRequestParamsFromServerTask;
	friend FJSONParamsModule;
	friend FJSONParamsEditorModule;
	friend UParamsSettings;
	friend UJSONParamsBrowserDataSource;

	static FParamsRegistry& Get();

	void RequestParamsFromServer();

	void CallWhenInitialized(TFunction<void()> Callback);

	static FParamRegistryDataPtr GetParam(const UScriptStruct* Type, const FName& ID);
	static FParamRegistryDataPtr GetParam(const FParamRegistryInfo& Param);

	template<typename T_ParamType>
	static FParamSharedPtr<const T_ParamType> GetParam(const FName& ID);
	template<typename T_ParamType>
	static FParamSharedPtr<const T_ParamType> GetParam(const FParamRegistryInfo& Param);

	int32 GetFailedParamsNumber() const;

	FText GetRegistryInfoText() const;

	TArray<UScriptStruct*> GetRegisteredTypes() const;
	TArray<FName> GetRegisteredNames(const UScriptStruct* Type) const;

#if WITH_EDITOR
	static FName GetParamVirtualPath(
		const UScriptStruct* Type,
		const FName& Name,
		const TArray<FString>& ParamsRootPaths);
	static bool CreateAndSaveParam(UScriptStruct* Type, const FName& ID, const FString& InFullPath);
	static bool SaveParamOnDisk(UScriptStruct* Type, const FName& ID);
	static bool SaveParamDataOnDisk(
		UScriptStruct* Type,
		const FName& Name,
		const void* ParamData,
		const FString& FilePath,
		int32& IndexInFile);
	static bool VerifyParamIndexForArray(
		int32& ParamIndex,
		const UScriptStruct* Type,
		const FName& Name,
		const TArray<TSharedPtr<FJsonValue>>& ParamsArray);
	static void ModifyParamChangedState(const UScriptStruct* Type, const FName& ID, bool IsChanged = true);
	static bool LoadParamFromDisk(UScriptStruct* Type, const FName& Name, const FString& FilePath, int32 IndexInFile);

	DECLARE_MULTICAST_DELEGATE(FOnParamsInitStarted);
	FOnParamsInitStarted OnParamsInitStarted;

	DECLARE_MULTICAST_DELEGATE(FOnParamsReloaded);
	FOnParamsReloaded OnParamsReloaded;
#endif

protected:
	static FParamsRegistry ParamsRegistry;

	TMap<UScriptStruct*, TMap<FName, FParamRegistryDataPtr>> ParamsTree;

#if WITH_EDITOR
	FCriticalSection EditorParamUniqueNameCheckMutex;
#endif

	FCriticalSection AddParamMutex;

	FThreadSafeBool ParamsInitialization;
	FThreadSafeCounter FailedParamsNumber;

	void GetParamsFromServer();
	int32 CheckParamsRefcounts();

	void ReloadParams(IPlatformFile* PlatformFile, const TArray<FString>& ParamsPaths);
	bool AddParam(
		const FParamRegistryInfo& ParamInfo,
		const FParamRegistryMeta* ParamMeta,
		TSharedPtr<FJsonObject> JsonData);

	FCriticalSection WhenInitCallbacksMutex;
	TArray<TFunction<void()>> WhenInitCallbacks;

	TArray<TStrongObjectPtr<const UObject>> UsedInstancedObjectsPtrs;

private:
	void Init();

	void AddParamsFromJsonObjects(const TArray<FJsonDataWithMeta>& DataWithContexts, bool IsDataFromDisk = false);
	void FinishParamsInitialization();

	void SavePointersToInstancedObjects(FParamRegistryDataPtr Param);

	void TraverseProperties(const UClass* Class, const void* Data);

	void TraverseProperty(const FProperty* Property, const void* Data);
	void TraverseArrayProperty(const FArrayProperty* Property, const void* Data);
	void TraverseSetProperty(const FSetProperty* Property, const void* Data);
	void TraverseMapProperty(const FMapProperty* Property, const void* Data);
};


template<typename T_ParamType>
bool FParamSharedPtr<T_ParamType>::CopyTo(typename TRemoveConst<T_ParamType>::Type& OutParam) const
{
	bool bRes = false;

	if (DataPtr)
	{
		OutParam = *(operator->());
		bRes = true;
	}

	return bRes;
}

template<typename T_ParamType>
const T_ParamType* FParamSharedPtr<T_ParamType>::operator->() const
{
	return static_cast<const T_ParamType*>(static_cast<void*>(DataPtr->Data.GetData()));
}

template<typename T_ParamType>
FParamSharedPtr<const T_ParamType> FParamRegistryInfo::Get() const
{
	return FParamsRegistry::GetParam<T_ParamType>(Name);
}

template<typename T_ParamType>
FParamSharedPtr<const T_ParamType> FParamsRegistry::GetParam(const FName& ID)
{
	return FParamsRegistry::GetParam(T_ParamType::StaticStruct(), ID);
}

template<typename T_ParamType>
FParamSharedPtr<const T_ParamType> FParamsRegistry::GetParam(const FParamRegistryInfo& Param)
{
	if (Param.Type == T_ParamType::StaticStruct())
	{
		return GetParam(Param);
	}

	return nullptr;
}
