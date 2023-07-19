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
#include "DiskUsage.h"

namespace Local_DiskUsage
{
	const FString NONE_NAME{"Unknown"};
	constexpr int32 NONE_VALUE{-1};
}

FDiskUsage::FDiskUsage() : Name{Local_DiskUsage::NONE_NAME}, UsagePercent{Local_DiskUsage::NONE_VALUE}
{
}

FDiskUsage::FDiskUsage(const FString& Name) : Name{Name}, UsagePercent{Local_DiskUsage::NONE_VALUE}
{
}

FString FDiskUsage::GetName() const
{
	return Name;
}

double FDiskUsage::GetUsagePercent() const
{
	return UsagePercent;
}

void FDiskUsage::SetName(const FString& Value)
{
	this->Name = Value;
}

void FDiskUsage::SetUsagePercent(const double Value)
{
	this->UsagePercent = Value;
}
