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
#include "ParamsRegistry.h"

#include "ParamsSettings.h"
#include "RequestParamsFromServer.h"
#include "Utils/ParamsUtils.h"

#include "Async/ParallelFor.h"
#include "Dom/JsonObject.h"
#include "JsonObjectConverter.h"
#include "Misc/FileHelper.h"
#include "ParamsValidationManager.h"

namespace FParamsRegistryLocal
{
	const FString HeaderKey = "header";
	const FString DataKey = "data";
}

bool FParamRegistryInfo::IsValid() const
{
	return Name.IsValid() && !Name.IsNone() && Type;
}

bool FParamRegistryInfo::operator==(const FParamRegistryInfo& Other) const
{
	return Name == Other.Name && Type == Other.Type;
}

bool FParamRegistryInfo::operator!=(const FParamRegistryInfo& Other) const
{
	return !(Name == Other.Name && Type == Other.Type);
}

bool FParamRegistryInfo::operator<(const FParamRegistryInfo& Other) const
{
	return Name.LexicalLess(Other.Name);
}

uint32 GetTypeHash(const FParamRegistryInfo& ParamRegistryInfo)
{
	return GetTypeHash(ParamRegistryInfo.Name);
}
FParamsRegistry FParamsRegistry::ParamsRegistry;

FParamsRegistry& FParamsRegistry::Get()
{
	return ParamsRegistry;
}

void FParamsRegistry::Init()
{
	FailedParamsNumber.Reset();

	if (ParamsInitialization.AtomicSet(true))
	{
		UE_LOG(LogParams, Error, TEXT("FParamsRegistry::Init: called before another initialization has finished"));
		return;
	}

	if (IsEngineExitRequested())
	{
		UE_LOG(LogParams, Warning, TEXT("FParamsRegistry::Init: called when IsEngineExitRequested"));
		return;
	}

	TRACE_CPUPROFILER_EVENT_SCOPE(FParamsRegistry::Init);

#if WITH_EDITOR
	Get().OnParamsInitStarted.Broadcast();
#endif

	TArray<IPlatformFile*> PlatformFiles;

#if !WITH_EDITORONLY_DATA
	IPlatformFile* PakFile = FPlatformFileManager::Get().GetPlatformFile(TEXT("PakFile"));

	// Unreal Engine creates PakFile with null LowerLevel if there are no packs.
	// It causes a crash on access to such File, so we want to avoid that.
	if (PakFile && PakFile->GetLowerLevel())
	{
		PlatformFiles.Add(PakFile);
	}
	else
	{
		UE_LOG(LogParams, Warning, TEXT("FParamsRegistry::Init: ignored an attempt to add invalid or empty PakFile."));
	}
#endif

#if WITH_EDITORONLY_DATA || UE_BUILD_DEVELOPMENT
	PlatformFiles.Add(&IPlatformFile::GetPlatformPhysical());
#endif

	const TArray<FString>& ParamFilesRootPaths = UParamsSettings::Get()->GetParamsRootPaths();

	int32 RefencedParamsCount = CheckParamsRefcounts();

	if (RefencedParamsCount > 0)
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsRegistry::Init: Found %d params with referencecounts != 1."),
			RefencedParamsCount);
	}

	{
		FScopeLock AddParamLock(&AddParamMutex);
		ParamsTree.Empty();
	}

	for (IPlatformFile* PlatformFile : PlatformFiles)
	{
		ReloadParams(PlatformFile, ParamFilesRootPaths);
	}

	GetParamsFromServer();
}

void FParamsRegistry::RequestParamsFromServer()
{
	FailedParamsNumber.Reset();

	if (IsEngineExitRequested())
	{
		UE_LOG(LogParams, Warning, TEXT("FParamsRegistry::RequestParamsFromServer: called when IsEngineExitRequested"));
		return;
	}

	if (ParamsInitialization.AtomicSet(true))
	{
		UE_LOG(
			LogParams,
			Error,
			TEXT("FParamsRegistry::RequestParamsFromServer: called before another initialization has finished"));
		return;
	}

	GetParamsFromServer();
}

void FParamsRegistry::CallWhenInitialized(TFunction<void()> Callback)
{
	if (!ParamsInitialization)
	{
		Callback();
	}
	else
	{
		FScopeLock WhenInitCallbacksLock(&WhenInitCallbacksMutex);
		WhenInitCallbacks.Add(Callback);
	}
}

