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

namespace VSPAsync
{
	namespace Detail
	{
		template<class ReturnType, class ArgType, bool IsArgVoid /* = true */>
		struct FunctionVoidSelector
		{
			using Function = TFunction<ReturnType()>;
		};

		template<class ReturnType, class ArgType>
		struct FunctionVoidSelector<ReturnType, ArgType, /* bool IsArgVoid = */ false>
		{
			using Function = TFunction<ReturnType(const ArgType&)>;
		};

		template<bool bReturnTypeVoid /* = true */>
		struct SuccessValueExtractor
		{
			template<class ResultType>
			static auto Extract(const ResultType&)
			{
				return nullptr;
			}
		};

		template<>
		struct SuccessValueExtractor</* bool bReturnTypeVoid = */ false>
		{
			template<class ResultType>
			static auto Extract(const ResultType& Result)
			{
				return &Result.GetResult();
			}
		};

		template<class _ArgType, bool IsArgVoid /* = true */>
		struct ExecutionSelector
		{
			template<class _FunctionType>
			static auto Execute(const _FunctionType& Func, const _ArgType*)
			{
				return Func();
			}
		};

		template<class _ArgType>
		struct ExecutionSelector<_ArgType, /* bool IsArgVoid = */ false>
		{
			template<class _FunctionType>
			static auto Execute(const _FunctionType& Func, const _ArgType* Arg)
			{
				VSPCheckCF(Arg, VSPAsyncLog, TEXT("Internal error: arg is nullptr"));
				return Func(*Arg);
			}
		};

		template<
			class NextFunctionConstructionDataType,
			class NextFunctionCreatorType,
			class HeadTaskType,
			class... RestTasksType>
		void UnrollTasks(
			NextFunctionConstructionDataType NextFunctionConstructionData,
			const NextFunctionCreatorType& NextFunctionCreator,
			HeadTaskType& HeadTask,
			RestTasksType&&... RestTasks);
	}
}