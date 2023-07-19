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
#include "IntegralConstant.h"
#include "SelectType.h"
#include "TIsSpecializationOf.h"

namespace VSPAsync
{
	namespace Detail
	{
		// this doesn't actually match Declval. it behaves differently for void!
		template<class T>
		T&& AsyncDeclval();

		template<class Function>
		auto IsCallable(Function Func, int) -> decltype(Func(), TrueType());
		template<class Function>
		FalseType IsCallable(Function, ...);

		struct BadType
		{
		};

		template<class ArgType, class FailureType, class Function, class Type>
		auto ReturnTypeHelper(Type Obj, Function Func, int, int)
			-> decltype(Func(DeclVal<TTask<Type, ArgType, FailureType>>()));
		template<class ArgType, class FailureType, class Function, class Type>
		auto ReturnTypeHelper(Type Obj, Function Func, int, ...) -> decltype(Func(Obj));
		template<class ArgType, class FailureType, class Function, class Type>
		auto ReturnTypeHelper(Type Obj, Function Func, ...) -> BadType;

		template<class ArgType, class FailureType, class Function>
		auto VoidReturnTypeHelper(Function Func, int, int)
			-> decltype(Func(DeclVal<TTask<void, ArgType, FailureType>>()));
		template<class ArgType, class FailureType, class Function>
		auto VoidReturnTypeHelper(Function Func, int, ...) -> decltype(Func());
		template<class ArgType, class FailureType, class Function>
		auto VoidReturnTypeHelper(Function Func, ...) -> BadType;

		template<class ArgType, class FailureType, class Function, class Type>
		auto IsTaskHelper(Type Obj, Function Func, int, int)
			-> decltype(Func(DeclVal<TTask<Type, ArgType, FailureType>>()), TrueType());
		template<class ArgType, class FailureType, class Function, class Type>
		auto IsTaskHelper(Type Obj, Function Func, int, ...) -> FalseType;

		template<class ArgType, class FailureType, class Function, class ExpectedParameterType>
		struct ContinuationFunctionTypeTraits
		{
			// void continuation is valid even is prev task returns value
			using _TryGetVoidFuncRetType =
				decltype(VoidReturnTypeHelper<ArgType, FailureType>(AsyncDeclval<Function>(), 0, 0));
			static constexpr bool bTakesVoid = !TIsSame<_TryGetVoidFuncRetType, BadType>::Value;

			using FuncRetType = VSPUtils::SelectType<
				bTakesVoid,
				_TryGetVoidFuncRetType,
				decltype(ReturnTypeHelper<ArgType, FailureType>(
					AsyncDeclval<ExpectedParameterType>(),
					AsyncDeclval<Function>(),
					0,
					0))>;

			static_assert(
				!TIsSame<FuncRetType, BadType>::Value,
				"Incorrect parameter type for the callable object: "
				"expected success return value of the previous chain element or void");

			using TakesTask = decltype(IsTaskHelper<ArgType, FailureType>(
				AsyncDeclval<ExpectedParameterType>(),
				AsyncDeclval<Function>(),
				0,
				0));
		};

		template<class ArgType, class FailureType, class Function>
		auto VoidIsTaskHelper(Function Func, int, int)
			-> decltype(Func(DeclVal<TTask<void, ArgType, FailureType>>()), TrueType());
		template<class ArgType, class FailureType, class Function>
		auto VoidIsTaskHelper(Function Func, int, ...) -> FalseType;

		template<class ArgType, class FailureType, class Function>
		struct ContinuationFunctionTypeTraits<ArgType, FailureType, Function, void>
		{
			using FuncRetType = decltype(VoidReturnTypeHelper<ArgType, FailureType>(AsyncDeclval<Function>(), 0, 0));
			static constexpr bool bTakesVoid = true;
			using TakesTask = decltype(VoidIsTaskHelper<ArgType, FailureType>(AsyncDeclval<Function>(), 0, 0));

			static_assert(
				!TIsSame<FuncRetType, BadType>::Value,
				"Incorrect parameter type for the callable object: "
				"expected success return value of the previous chain element (void)");
		};

		template<class PrevTaskSuccessType, class PrevArgType, class FailureType, class IsTaskType>
		struct ContinuationArgTypeHelper
		{
			using ArgType = PrevTaskSuccessType;
		};

