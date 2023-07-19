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
#include "VSPSystemMonitorRunnable.h"
#include "Trace/Trace.inl"


#if PLATFORM_WINDOWS
#include "WindowsSystemMonitor.h"
#endif

#if PLATFORM_LINUX
#include "LinuxSystemMonitor.h"
#endif

DECLARE_LOG_CATEGORY_EXTERN(LogVSPSystemMonitorRunnable, Log, All);
DEFINE_LOG_CATEGORY(LogVSPSystemMonitorRunnable);

//VSPHardwareMonitor
UE_TRACE_EVENT_BEGIN(Cpu, VSPUE_HardwareData, NoSync)
	UE_TRACE_EVENT_FIELD(double, CpuLoad)
	UE_TRACE_EVENT_FIELD(uint64, AvailableRamMb)
	UE_TRACE_EVENT_FIELD(double, HddLoad1)
	UE_TRACE_EVENT_FIELD(double, HddLoad2)
	UE_TRACE_EVENT_FIELD(double, HddLoad3)
	UE_TRACE_EVENT_FIELD(double, HddLoad4)
	UE_TRACE_EVENT_FIELD(double, HddLoad5)
UE_TRACE_EVENT_END()

// ~60fps
constexpr float GTimeout_Sec { 0.016f };

FVSPSystemMonitorRunnable::FVSPSystemMonitorRunnable() : FRunnable {}, bThreadInProcess { false }, SystemMonitor { nullptr }
{
	Thread.Reset(FRunnableThread::Create(this, TEXT("VSPSystemMonitorRunnable")));
}

bool FVSPSystemMonitorRunnable::Init()
{
	bThreadInProcess = true;
#if PLATFORM_LINUX
	SystemMonitor.Reset(new FLinuxSystemMonitor());
#endif
#if PLATFORM_WINDOWS
	SystemMonitor.Reset(new FWindowsSystemMonitor());
#endif

	if (!SystemMonitor->Init())
	{
		UE_LOG(LogVSPSystemMonitorRunnable, Error, TEXT("FVSPSystemMonitorRunnable::Init. Error on init SystemMonitor"));
		return false;
	}
	return FRunnable::Init();
}

uint32 FVSPSystemMonitorRunnable::Run()
{
	TArray<double> HddsLoad;
	HddsLoad.SetNum(SystemMonitor->GetDiskCount());
	while (bThreadInProcess)
	{
		if (!SystemMonitor->Update())
		{
#ifdef VSP_SYSTEM_MONITOR_DEBUG
			UE_LOG(LogVSPSystemMonitorRunnable, VeryVerbose, TEXT("FVSPSystemMonitorRunnable. Not updated!"));
#endif
		}
		else
		{
			for (int32 i = 0; i < HddsLoad.Num(); ++i)
			{
				FDiskUsage DiskUsage;
				SystemMonitor->GetDiskUsagePercent(DiskUsage, i);
				HddsLoad[i] = DiskUsage.GetUsagePercent();
			}

			UE_TRACE_LOG_SCOPED_T(Cpu, VSPUE_HardwareData, VSPUEChannel)
				<< VSPUE_HardwareData.CpuLoad(SystemMonitor->GetCpuUtilizationPercent())
				<< VSPUE_HardwareData.AvailableRamMb(SystemMonitor->GetAvailableRamMb())
				<< VSPUE_HardwareData.HddLoad1(HddsLoad[0])
				<< VSPUE_HardwareData.HddLoad2(HddsLoad[1])
				<< VSPUE_HardwareData.HddLoad3(HddsLoad[2])
				<< VSPUE_HardwareData.HddLoad4(HddsLoad[3])
				<< VSPUE_HardwareData.HddLoad5(HddsLoad[4]);

#ifdef VSP_SYSTEM_MONITOR_DEBUG
			UE_LOG(
				LogVSPSystemMonitorRunnable,
				VeryVerbose,
				TEXT("CPU: %f, RAM: %lld, Disk usage: %f"),
				SystemMonitor->GetCpuUtilizationPercent(),
				SystemMonitor->GetAvailableRamMb(),
				HddsLoad[0]);
#endif

		}
		FPlatformProcess::Sleep(GTimeout_Sec);
	}
	return 0;
}

FVSPSystemMonitorRunnable::~FVSPSystemMonitorRunnable()
{
	bThreadInProcess = false;
	Thread->Kill(true);
}
