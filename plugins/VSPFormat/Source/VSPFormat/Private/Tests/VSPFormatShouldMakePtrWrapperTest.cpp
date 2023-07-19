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
#include "VSPFormat.h"

#include "CoreMinimal.h"
#include "CoreUObject/Public/UObject/Object.h"

static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<TCHAR*>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<TCHAR*&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<TCHAR*&&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const TCHAR*>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const TCHAR*&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const TCHAR*&&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<volatile TCHAR*&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const volatile TCHAR*&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const volatile TCHAR*&&>::Value, "");

// cv pointers to cv types
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<TCHAR* const>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<TCHAR* const&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<TCHAR* const&&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const TCHAR* const volatile>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const TCHAR* const&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<volatile TCHAR* const&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const volatile TCHAR* const volatile&>::Value, "");

static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<void*>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<void*&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<void*&&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<volatile void*&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<volatile void*&&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const void*&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const void*&&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const volatile void*&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const volatile void*&&>::Value, "");

static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<UObject*>::Value, "");
static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<UObject*&>::Value, "");
static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<UObject*&&>::Value, "");
static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<volatile UObject*&>::Value, "");
static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<volatile UObject*&&>::Value, "");
static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const UObject*&>::Value, "");
static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const UObject*&&>::Value, "");
static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const volatile UObject*&>::Value, "");
static_assert(_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const volatile UObject*&&>::Value, "");

static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<FString>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<FString&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<const FString&>::Value, "");
static_assert(!_Detail_V_S_P_F_M_T::TShouldMakePointerWrapper<FString&&>::Value, "");