FParamRegistryDataPtr FParamsRegistry::GetParam(const UScriptStruct* Type, const FName& ID)
{
	if (Get().ParamsInitialization)
	{
		UE_LOG(LogParams, Error, TEXT("FParamsRegistry::GetParam: called before initialization has finished"));
		return nullptr;
	}

	if (TMap<FName, FParamRegistryDataPtr>* Params = Get().ParamsTree.Find(Type))
	{
		FParamRegistryDataPtr* DataRef = Params->Find(ID);

		if (DataRef)
		{
			return *DataRef;
		}
	}
	return nullptr;
}

FParamRegistryDataPtr FParamsRegistry::GetParam(const FParamRegistryInfo& Param)
{
	return GetParam(Param.Type, Param.Name);
}


int32 FParamsRegistry::GetFailedParamsNumber() const
{
	return FailedParamsNumber.GetValue();
}

FText FParamsRegistry::GetRegistryInfoText() const
{
	TArray<UScriptStruct*> TreeKeys;
	ParamsTree.GetKeys(TreeKeys);

	FFormatNamedArguments Args;
	Args.Add(TEXT("TypesNum"), ParamsTree.Num());
	Args.Add(
		TEXT("Types"),
		FText::FromString(FString::JoinBy(
			TreeKeys,
			TEXT("\n"),
			[this](const UScriptStruct* ScriptStruct)
			{
				TArray<FName> Names;
				ParamsTree[ScriptStruct].GetKeys(Names);
				return "- " + ScriptStruct->GetName() + "=" + FString::FromInt(ParamsTree[ScriptStruct].Num()) + "\n"
					+ FString::JoinBy(
						   Names,
						   TEXT("\n"),
						   [](const FName& Name)
						   {
							   return "\t- " + Name.ToString();
						   });
			})));
	return FText::Format(NSLOCTEXT("ParamsRegistry", "RegistryInfo", "Structs={TypesNum}\n{Types}"), Args);
}

TArray<UScriptStruct*> FParamsRegistry::GetRegisteredTypes() const
{
	TArray<UScriptStruct*> RegisteredTypes;
	ParamsTree.GetKeys(RegisteredTypes);
	return RegisteredTypes;
}

TArray<FName> FParamsRegistry::GetRegisteredNames(const UScriptStruct* Type) const
{
	TArray<FName> RegisteredNames;
	if (const TMap<FName, FParamRegistryDataPtr>* RegistryDataAndName = ParamsTree.Find(Type))
		RegistryDataAndName->GetKeys(RegisteredNames);
	return RegisteredNames;
}

#if WITH_EDITOR
FName FParamsRegistry::GetParamVirtualPath(
	const UScriptStruct* Type,
	const FName& Name,
	const TArray<FString>& ParamsRootPaths)
{
	const FParamRegistryDataPtr Param = GetParam(Type, Name);
	if (!Param)
		return NAME_None;

	FString Path = Param->Meta.FilePath;
	const FString VirtualRoot = UParamsSettings::Get()->GetContentBrowserVirtualRoot().ToString();

	for (const FString& ParamsRootPath : ParamsRootPaths)
	{
		if (Path.RemoveFromStart(ParamsRootPath))
		{
			Path = FPaths::Combine(VirtualRoot, Path, Name.ToString());
			return FName(Path);
		}
	}

	return NAME_None;
}

bool FParamsRegistry::CreateAndSaveParam(UScriptStruct* Type, const FName& ID, const FString& InFullPath)
{
	TArray<uint8> Data;
	Data.SetNumUninitialized(Type->GetStructureSize());
	Type->InitializeStruct(Data.GetData());
	int32 CreatedParamIndex = INDEX_NONE;
	if (!SaveParamDataOnDisk(Type, ID, Data.GetData(), InFullPath, CreatedParamIndex))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("CreateAndSaveParam: New param '%s' of type '%s' can't be saved in '%s' "),
			*ID.ToString(),
			*Type->GetName(),
			*InFullPath);
		return false;
	}
	if (!LoadParamFromDisk(Type, ID, InFullPath, CreatedParamIndex))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("CreateAndSaveParam: New param '%s' of type '%s' can't be loaded from '%s' "),
			*ID.ToString(),
			*Type->GetName(),
			*InFullPath);
		return false;
	}

	return true;
}

