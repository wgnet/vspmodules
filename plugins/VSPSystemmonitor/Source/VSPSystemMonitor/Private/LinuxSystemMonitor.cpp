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
#include "LinuxSystemMonitor.h"
#include "Internationalization/Regex.h"

#if PLATFORM_LINUX
#include <sys/statvfs.h>
#endif

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fstream>
#include <string>

DECLARE_LOG_CATEGORY_EXTERN(LogLinuxSystemMonitor, Log, All);

DEFINE_LOG_CATEGORY(LogLinuxSystemMonitor);

struct FCpuStats
{
	int64 User{0};
	int64 Nice{0};
	int64 System{0};
	int64 Idle{0};
	int64 IoWait{0};
	int64 Irq{0};
	int64 SoftIrq{0};
	int64 Steal{0};
	int64 Guest{0};
	int64 GuestNice{0};

	int64 GetTotalIdle() const
	{
		return Idle + IoWait;
	}

	int64 GetTotalActive() const
	{
		return User + Nice + System + Irq + SoftIrq + Steal + Guest + GuestNice;
	}

	void Update(const FCpuStats &Value)
	{
		this->User = Value.User;
		this->Nice = Value.Nice;
		this->System = Value.System;
		this->Idle = Value.Idle;
		this->IoWait = Value.IoWait;
		this->Irq = Value.Irq;
		this->SoftIrq = Value.SoftIrq;
		this->Steal = Value.Steal;
		this->Guest = Value.Guest;
		this->GuestNice = Value.GuestNice;
	}

	static double ComputeCpuUsage(const FCpuStats &First, const FCpuStats &Second)
	{
		const int64 ActiveTime = Second.GetTotalActive() - First.GetTotalActive();
		const int64 IdleTime = Second.GetTotalIdle() - First.GetTotalIdle();
		const double TotalTime = static_cast<double>(ActiveTime) + static_cast<double>(IdleTime);
		return 100 * (ActiveTime / TotalTime);
	}
};

FLinuxSystemMonitor::FLinuxSystemMonitor():
FBaseSystemMonitor{},
CurrentCpuStat{MakeUnique<FCpuStats>()}
{}

FLinuxSystemMonitor::~FLinuxSystemMonitor() {}

bool FLinuxSystemMonitor::Init()
{
	bIsInitialized = true;
	return bIsInitialized;
}

bool FLinuxSystemMonitor::IsInitialized() const
{
	return bIsInitialized;
}

bool FLinuxSystemMonitor::Update()
{
	return ComputeCpuUsage() && ComputeAvailableRam() && ComputeDiskUsage();
}

void FLinuxSystemMonitor::Dispose()
{}

bool FLinuxSystemMonitor::ComputeCpuUsage()
{
	const FString FPathStat{"/proc/stat"};

	const FRegexPattern PatternAvgCpuInfo(TEXT(R"(cpu\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+)\s+(\d+))"));
	std::ifstream InFile(TCHAR_TO_UTF8(*FPathStat));

	if (InFile.is_open())
	{
		std::string Line;
		if (std::getline(InFile, Line))
		{
			const FString AvgCpuInfo = UTF8_TO_TCHAR(Line.c_str());
			FRegexMatcher Matcher(PatternAvgCpuInfo, AvgCpuInfo);
			if (Matcher.FindNext())
			{
				FCpuStats CpuStats;
				CpuStats.User = FCString::Atoi64(*Matcher.GetCaptureGroup(1));
				CpuStats.Nice = FCString::Atoi64(*Matcher.GetCaptureGroup(2));
				CpuStats.System = FCString::Atoi64(*Matcher.GetCaptureGroup(3));
				CpuStats.Idle = FCString::Atoi64(*Matcher.GetCaptureGroup(4));
				CpuStats.IoWait = FCString::Atoi64(*Matcher.GetCaptureGroup(5));
				CpuStats.Irq = FCString::Atoi64(*Matcher.GetCaptureGroup(6));
				CpuStats.SoftIrq = FCString::Atoi64(*Matcher.GetCaptureGroup(7));
				CpuStats.Steal = FCString::Atoi64(*Matcher.GetCaptureGroup(8));
				CpuStats.Guest = FCString::Atoi64(*Matcher.GetCaptureGroup(9));
				CpuStats.GuestNice = FCString::Atoi64(*Matcher.GetCaptureGroup(10));
				CpuUtilizationPercent = FCpuStats::ComputeCpuUsage(*CurrentCpuStat, CpuStats);
				CurrentCpuStat->Update(CpuStats);

				return true;
			}
			else
			{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
				UE_LOG(LogLinuxSystemMonitor, VeryVerbose, TEXT("ComputeCpuUsage. %s not matched"), *AvgCpuInfo );
#endif
				return false;
			}
		}
	}
#ifdef VSP_SYSTEM_MONITOR_DEBUG
	UE_LOG(LogLinuxSystemMonitor, VeryVerbose, TEXT("ComputeCpuUsage. Can't open file %s"), *FPathStat);
#endif
	return false;
}