		template<class PrevTaskSuccessType, class PrevArgType, class FailureType>
		struct ContinuationArgTypeHelper<PrevTaskSuccessType, PrevArgType, FailureType, TrueType>
		{
			using ArgType = TTask<PrevTaskSuccessType, PrevArgType, FailureType>;
		};

		// Unwrap TTask
		template<class ArgType, class FailureType, class RetValType>
		RetValType GetUnwrappedType(TTask<RetValType, ArgType, FailureType>);

		// Unwrap all supported types
		template<class ArgType, class FailureType, class T>
		auto GetUnwrappedReturnType(T Arg, int) -> decltype(GetUnwrappedType<ArgType, FailureType>(Arg));

		// fallback
		template<class ArgType, class FailureType, class T>
		T GetUnwrappedReturnType(T, ...);

		// Non-Callable
		template<class ArgType, class FailureType, class T>
		auto GetTaskType(T NonFunc, FalseType) -> decltype(GetUnwrappedType<ArgType, FailureType>(NonFunc));

		// Callable
		template<class ArgType, class FailureType, class T>
		auto GetTaskType(T Func, TrueType) -> decltype(GetUnwrappedReturnType<ArgType, FailureType>(Func(), 0));

		// Special callable returns void
		template<class ArgType, class FailureType>
		void GetTaskType(TFunction<void()>, TrueType);

		template<class ArgType, class FailureType, class ParamType>
		auto FilterValidTaskType(ParamType Param, int)
			-> decltype(GetTaskType<ArgType, FailureType>(Param, IsCallable(Param, 0)));

		template<class ArgType, class FailureType, class ParamType>
		BadType FilterValidTaskType(ParamType Param, ...);

		template<class ArgType, class FailureType, class ParamType>
		struct TaskTypeFromParam
		{
			using Type = decltype(FilterValidTaskType<ArgType, FailureType>(AsyncDeclval<ParamType>(), 0));
			static_assert(!TIsSame<Type, BadType>::Value, "Invalid task type");
		};

		template<class TestingType, bool bIsTaskPtr /* = true */>
		struct IsTaskSelector
		{
			using TaskSuccessType = typename TestingType::ElementType::TaskSuccessType;
		};

		template<class TestingType>
		struct IsTaskSelector<TestingType, /* bool bIsTaskPtr = */ false>
		{
			using TaskSuccessType = TestingType;
		};

		struct CIsTaskPtr
		{
			template<typename TChecked>
			auto Requires(TChecked& Value) -> decltype(Value.ToSharedRef()->~TTask());
		};

		template<class TestingType>
		using ResolveTaskTypes = IsTaskSelector<TestingType, TModels<CIsTaskPtr, TestingType>::Value>;

		template<class Function, class FailureType>
		struct CreationTypeTraits
		{
			using _RealResultType = typename TaskTypeFromParam<void, FailureType, Function>::Type;

			using TaskArgType = void;
			using TaskSuccessType = typename ResolveTaskTypes<_RealResultType>::TaskSuccessType;
			static constexpr bool bTakesVoid = true;
			using TaskType = TTask<TaskSuccessType, TaskArgType, FailureType>;
		};

		template<class Function, class PrevTaskSuccessType, class PrevArgType, class FailureType>
		struct ContinuationTypeTraits
		{
			using _ContinuationFunctionTypeTraits =
				ContinuationFunctionTypeTraits<void, FailureType, Function, PrevTaskSuccessType>;
			using _RealResultType = typename _ContinuationFunctionTypeTraits::FuncRetType;

			using TaskArgType = typename ContinuationArgTypeHelper<
				PrevTaskSuccessType,
				PrevArgType,
				FailureType,
				typename _ContinuationFunctionTypeTraits::TakesTask>::ArgType;
			using TaskSuccessType = typename ResolveTaskTypes<_RealResultType>::TaskSuccessType;
			static constexpr bool bTakesVoid = _ContinuationFunctionTypeTraits::bTakesVoid;
			using TaskType = TTask<TaskSuccessType, TaskArgType, FailureType>;
		};

		template<class Traits>
		struct TaskFunctionCreator
		{
		private:
			template<class _Traits, bool bRealResultTypeIsSuccess /* = true */>
			struct IsRealResultTypeSuccessSelector
			{
				template<class __Traits, bool bArgVoid /* = true */>
				struct IsArgTypeVoidSelector
				{
				private:
					template<class ___Traits, bool bReturnTypeVoid /* = true */>
					struct IsReturnTypeVoidSelector
					{
						template<class Function>
						static typename ___Traits::TaskType::TaskFunctionType Create(Function Func)
						{
							return typename ___Traits::TaskType::TaskFunctionType(
								[_Func = MoveTemp(Func)]()
								{
									_Func();
									return ___Traits::TaskType::MakeSuccess();
								});
						}
					};

