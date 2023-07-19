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

#include "VSPCheckDetails.h"


// -- VSPCheck*CF ------------------------
// custom message and category

// simple
#define VSPCheckCF(Condition, Category, UserMessage) _VSPCheckCF(Condition, Category, UserMessage)
#define VSPNoEntryCF(Category, UserMessage) VSPCheckCF(false, Category, UserMessage)

// return
#define VSPCheckReturnCF(Condition, Category, UserMessage, ...) \
	_VSPCheckReturnCF(Condition, Category, UserMessage, ##__VA_ARGS__)
#define VSPNoEntryReturnCF(Category, UserMessage, ...) VSPCheckReturnCF(false, Category, UserMessage, ##__VA_ARGS__)

// continue
#define VSPCheckContinueCF(Condition, Category, UserMessage) _VSPCheckContinueCF(Condition, Category, UserMessage)
#define VSPNoEntryContinueCF(Category, UserMessage) VSPCheckContinueCF(false, Category, UserMessage)

// break
#define VSPCheckBreakCF(Condition, Category, UserMessage) _VSPCheckBreakCF(Condition, Category, UserMessage)
#define VSPNoEntryBreakCF(Category, UserMessage) VSPCheckBreakCF(false, Category, UserMessage)

// -- VSPCheck*F ------------------------
// custom message and default category

// simple
#define VSPCheckF(Condition, UserMessage) VSPCheckCF(Condition, _VSP_GET_CHECK_DEFAULT_CATEGORY(), UserMessage)
#define VSPNoEntryF(UserMessage) VSPNoEntryCF(_VSP_GET_CHECK_DEFAULT_CATEGORY(), UserMessage)

// return
#define VSPCheckReturnF(Condition, UserMessage, ...) \
	VSPCheckReturnCF(Condition, _VSP_GET_CHECK_DEFAULT_CATEGORY(), UserMessage, __VA_ARGS__)
#define VSPNoEntryReturnF(UserMessage, ...) VSPNoEntryReturnCF(_VSP_GET_CHECK_DEFAULT_CATEGORY(), UserMessage, __VA_ARGS__)

// continue
#define VSPCheckContinueF(Condition, UserMessage) \
	VSPCheckContinueCF(Condition, _VSP_GET_CHECK_DEFAULT_CATEGORY(), UserMessage)
#define VSPNoEntryContinueF(UserMessage) VSPNoEntryContinueCF(_VSP_GET_CHECK_DEFAULT_CATEGORY(), UserMessage)

// break
#define VSPCheckBreakF(Condition, UserMessage) VSPCheckBreakCF(Condition, _VSP_GET_CHECK_DEFAULT_CATEGORY(), UserMessage)
#define VSPNoEntryBreakF(UserMessage) VSPNoEntryBreakCF(_VSP_GET_CHECK_DEFAULT_CATEGORY(), UserMessage)


// -- VSPCheck*C ------------------------
// default message and custom category

// simple
#define VSPCheckC(Condition, Category) VSPCheckCF(Condition, Category, TEXT(""))
#define VSPNoEntryC(Category) VSPNoEntryCF(Category, TEXT("No entry"))

// return
#define VSPCheckReturnC(Condition, Category, ...) VSPCheckReturnCF(Condition, Category, TEXT(""), __VA_ARGS__)
#define VSPNoEntryReturnC(Category, ...) VSPNoEntryReturnCF(Category, TEXT("No entry"), __VA_ARGS__)

// continue
#define VSPCheckContinueC(Condition, Category) VSPCheckContinueCF(Condition, Category, TEXT(""))
#define VSPNoEntryContinueC(Category) VSPNoEntryContinueCF(Category, TEXT("No entry"))

// break
#define VSPCheckBreakC(Condition, Category) VSPCheckBreakCF(Condition, Category, TEXT(""))
#define VSPNoEntryBreakC(Category) VSPNoEntryBreakCF(Category, TEXT("No entry"))


// -- VSPCheck* ------------------------
// default message and category

// simple
#define VSPCheck(Condition) VSPCheckC(Condition, _VSP_GET_CHECK_DEFAULT_CATEGORY())
#define VSPNoEntry() VSPNoEntryC(_VSP_GET_CHECK_DEFAULT_CATEGORY())

// return
#define VSPCheckReturn(Condition, ...) VSPCheckReturnC(Condition, _VSP_GET_CHECK_DEFAULT_CATEGORY(), __VA_ARGS__)
#define VSPNoEntryReturn(...) VSPNoEntryReturnC(_VSP_GET_CHECK_DEFAULT_CATEGORY(), __VA_ARGS__)

// continue
#define VSPCheckContinue(Condition) VSPCheckContinueC(Condition, _VSP_GET_CHECK_DEFAULT_CATEGORY())
#define VSPNoEntryContinue() VSPNoEntryContinueC(_VSP_GET_CHECK_DEFAULT_CATEGORY())

// break
#define VSPCheckBreak(Condition) VSPCheckBreakC(Condition, _VSP_GET_CHECK_DEFAULT_CATEGORY())
#define VSPNoEntryBreak() VSPNoEntryBreakC(_VSP_GET_CHECK_DEFAULT_CATEGORY())
