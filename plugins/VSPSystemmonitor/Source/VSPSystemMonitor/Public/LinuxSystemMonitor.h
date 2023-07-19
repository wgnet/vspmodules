﻿/*
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

#include "BaseSystemMonitor.h"

struct FCpuStats;

class FLinuxSystemMonitor: public FBaseSystemMonitor
{
public:
	FLinuxSystemMonitor();
	virtual ~FLinuxSystemMonitor() override;

	virtual bool Init() override;
	virtual bool IsInitialized() const override;
	virtual bool Update() override;
	virtual void Dispose() override;
private:
	TUniquePtr<FCpuStats> CurrentCpuStat;

	bool ComputeCpuUsage();
	bool ComputeAvailableRam();
	bool ComputeDiskUsage();
};