					template<class ___Traits>
					struct IsReturnTypeVoidSelector<___Traits, /* bool bReturnTypeVoid = */ false>
					{
						template<class Function>
						static typename ___Traits::TaskType::TaskFunctionType Create(Function Func)
						{
							return typename ___Traits::TaskType::TaskFunctionType(
								[_Func = MoveTemp(Func)]()
								{
									return ___Traits::TaskType::MakeSuccess(_Func());
								});
						}
					};

				public:
					template<class Function>
					static typename __Traits::TaskType::TaskFunctionType Create(Function Func)
					{
						return IsReturnTypeVoidSelector<
							__Traits,
							TIsSame<typename __Traits::TaskSuccessType, void>::Value>::Create(MoveTemp(Func));
					}
				};

				template<class __Traits>
				struct IsArgTypeVoidSelector<__Traits, /* bool bArgVoid = */ false>
				{
				private:
					template<class ___Traits, bool bTakesVoid /* = true */>
					struct IsTakesVoidSelector
					{
						template<class Function>
						static typename ___Traits::TaskType::TaskFunctionType Create(Function Func)
						{
							return typename ___Traits::TaskType::TaskFunctionType(
								[_Func = MoveTemp(Func)](const typename ___Traits::TaskArgType&)
								{
									_Func();
									return ___Traits::TaskType::MakeSuccess();
								});
						}
					};

					template<class ___Traits>
					struct IsTakesVoidSelector<___Traits, /* bool bTakesVoid = */ false>
					{
					private:
						template<class ____Traits, bool bReturnTypeVoid /* = true */>
						struct IsReturnTypeVoidSelector
						{
							template<class Function>
							static typename ____Traits::TaskType::TaskFunctionType Create(Function Func)
							{
								return typename ____Traits::TaskType::TaskFunctionType(
									[_Func = MoveTemp(Func)](const typename ____Traits::TaskArgType& Arg)
									{
										_Func(Arg);
										return ____Traits::TaskType::MakeSuccess();
									});
							}
						};

						template<class ____Traits>
						struct IsReturnTypeVoidSelector<____Traits, /* bool bReturnTypeVoid = */ false>
						{
							template<class Function>
							static typename ____Traits::TaskType::TaskFunctionType Create(Function Func)
							{
								return typename ____Traits::TaskType::TaskFunctionType(
									[_Func = MoveTemp(Func)](const typename ____Traits::TaskArgType& Arg)
									{
										return ____Traits::TaskType::MakeSuccess(_Func(Arg));
									});
							}
						};

					public:
						template<class Function>
						static typename ___Traits::TaskType::TaskFunctionType Create(Function Func)
						{
							return IsReturnTypeVoidSelector<
								___Traits,
								TIsSame<typename ___Traits::TaskSuccessType, void>::Value>::Create(MoveTemp(Func));
						}
					};

				public:
					template<class Function>
					static typename __Traits::TaskType::TaskFunctionType Create(Function Func)
					{
						return IsTakesVoidSelector<__Traits, __Traits::bTakesVoid>::Create(MoveTemp(Func));
					}
				};

				// TaskSuccessType
				template<class Function>
				static typename _Traits::TaskType::TaskFunctionType Create(Function Func)
				{
					return IsArgTypeVoidSelector<_Traits, TIsSame<typename _Traits::TaskArgType, void>::Value>::Create(
						MoveTemp(Func));
				}
			};

			template<class _Traits>
			struct IsRealResultTypeSuccessSelector<_Traits, /* bool bRealResultTypeIsSuccess = */ false>
			{
				// TaskType
				template<class Function>
				static typename _Traits::TaskType::TaskFunctionType Create(Function Func)
				{
					return typename _Traits::TaskType::TaskFunctionType(MoveTemp(Func));
				}
			};

		public:
			template<class Function>
			static typename Traits::TaskType::TaskFunctionType Create(Function Func)
			{
				using IsRealResultTypeSuccess = IsRealResultTypeSuccessSelector<
					Traits,
					TIsSame<typename Traits::_RealResultType, typename Traits::TaskSuccessType>::Value>;

				return IsRealResultTypeSuccess::Create(MoveTemp(Func));
			}
		};
	}
}