bool FLinuxSystemMonitor::ComputeAvailableRam()
{
	const FString FPathMemInfo{"/proc/meminfo"};

	const FRegexPattern PatternAvailableRam(TEXT(R"(MemTotal:\s+(\d+))"));

	std::ifstream InFile(TCHAR_TO_UTF8(*FPathMemInfo));
	if (InFile.is_open())
	{
		std::string Line;
		if (std::getline(InFile, Line))
		{
			const FString MemInfo = UTF8_TO_TCHAR(Line.c_str());
			FRegexMatcher Matcher(PatternAvailableRam, MemInfo);
			if (Matcher.FindNext())
			{
				AvailableRamMb = FCString::Atoi64(*Matcher.GetCaptureGroup(1)) / 1024;
				return true;
			}
			else
			{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
				UE_LOG(LogLinuxSystemMonitor, VeryVerbose, TEXT("ComputeAvailableRam. %s not matched"), *MemInfo );
#endif
				return false;
			}
		}
	}
#ifdef VSP_SYSTEM_MONITOR_DEBUG
	UE_LOG(LogLinuxSystemMonitor, VeryVerbose, TEXT("ComputeAvailableRam. Can't open file %s"), *FPathMemInfo);
#endif
	return false;
}

bool FLinuxSystemMonitor::ComputeDiskUsage()
{
	constexpr int32 UNKNOWN_VALUE{-1};
	const FString FPathDiskUsage{"/proc/mounts"};
	const FRegexPattern PatternDiskUsage(TEXT("(/dev/sd\\w)\\s+(/.*)+\\s+(\\w+)\\s([\\w|,|=|-]+)\\s+(\\d)+\\s+(\\d)+"));
	std::ifstream InFile(TCHAR_TO_UTF8(*FPathDiskUsage));
	if (InFile.is_open())
	{
		std::string Line;
		int32 Counter = 0;
		while (std::getline(InFile, Line))
		{
			const FString MountLine = UTF8_TO_TCHAR(Line.c_str());
			FRegexMatcher Matcher(PatternDiskUsage, MountLine);
			if (Matcher.FindNext())
			{
				auto Result = UNKNOWN_VALUE;
				const auto DiscName = Matcher.GetCaptureGroup(1);
				const auto DiscPath = Matcher.GetCaptureGroup(2);
#if PLATFORM_LINUX
				const auto DiscPathData = TCHAR_TO_UTF8(*DiscPath);
				struct statvfs DiskData;
				statvfs(DiscPathData, &DiskData);

				const auto Total = DiskData.f_blocks;
				const auto Free = DiskData.f_bfree;
				const auto Diff = Total - Free;
				Result = 100 * static_cast<double>(Diff) / Total;
#endif
				if (Counter < DiskUsages.Num())
				{
					DiskUsages[Counter].SetName(DiscName);
				}
				else
				{
					DiskUsages.Emplace(FDiskUsage(DiscName));
				}
				DiskUsages[Counter].SetUsagePercent(Result);
				Counter++;
			}
			else
			{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
				UE_LOG(LogLinuxSystemMonitor, VeryVerbose, TEXT("ComputeDiskUsage. %s not matched"), *MountLine );
#endif
			}
		}
		return true;
	}
#ifdef VSP_SYSTEM_MONITOR_DEBUG
	UE_LOG(LogLinuxSystemMonitor, VeryVerbose, TEXT("ComputeDiskUsage. Can't open file %s"), *FPathDiskUsage);
#endif
	return false;
}