bool FParamsRegistry::SaveParamOnDisk(UScriptStruct* Type, const FName& ID)
{
	const FParamRegistryDataPtr ParamPtr = GetParam(Type, ID);
	if (ParamPtr == nullptr)
	{
		UE_LOG(
			LogParams,
			Error,
			TEXT("FParamsRegistry::SaveParamOnDisk: getting data for param '%s' type '%s' failed"),
			*ID.ToString(),
			*Type->GetName());
		return false;
	}

	const FParamRegistryMeta* ParamMeta = &ParamPtr->Meta;
	if (ParamMeta == nullptr)
	{
		UE_LOG(
			LogParams,
			Error,
			TEXT("FParamsRegistry::SaveParamOnDisk: getting meta for param '%s' type '%s' failed"),
			*ID.ToString(),
			*Type->GetName());
		return false;
	}
	if (!ParamMeta->bIsSavable)
	{
		UE_LOG(
			LogParams,
			Error,
			TEXT("FParamsRegistry::SaveParamOnDisk: param '%s' type '%s' is not savable"),
			*ID.ToString(),
			*Type->GetName());
		return false;
	}

	const FString& FilePath = ParamMeta->FilePath;
	int32 ParamIndex = ParamMeta->ParamIndex;

	if (!SaveParamDataOnDisk(Type, ID, ParamPtr->Data.GetData(), FilePath, ParamIndex))
	{
		UE_LOG(
			LogParams,
			Error,
			TEXT("FParamsRegistry::SaveParamOnDisk: Can't save param '%s' (type '%s') data on disk "),
			*ID.ToString(),
			*Type->GetName());
		return false;
	}

	ModifyParamChangedState(Type, ID, false);
	return true;
}

bool FParamsRegistry::SaveParamDataOnDisk(
	UScriptStruct* Type,
	const FName& Name,
	const void* ParamData,
	const FString& FilePath,
	int32& IndexInFile)
{
	if (Get().ParamsInitialization)
	{
		UE_LOG(
			LogParams,
			Error,
			TEXT("FParamsRegistry::SaveParamDataOnDisk: called before initialization has finished"));
		return false;
	}

	// Create JSON object of param
	const FParamRegistryInfo Info { Type, Name };
	const TSharedPtr<FJsonObject> JsonHeader = FJsonObjectConverter::UStructToJsonObject(Info);
	if (!JsonHeader.IsValid())
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsRegistry::SaveParamDataOnDisk: Failed to create param header for %s of type %s"),
			*Name.ToString(),
			*Type->GetName());
		return false;
	}
	const TSharedPtr<FJsonObject> JsonData = MakeShareable(new FJsonObject);
	FJsonObjectConverter::CustomExportCallback ExportCb;
	ExportCb.BindStatic(FParamsUtils::CustomExportToJsonCallback);
	if (!FJsonObjectConverter::UStructToJsonObject(Type, ParamData, JsonData.ToSharedRef(), 0, 0, &ExportCb))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsRegistry::SaveParamDataOnDisk: Failed to create param data for %s of type %s"),
			*Name.ToString(),
			*Type->GetName());
		return false;
	}
	const TSharedPtr<FJsonObject> JsonParam = MakeShareable(new FJsonObject);
	JsonParam->SetObjectField(FParamsRegistryLocal::HeaderKey, JsonHeader);
	JsonParam->SetObjectField(FParamsRegistryLocal::DataKey, JsonData);

	// Get and change data from File if it exist
	TArray<TSharedPtr<FJsonValue>> ParamsArray;
	if (FPaths::FileExists(*FilePath))
	{
		if (!FParamsUtils::ReadJsonArrayFromFile(ParamsArray, FilePath))
		{
			UE_LOG(
				LogParams,
				Warning,
				TEXT("FParamsRegistry::SaveParamDataOnDisk: Failed to load param '%s' type '%s' from file"),
				*Name.ToString(),
				*Type->GetName());
			return false;
		}
	}
	if (VerifyParamIndexForArray(IndexInFile, Type, Name, ParamsArray))
	{
		ParamsArray[IndexInFile] = MakeShareable(new FJsonValueObject(JsonParam));
	}
	else
	{
		IndexInFile = ParamsArray.Num();
		ParamsArray.Add(MakeShareable(new FJsonValueObject(JsonParam)));
	}

	// Write changed data to file
	if (!FParamsUtils::WriteJsonArrayToFile(ParamsArray, FilePath))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsRegistry::SaveParamDataOnDisk: Failed to save param '%s' type '%s' to file"),
			*Name.ToString(),
			*Type->GetName());
		return false;
	}

	return true;
}

