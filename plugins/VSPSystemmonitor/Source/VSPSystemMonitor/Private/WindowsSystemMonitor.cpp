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
#include "WindowsSystemMonitor.h"

#include "HAL/PlatformFilemanager.h"
#include "GenericPlatform/GenericPlatformFile.h"

DECLARE_LOG_CATEGORY_EXTERN(LogWindowsSystemMonitor, Log, All);

DEFINE_LOG_CATEGORY(LogWindowsSystemMonitor);

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define _WIN32_WINDOWS 0x0410
#include <Pdh.h>
#pragma comment(lib, "pdh.lib")
#endif

FWindowsSystemMonitor::FWindowsSystemMonitor() :
FBaseSystemMonitor{},
Query{nullptr},
CpuCounter{nullptr},
MemoryCounter{nullptr}
{
}

FWindowsSystemMonitor::~FWindowsSystemMonitor()
{
#if PLATFORM_WINDOWS
	PdhCloseQuery(Query);
#endif
}

bool FWindowsSystemMonitor::Init()
{
#if PLATFORM_WINDOWS
	if (!bIsInitialized)
	{
		PDH_STATUS Status = PdhOpenQuery(NULL, NULL, &Query);
		if (Status != ERROR_SUCCESS)
		{
			Dispose();
#ifdef VSP_SYSTEM_MONITOR_DEBUG
			UE_LOG(LogWindowsSystemMonitor, Error, TEXT("Can't create PDH query"));
#endif
			return false;
		}
		Status = PdhAddEnglishCounter(Query,TEXT("\\Processor Information(_Total)\\% Processor Time"),NULL, &CpuCounter);
		if (Status != ERROR_SUCCESS)
		{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
			UE_LOG(LogWindowsSystemMonitor, Error, TEXT("Error on add CPU counter"));
#endif
			Dispose();
			return false;
		}
		Status = PdhAddEnglishCounter(Query,TEXT("\\Memory\\Available MBytes"),NULL, &MemoryCounter);
		if (Status != ERROR_SUCCESS)
		{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
			UE_LOG(LogWindowsSystemMonitor, Error, TEXT("Error on add RAM counter"));
#endif
			Dispose();
			return false;
		}
		auto DrivesNames = GetDrivesNames();
		if (DrivesNames.Num() == 0)
		if (DrivesNames.Num() == 0)
		{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
			UE_LOG(LogWindowsSystemMonitor, Error, TEXT("HDD not found"));
#endif
			Dispose();
			return false;
		}
		int32 DiskCounter = 0;
		TArray<FString> DrivesCountersNames = GetDrivesUsageCountersNames(DrivesNames);
		for (int32 i = 0; i < DrivesNames.Num(); ++i)
		{
			HANDLE Counter = nullptr;
			Status = PdhAddEnglishCounter(
				Query,
				TCHAR_TO_WCHAR(*DrivesCountersNames[i]),
				NULL,
				&Counter);
			if (Status != ERROR_SUCCESS)
			{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
				UE_LOG(LogWindowsSystemMonitor, Warning, TEXT("Error on add drive counter: %s"), *DrivesCountersNames[i]);
#endif
			}
			else
			{
				if (DiskCounter < DiskUsages.Num())
				{
					DiskUsages[DiskCounter] = DrivesNames[i];
				}
				else
				{
					DiskUsages.Emplace(DrivesNames[i]);
				}
				DiskCounter++;
				DrivesCounters.Add(Counter);
			}
		}
		bIsInitialized = true;
		return true;
	}
#ifdef VSP_SYSTEM_MONITOR_DEBUG
	UE_LOG(LogWindowsSystemMonitor, Warning, TEXT("WindowsSystemMonitor is not initialized"));
#endif

#endif

	return false;
}

bool FWindowsSystemMonitor::IsInitialized() const
{
	return bIsInitialized;
}

bool FWindowsSystemMonitor::Update()
{
#if PLATFORM_WINDOWS
	if (bIsInitialized)
	{
		PdhCollectQueryData(Query);
		PDH_FMT_COUNTERVALUE PdhValue;
		DWORD DwValue;

		PDH_STATUS Status = PdhGetFormattedCounterValue(CpuCounter, PDH_FMT_DOUBLE, &DwValue, &PdhValue);
		if (Status != ERROR_SUCCESS)
		{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
			UE_LOG(LogWindowsSystemMonitor, VeryVerbose, TEXT("Error on get CPU usage"));
#endif
			return false;
		}
		CpuUtilizationPercent = PdhValue.doubleValue;

		Status = PdhGetFormattedCounterValue(MemoryCounter, PDH_FMT_LONG, &DwValue, &PdhValue);
		if (Status != ERROR_SUCCESS)
		{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
			UE_LOG(LogWindowsSystemMonitor, VeryVerbose, TEXT("Error on get available RAM"));
#endif
			return false;
		}
		AvailableRamMb = PdhValue.longValue;

		for (int32 i = 0; i < DrivesCounters.Num(); ++i)
		{
			Status = PdhGetFormattedCounterValue(DrivesCounters[i], PDH_FMT_DOUBLE, &DwValue, &PdhValue);
			if (Status == ERROR_SUCCESS)
			{
				DiskUsages[i].SetUsagePercent(PdhValue.doubleValue);
			}
		}
		return true;
	}
#endif

	return false;
}

void FWindowsSystemMonitor::Dispose()
{
	bIsInitialized = false;
#if PLATFORM_WINDOWS
	PdhCloseQuery(Query);
#endif
}

TArray<FString> FWindowsSystemMonitor::GetDrivesNames()
{
	TArray<FString> DrivesNames;
	FString Letters = TEXT("CDEFGHIJKLMNOPQRSTUVWXYZ");
	for (int32 i = 0; i < Letters.Len(); i++)
	{
		FString DriveName = FString(1, (&Letters.GetCharArray()[i]));
		FString Drive = DriveName + TEXT(":/");
		const FFileStatData Data = FPlatformFileManager::Get().GetPlatformFile().GetStatData(*Drive);
		if (Data.bIsDirectory)
		{
			DrivesNames.Add(DriveName);
		}
	}
	return DrivesNames;
}

TArray<FString> FWindowsSystemMonitor::GetDrivesUsageCountersNames(const TArray<FString>& DrivesNames)
{
	TArray<FString> DrivesCountersNames;
	int32 CounterIndex = 0;
	for (const FString &DriveName : DrivesNames)
	{
		FString CounterName = "\\PhysicalDisk(";
		CounterName.AppendInt(CounterIndex++);
		CounterName.Append(" ");
		CounterName.Append(DriveName);
		CounterName.Append(":)\\% Disk Time");
		DrivesCountersNames.Add(CounterName);
	}
	return DrivesCountersNames;
}
