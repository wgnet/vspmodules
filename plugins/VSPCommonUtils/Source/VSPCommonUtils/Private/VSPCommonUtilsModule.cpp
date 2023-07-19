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
#include "VSPCommonUtilsModule.h"

/*
#ifndef VSP_DEBUG_ENABLED
	#error VSP_DEBUG_ENABLED definition missing, check *.Target.cs
#endif

#ifVSP_DEBUG_ENABLED != !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	#error VSP_DEBUG_ENABLED is expected to be 0 in UE_BUILD_SHIPPING or UE_BUILD_TEST
#endif

#ifndef VSP_CHEATS_ENABLED
	#error VSP_CHEATS_ENABLED definition missing, check *.Target.cs
#endif

#ifVSP_CHEATS_ENABLED != !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
	#error VSP_CHEATS_ENABLED is expected to be 0 in UE_BUILD_SHIPPING or UE_BUILD_TEST
#endif
*/
void FVSPCommonUtilsModule::StartupModule()
{
}

void FVSPCommonUtilsModule::ShutdownModule()
{
}

// Empty module class is used intentionally as a template for future plugin expansion.
IMPLEMENT_MODULE(FVSPCommonUtilsModule, VSPCommonUtils)