bool FParamsRegistry::VerifyParamIndexForArray(
	int32& ParamIndex,
	const UScriptStruct* Type,
	const FName& Name,
	const TArray<TSharedPtr<FJsonValue>>& ParamsArray)
{
	auto CheckParamJsonHeader = [Type, Name](TSharedPtr<FJsonValue> ParamJsonValue)
	{
		const TSharedPtr<FJsonObject>* ParamJsonObject = nullptr;
		if (ParamJsonValue != nullptr && ParamJsonValue->TryGetObject(ParamJsonObject))
		{
			const TSharedPtr<FJsonObject>* HeaderJsonObject = nullptr;
			if ((*ParamJsonObject)->TryGetObjectField(FParamsRegistryLocal::HeaderKey, HeaderJsonObject))
			{
				FParamRegistryInfo Info;
				if (FJsonObjectConverter::JsonObjectToUStruct(HeaderJsonObject->ToSharedRef(), &Info))
				{
					if (Info.Name == Name && Info.Type == Type)
						return true;
				}
			}
		}

		return false;
	};

	if (ParamIndex != INDEX_NONE && ParamIndex < ParamsArray.Num() && CheckParamJsonHeader(ParamsArray[ParamIndex]))
		return true;

	for (int32 Index = 0; Index < ParamsArray.Num(); Index++)
	{
		if (Index == ParamIndex)
			continue;

		if (CheckParamJsonHeader(ParamsArray[Index]))
		{
			ParamIndex = Index;
			return true;
		}
	}

	return false;
}

void FParamsRegistry::ModifyParamChangedState(const UScriptStruct* Type, const FName& ID, bool IsChanged)
{
	if (Get().ParamsInitialization)
	{
		UE_LOG(
			LogParams,
			Error,
			TEXT("FParamsRegistry::ModifyParamChangedState: called before initialization has finished"));
		return;
	}

	FParamRegistryDataPtr Param = GetParam(Type, ID);

	if (Param)
	{
		Param->Meta.bIsChanged = IsChanged;
	}
}

bool FParamsRegistry::LoadParamFromDisk(
	UScriptStruct* Type,
	const FName& Name,
	const FString& FilePath,
	int32 IndexInFile)
{
	FString Context = FString::Format(
		TEXT("param '{0}' of type '{1}' from file '{2}'"),
		{ Type->GetName(), Name.ToString(), FilePath });

	if (!FPaths::FileExists(*FilePath))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsRegistry::LoadParamFromDisk: File not found while loading '%s'"),
			*Context);
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> ParamsArray;
	if (!FParamsUtils::ReadJsonArrayFromFile(ParamsArray, FilePath))
	{
		UE_LOG(LogParams, Warning, TEXT("FParamsRegistry::LoadParamFromDisk: Loading file '%s' is failed"), *FilePath);
		return false;
	}

	if (!VerifyParamIndexForArray(IndexInFile, Type, Name, ParamsArray))
	{
		UE_LOG(LogParams, Warning, TEXT("FParamsRegistry::LoadParamFromDisk: Not found '%s'"), *Context);
		return false;
	}

	const FParamRegistryInfo ParamInfo { Type, Name };
	const FParamRegistryMeta ParamMeta { FilePath, IndexInFile, true, false };
	const TSharedPtr<FJsonObject>* ParamJsonObject = nullptr;
	if (!ParamsArray[IndexInFile]->TryGetObject(ParamJsonObject))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsRegistry::LoadParamFromDisk: Getting 'ParamJsonObject' failed while loading '%s'"),
			*Context);
		return false;
	}
	const TSharedPtr<FJsonObject>* ParamDataObject = nullptr;
	if (!(*ParamJsonObject)->TryGetObjectField(FParamsRegistryLocal::DataKey, ParamDataObject))
	{
		UE_LOG(
			LogParams,
			Warning,
			TEXT("FParamsRegistry::LoadParamFromDisk: Getting 'ParamDataObject' failed while loading '%s'"),
			*Context);
		return false;
	}
	if (!Get().AddParam(ParamInfo, &ParamMeta, *ParamDataObject))
	{
		UE_LOG(LogParams, Warning, TEXT("FParamsRegistry::LoadParamFromDisk: Validation failed for  '%s'"), *Context);
		return false;
	}

	Get().OnParamsReloaded.Broadcast();
	return true;
}
#endif

