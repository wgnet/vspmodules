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
#include "ProjectVSPVivox.h"

#include "VSPVivox.h"


FProjectVSPVivox* FProjectVSPVivox::Get()
{
	auto This = FModuleManager::GetModulePtr<FProjectVSPVivox>("ProjectVSPVivox");
	VSPvCheck(This);
	return This;
}

UVSPVivox* FProjectVSPVivox::GetVSPVivox()
{
	auto This = Get();
	VSPvCheckReturn(This, nullptr);
	return This->VSPVivox;
}

void FProjectVSPVivox::StartupModule()
{
	VSPV_LOG(Log, "");

	VSPvCheckReturn(!VSPVivox);
	VSPVivox = NewObject<UVSPVivox>();
	VSPVivox->AddToRoot();
}

void FProjectVSPVivox::ShutdownModule()
{
	VSPV_LOG(Log, "");
}

DEFINE_LOG_CATEGORY(VSPVivoxLog);
DEFINE_LOG_CATEGORY(VSPChatsManagingLog);

IMPLEMENT_MODULE(FProjectVSPVivox, ProjectVSPVivox)
