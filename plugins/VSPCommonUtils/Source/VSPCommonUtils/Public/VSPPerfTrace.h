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

#include "Trace/Trace.inl"


UE_TRACE_CHANNEL_EXTERN(VSPChannel, VSPCOMMONUTILS_API);

// TODO: move this channel to UE 
UE_TRACE_CHANNEL_EXTERN(VSPUEChannel, VSPCOMMONUTILS_API);


// perf scopes with meta info
#define VSP_PERF_SCOPE_META_DEFINE(EventName) UE_TRACE_EVENT_DEFINE(Cpu,VSP_##EventName)
#define VSP_PERF_SCOPE_META_BEGIN(EventName) UE_TRACE_EVENT_BEGIN(Cpu,VSP_##EventName, NoSync)
#define VSP_PERF_SCOPE_META_BEGIN_EXTERN(EventName) UE_TRACE_EVENT_BEGIN_EXTERN(Cpu,VSP_##EventName, NoSync)

#if UE_TRACE_ENABLED
	#define VSP_PERF_SCOPE_META_BEGIN_MODULE_EXTERN(ModuleApi, EventName) \
		TRACE_PRIVATE_EVENT_BEGIN_IMPL(ModuleApi extern, Cpu,VSP_##EventName, NoSync)
#else
	#define VSP_PERF_SCOPE_META_BEGIN_MODULE_EXTERN(ModuleApi, EventName) \
		TRACE_PRIVATE_EVENT_BEGIN_IMPL(Cpu,VSP_##EventName)
#endif

#define VSP_PERF_SCOPE_META_FIELD(FieldType, FieldName) UE_TRACE_EVENT_FIELD(FieldType, FieldName)
#define VSP_PERF_SCOPE_META_END() UE_TRACE_EVENT_END()

#define VSP_PERF_SCOPE_META(EventName) UE_TRACE_LOG_SCOPED_T(Cpu,VSP_##EventName, VSPChannel)
#define VSP_PERF_SCOPE_META_ADD(EventName, Key, Value) <<VSP_##EventName.Key(Value)


// bare perf scopes
#if UE_TRACE_ENABLED

	#define VSP_PERF_SCOPE_UCLASS()VSP_PERF_SCOPE_PRIVATE(*StaticClass()->GetName(), )
	#define VSP_PERF_SCOPE_UCLASS_F(Format, ...) \
		VSP_PERF_SCOPE_PRIVATE(*StaticClass()->GetName(), " " Format, ##__VA_ARGS__)

	#define VSP_PERF_SCOPE_FCLASS(ClassName)VSP_PERF_SCOPE_PRIVATE(TEXT(#ClassName), )
	#define VSP_PERF_SCOPE_FCLASS_F(ClassName, Format, ...) \
		VSP_PERF_SCOPE_PRIVATE(TEXT(#ClassName), " " Format, ##__VA_ARGS__)

	#define VSP_PERF_SCOPE_PRIVATE(ClassName, Format, ...)                                                              \
		const auto PREPROCESSOR_JOIN(NamedEventText_, __LINE__) =                                                      \
			FString::Printf(TEXT("VSP_%s_%s" Format), ClassName, ANSI_TO_TCHAR(__func__), ##__VA_ARGS__);               \
		FScopedNamedEvent ANONYMOUS_VARIABLE(NamedEvent_)(FColor::Red, *PREPROCESSOR_JOIN(NamedEventText_, __LINE__)); \
		TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(*PREPROCESSOR_JOIN(NamedEventText_, __LINE__), VSPChannel)


	#define VSP_TRACE_SCOPE_F(Format, ...)                                                                              \
		const auto PREPROCESSOR_JOIN(NamedEventText_, __LINE__) = FString::Printf(TEXT("VSP_" Format), ##__VA_ARGS__);  \
		FScopedNamedEvent ANONYMOUS_VARIABLE(NamedEvent_)(FColor::Red, *PREPROCESSOR_JOIN(NamedEventText_, __LINE__)); \
		TRACE_CPUPROFILER_EVENT_SCOPE_TEXT_ON_CHANNEL(*PREPROCESSOR_JOIN(NamedEventText_, __LINE__), VSPChannel)


#else

	#define VSP_PERF_SCOPE_UCLASS()
	#define VSP_PERF_SCOPE_UCLASS_F(Format, ...)
	#define VSP_PERF_SCOPE_FCLASS(ClassName)
	#define VSP_PERF_SCOPE_FCLASS_F(ClassName, Format, ...)
	#define VSP_TRACE_SCOPE_F(Format, ...)

#endif