void FParamsRegistry::FinishParamsInitialization()
{
	ParamsInitialization.AtomicSet(false);

	AsyncTask(
		ENamedThreads::GameThread,
		[this]()
		{
			FScopeLock WhenInitCallbacksLock(&WhenInitCallbacksMutex);
			for (auto Callback : WhenInitCallbacks)
			{
				Callback();
			}
			WhenInitCallbacks.Empty();
#if WITH_EDITOR
			OnParamsReloaded.Broadcast();
#endif
		});
}

void FParamsRegistry::GetParamsFromServer()
{
	if (UParamsSettings::Get()->EnableParamsServer)
	{
		FRequestParamsFromServerTask::RequestParamsFromServer();
	}
	else
	{
		FinishParamsInitialization();
	}
}

int32 FParamsRegistry::CheckParamsRefcounts()
{
	int32 ParamsCount = 0;

	for (const auto& DataTypeMap : ParamsTree)
	{
		for (const auto& Param : DataTypeMap.Value)
		{
			if (Param.Value)
			{
				const int32 ReferenceCount = Param.Value.GetSharedReferenceCount();

				if (ReferenceCount != 1)
				{
					ParamsCount++;

					UE_LOG(
						LogParams,
						Warning,
						TEXT(
							"FParamsRegistry::CheckParamsRefcounts: found param with multiple referencecount: %d. Should be 1. %s."),
						ReferenceCount,
						*Param.Value->Info.Name.ToString());
				}
			}
		}
	}

	return ParamsCount;
}

void FParamsRegistry::ReloadParams(IPlatformFile* PlatformFile, const TArray<FString>& ParamsPaths)
{
	TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*(FString("FParamsRegistry::ReloadParams [") + PlatformFile->GetName() + "]"));

	FailedParamsNumber.Reset();
	UsedInstancedObjectsPtrs.Empty();

	TArray<FString> Filenames;
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(
			*(FString("FParamsRegistry::ReloadParams [") + PlatformFile->GetName() + "]: SearchFilesRecursive"));
		for (const FString& ParamsPath : ParamsPaths)
		{
			FParamsUtils::SearchFilesRecursive(
				PlatformFile,
				ParamsPath,
				UParamsSettings::Get()->ParamFileNameWildcard,
				Filenames);
		}
	}

	TArray<FJsonDataWithMeta> DataWithContexts;
	FCriticalSection AddDataMutex;

	auto HandleFile = [&](int32 Index)
	{
		TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*(FString("FParamsRegistry::HandleFile [") + PlatformFile->GetName() + "]"));
		const FString& FilePath = Filenames[Index];
		const FString& Filename = *FPaths::GetBaseFilename(FilePath);
		TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(*(
			FString("FParamsRegistry::HandleFile [") + PlatformFile->GetName() + "]: '" + Filename + "' " + FilePath));

		FString JsonString;
		JsonString.Reserve(2048);
		FFileHelper::LoadFileToString(JsonString, PlatformFile, *FilePath);

		FParamsUtils::LoadJsonFromString(JsonString, DataWithContexts, FilePath, &AddDataMutex);
	};

	ParallelFor(Filenames.Num(), HandleFile, EParallelForFlags::Unbalanced);

	AddParamsFromJsonObjects(DataWithContexts, true);
}

bool FParamsRegistry::AddParam(
	const FParamRegistryInfo& ParamInfo,
	const FParamRegistryMeta* ParamMeta,
	TSharedPtr<FJsonObject> JsonData)
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FParamsRegistry::AddParam);
	bool bRes = false;

	TArray<uint8> Data;

	if (FParamsUtils::FillDataFromJson(ParamInfo.Type, Data, JsonData))
	{
		const FString CPPClassName =
			FString::Printf(TEXT("%s%s"), ParamInfo.Type->GetPrefixCPP(), *ParamInfo.Type->GetName());

		if (ParamsValidation::FParamsValidationManager::Get().ValidateParam(CPPClassName, Data.GetData()))
		{
			FScopeLock AddParamLock(&AddParamMutex);

			TMap<FName, FParamRegistryDataPtr>& DataTypeMap = ParamsTree.FindOrAdd(ParamInfo.Type);
			DataTypeMap.Remove(ParamInfo.Name);

			FParamRegistryDataPtr* Param = &DataTypeMap.Add(ParamInfo.Name);
			*Param = MakeShared<FParamRegistryData>();
			(*Param)->Info = ParamInfo;

#if WITH_EDITOR
			if (ParamMeta != nullptr)
			{
				(*Param)->Meta = *ParamMeta;
			}
#endif
			(*Param)->Data = Data;

			SavePointersToInstancedObjects(*Param);

			bRes = true;
		}
		else
		{
			FailedParamsNumber.Increment();
			UE_LOG(
				LogParams,
				Warning,
				TEXT("AddParam: validation failed for object: Name = %s, Type = %s."),
				*ParamInfo.Name.ToString(),
				*CPPClassName);
			bRes = false;
		}
	}

	return bRes;
}

void FParamsRegistry::AddParamsFromJsonObjects(const TArray<FJsonDataWithMeta>& DataWithContexts, bool IsDataFromDisk)
{
#if WITH_EDITOR
	TMap<UScriptStruct*, TMap<FName, FString>> UniqueTypeToNameMap;
#endif

	auto HandleObject = [this,
						 &DataWithContexts,
						 IsDataFromDisk
#if WITH_EDITOR
						 ,
						 &UniqueTypeToNameMap
#endif
	](int32 Index)
	{
		const FJsonDataWithMeta& DataWithContext = DataWithContexts[Index];
		const FString Context = DataWithContext.GetContext();
		const TSharedPtr<FJsonObject> JsonObject = DataWithContext.Data;

		TRACE_CPUPROFILER_EVENT_SCOPE_TEXT(
			*(FString("FParamsRegistry::AddParamsFromJsonObjects [") + Context + "]: HandleObject"));

		const TSharedPtr<FJsonObject>* HeaderObject = nullptr;
		if (!JsonObject->TryGetObjectField(FParamsRegistryLocal::HeaderKey, HeaderObject))
		{
			UE_LOG(LogParams, Warning, TEXT("Init: Can't read 'header' field from JSON: Context='%s'"), *Context)
			return;
		}
		FParamRegistryInfo Info;
		if (!FJsonObjectConverter::JsonObjectToUStruct(HeaderObject->ToSharedRef(), &Info))
		{
			UE_LOG(
				LogParams,
				Warning,
				TEXT("Init: Can't convert JSON 'header' object to struct: Context='%s'"),
				*Context)
			return;
		}
		if (Info.Name.IsNone() || !Info.Name.IsValid())
		{
			UE_LOG(LogParams, Warning, TEXT("Init: Name is invalid or None! Context='%s'"), *Context)
			return;
		}
		if (!IsValid(Info.Type))
		{
			UE_LOG(
				LogParams,
				Warning,
				TEXT("Init: Invalid type! Name='%s', Context='%s'"),
				*Info.Name.ToString(),
				*Context)
			return;
		}

		const FParamRegistryMeta* MetaPtr = nullptr;
#if WITH_EDITOR
		FParamRegistryMeta Meta;
		Meta.bIsSavable = IsDataFromDisk;
		Meta.FilePath = DataWithContext.ContextualPath;
		Meta.ParamIndex = DataWithContext.ContextualIndex;
		MetaPtr = &Meta;
		{
			FScopeLock EditorParamUniqueNameCheck(&EditorParamUniqueNameCheckMutex);

			TMap<FName, FString>& UniqueNames = UniqueTypeToNameMap.FindOrAdd(Info.Type);
			if (const FString* OriginalPath = UniqueNames.Find(Info.Name))
			{
				FailedParamsNumber.Increment();
				UE_LOG(
					LogParams,
					Error,
					TEXT(
						"Init: Name dublication found! Name='%s' Struct='%s' Path='%s'. Already loaded struct path: '%s', Context='%s'"),
					*Info.Name.ToString(),
					*Info.Type->GetName(),
					*Context,
					**OriginalPath,
					*Context)
				return;
			}
			UniqueNames.Add(Info.Name, Context);
		}
#endif

		const TSharedPtr<FJsonObject>* DataObject = nullptr;
		if (!JsonObject->TryGetObjectField(FParamsRegistryLocal::DataKey, DataObject))
		{
			FailedParamsNumber.Increment();

			UE_LOG(
				LogParams,
				Warning,
				TEXT("Init: Can't read 'data' field from JSON: Name='%s', Type='%s', Context='%s'"),
				*Info.Name.ToString(),
				*Info.Type->GetName(),
				*Context)
			return;
		}

		if (!AddParam(Info, MetaPtr, *DataObject))
		{
			UE_LOG(
				LogParams,
				Warning,
				TEXT("Init: can't add parameter: Name='%s', Type='%s', Context='%s'. It's not added to registry."),
				*Info.Name.ToString(),
				*Info.Type->GetName(),
				*Context);
			return;
		}
	};

	ParallelFor(DataWithContexts.Num(), HandleObject, EParallelForFlags::ForceSingleThread);
}

void FParamsRegistry::SavePointersToInstancedObjects(FParamRegistryDataPtr Param)
{
	const UScriptStruct* Type = Param->Info.Type;
	const void* Data = Param->Data.GetData();

	for (TPropertyValueIterator<FProperty> It(Type, Data); It; ++It)
	{
		FProperty* Property = It->Key;
		const void* PropertyData = It->Value;

		TraverseProperty(Property, PropertyData);
	}
}

void FParamsRegistry::TraverseArrayProperty(const FArrayProperty* Property, const void* Data)
{
	FScriptArrayHelper ArrayHelper(Property, Data);

	for (int32 Index = 0; Index < ArrayHelper.Num(); Index++)
	{
		const void* ElementData = ArrayHelper.GetRawPtr(Index);
		if (ElementData)
			TraverseProperty(Property->Inner, ElementData);
	}
}

void FParamsRegistry::TraverseMapProperty(const FMapProperty* Property, const void* Data)
{
	FScriptMapHelper Helper(Property, Data);

	for (int32 Index = 0; Index < Helper.Num(); Index++)
	{
		const void* ElementData = Helper.GetValuePtr(Index);
		if (ElementData)
			TraverseProperty(Property->ValueProp, ElementData);
	}
}

void FParamsRegistry::TraverseSetProperty(const FSetProperty* Property, const void* Data)
{
	const FScriptSetHelper Helper(Property, Data);

	for (int32 Index = 0; Index < Helper.Num(); Index++)
	{
		const void* ElementData = Helper.GetElementPtr(Index);
		if (ElementData)
			TraverseProperty(Property->ElementProp, ElementData);
	}
}

void FParamsRegistry::TraverseProperty(const FProperty* Property, const void* Data)
{
	if (const FArrayProperty* ArrayProperty = CastField<const FArrayProperty>(Property))
	{
		TraverseArrayProperty(ArrayProperty, Data);
	}
	else if (const FObjectProperty* ObjectProperty = CastField<const FObjectProperty>(Property))
	{
		const UObject* Object = ObjectProperty->GetObjectPropertyValue(Data);
		if (Object)
		{
			TraverseProperties(Object->GetClass(), Object);
			UsedInstancedObjectsPtrs.Push(TStrongObjectPtr<const UObject>(Object));
		}
	}
	else if (const FStructProperty* StructProperty = CastField<const FStructProperty>(Property))
	{
		UClass* Class = StructProperty->Struct->GetClass();
		if (Class && Data)
			TraverseProperties(Class, Data);
	}
	else if (const FMapProperty* MapProperty = CastField<const FMapProperty>(Property))
	{
		TraverseMapProperty(MapProperty, Data);
	}
	else if (const FSetProperty* SetProperty = CastField<const FSetProperty>(Property))
	{
		TraverseSetProperty(SetProperty, Data);
	}
}

void FParamsRegistry::TraverseProperties(const UClass* Class, const void* Data)
{
	for (const FProperty* Property = Class->PropertyLink; Property != nullptr; Property = Property->PropertyLinkNext)
		TraverseProperty(Property, Property->ContainerPtrToValuePtr<const void*>(Data));
}
