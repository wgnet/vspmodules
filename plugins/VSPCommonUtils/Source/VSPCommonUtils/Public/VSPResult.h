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

#include "ToStringCommon.h"

#include "Misc/Optional.h"
#include "Misc/TVariant.h"

template<typename TFailureType, typename TResultValueType>
class TVSPResult
{
public:
	using FailureType = TFailureType;
	using ResultType = TResultValueType;

	static TVSPResult MakeSuccess(const TResultValueType& Result);
	static TVSPResult MakeSuccess(TResultValueType&& Result);
	static TVSPResult MakeFailure(const TFailureType& Failure);
	static TVSPResult MakeFailure(TFailureType&& Failure);
	TVSPResult();
	TVSPResult(const TVSPResult& ResultToCopy) = default;
	TVSPResult(TVSPResult&& ResultToMove) = default;
	TVSPResult& operator=(const TVSPResult& ResultToCopy) = default;
	TVSPResult& operator=(TVSPResult&& ResultToMove) = default;

	bool IsSuccess() const;
	bool IsFailure() const;

	const TResultValueType& GetResult() const;
	const TFailureType& GetFailure() const;

	void Apply(const TFunction<TFailureType>& FailureFunc, const TFunction<TResultValueType>& ResultValueFunc) const;

private:
	using Impl = TVariant<TResultValueType, TFailureType>;

	TVSPResult(Impl&& InResultOrFailure);

	Impl ResultOrFailure;
};

//---

template<typename TFailureType>
class TVSPResult<TFailureType, void>
{
public:
	using FailureType = TFailureType;
	using ResultType = void;
	using ThisType = TVSPResult<TFailureType, void>;

	static ThisType MakeSuccess();
	static ThisType MakeFailure(const TFailureType& Failure);
	static ThisType MakeFailure(TFailureType&& Failure);
	TVSPResult();
	TVSPResult(const TVSPResult& ResultToCopy) = default;
	TVSPResult(TVSPResult&& ResultToMove) = default;
	TVSPResult& operator=(const TVSPResult& ResultToCopy) = default;
	TVSPResult& operator=(TVSPResult&& ResultToMove) = default;

	bool IsSuccess() const;
	bool IsFailure() const;

	const TFailureType& GetFailure() const;

	void Apply(const TFunction<void(TFailureType)>& FailureFunc, const TFunction<void()>& ResultValueFunc) const;

private:
	TVSPResult(TOptional<TFailureType>&& InFailure);

	TOptional<TFailureType> Failure;
};

//---

template<typename TResultValueType>
class TVSPResult<void, TResultValueType>
{
public:
	using FailureType = void;
	using ResultType = TResultValueType;
	using ThisType = TVSPResult<void, TResultValueType>;

	static ThisType MakeSuccess(const TResultValueType& Result);
	static ThisType MakeSuccess(TResultValueType&& Result);
	static ThisType MakeFailure();
	TVSPResult();
	TVSPResult(const TVSPResult& ResultToCopy) = default;
	TVSPResult(TVSPResult&& ResultToMove) = default;
	TVSPResult& operator=(const TVSPResult& ResultToCopy) = default;
	TVSPResult& operator=(TVSPResult&& ResultToMove) = default;

	bool IsSuccess() const;
	bool IsFailure() const;

	const TResultValueType& GetResult() const;

	void Apply(const TFunction<void()>& FailureFunc, const TFunction<void(TResultValueType)>& ResultValueFunc) const;

private:
	TVSPResult(TOptional<TResultValueType>&& InResult);

	TOptional<TResultValueType> Result;
};

//---

template<>
class TVSPResult<void, void>
{
public:
	using FailureType = void;
	using ResultType = void;
	using ThisType = TVSPResult<void, void>;

	static ThisType MakeSuccess()
	{
		return ThisType { true };
	}

	static ThisType MakeFailure()
	{
		return ThisType { false };
	}

	static ThisType Make(const bool bSuccess)
	{
		return ThisType { bSuccess };
	}

	TVSPResult() = default;
	TVSPResult(const ThisType& ResultToCopy) = default;
	TVSPResult(ThisType&& ResultToMove) = default;
	TVSPResult& operator=(const ThisType& ResultToCopy) = default;
	TVSPResult& operator=(ThisType&& ResultToMove) = default;

	bool IsSuccess() const
	{
		return bIsSuccess;
	}

	bool IsFailure() const
	{
		return !bIsSuccess;
	}

	void Apply(const TFunction<void()>& FailureFunc, const TFunction<void()>& ResultValueFunc) const
	{
		if (IsSuccess())
			ResultValueFunc();
		else
			FailureFunc();
	}

private:
	TVSPResult(const bool InIsSuccess) : bIsSuccess(InIsSuccess)
	{
	}

	bool bIsSuccess = false;
};

//---

inline FString ToString(const TVSPResult<void, void>& Result, const int32 Offset = 0);

template<typename TFailureType>
inline FString ToString(const TVSPResult<TFailureType, void>& Result, const int32 Offset = 0);

template<typename TResultValueType>
FString ToString(const TVSPResult<void, TResultValueType>& Result, const int32 Offset = 0);

template<typename TFailureType, typename TResultValueType>
FString ToString(const TVSPResult<TFailureType, TResultValueType>& Result, const int32 Offset = 0);

// ================================================================================================================
// ====================================== Implementation ==========================================================
// ================================================================================================================

template<typename TFailureType, typename TResultValueType>
TVSPResult<TFailureType, TResultValueType> TVSPResult<TFailureType, TResultValueType>::MakeSuccess(
	const TResultValueType& Result)
{
	return TVSPResult { Impl { TInPlaceType<TResultValueType>(), Result } };
}

template<typename TFailureType, typename TResultValueType>
TVSPResult<TFailureType, TResultValueType> TVSPResult<TFailureType, TResultValueType>::MakeSuccess(
	TResultValueType&& Result)
{
	return TVSPResult { Impl { TInPlaceType<TResultValueType>(), MoveTemp(Result) } };
}

template<typename TFailureType, typename TResultValueType>
TVSPResult<TFailureType, TResultValueType> TVSPResult<TFailureType, TResultValueType>::MakeFailure(
	const TFailureType& Failure)
{
	return TVSPResult { Impl { TInPlaceType<TFailureType>(), Failure } };
}

template<typename TFailureType, typename TResultValueType>
TVSPResult<TFailureType, TResultValueType> TVSPResult<TFailureType, TResultValueType>::MakeFailure(TFailureType&& Failure)
{
	return TVSPResult { Impl { TInPlaceType<TFailureType>(), MoveTemp(Failure) } };
}

template<typename TFailureType, typename TResultValueType>
TVSPResult<TFailureType, TResultValueType>::TVSPResult()
	: TVSPResult(Impl { TInPlaceType<TFailureType>(), TFailureType {} })
{
}

template<typename TFailureType, typename TResultValueType>
bool TVSPResult<TFailureType, TResultValueType>::IsSuccess() const
{
	return ResultOrFailure.template IsType<TResultValueType>();
}

template<typename TFailureType, typename TResultValueType>
bool TVSPResult<TFailureType, TResultValueType>::IsFailure() const
{
	return ResultOrFailure.template IsType<TFailureType>();
}

template<typename TFailureType, typename TResultValueType>
const TResultValueType& TVSPResult<TFailureType, TResultValueType>::GetResult() const
{
	auto MakeErrorMessageLambda = [this]()
	{
		return FString::Printf(TEXT("Expected success, but is failure:\n") TEXT("%s"), *TryToString(GetFailure()));
	};
	VSPCheckF(IsSuccess(), *MakeErrorMessageLambda());

	return ResultOrFailure.template Get<TResultValueType>();
}

template<typename TFailureType, typename TResultValueType>
const TFailureType& TVSPResult<TFailureType, TResultValueType>::GetFailure() const
{
	auto MakeErrorMessageLambda = [this]()
	{
		return FString::Printf(TEXT("Expected success, but is failure:\n") TEXT("%s"), *TryToString(GetFailure()));
	};
	VSPCheckF(IsFailure(), *MakeErrorMessageLambda());

	return ResultOrFailure.template Get<TFailureType>();
}

template<typename TFailureType, typename TResultValueType>
void TVSPResult<TFailureType, TResultValueType>::Apply(
	const TFunction<TFailureType>& FailureFunc,
	const TFunction<TResultValueType>& ResultValueFunc) const
{
	if (IsSuccess())
		ResultValueFunc(GetResult());
	else
		FailureFunc(GetFailure());
}

template<typename TFailureType, typename TResultValueType>
TVSPResult<TFailureType, TResultValueType>::TVSPResult(Impl&& InResultOrFailure) : ResultOrFailure(InResultOrFailure)
{
}

//-------------------------------------------------------

template<typename TFailureType>
TVSPResult<TFailureType, void> TVSPResult<TFailureType, void>::MakeSuccess()
{
	return ThisType { {} };
}

template<typename TFailureType>
TVSPResult<TFailureType, void> TVSPResult<TFailureType, void>::MakeFailure(const TFailureType& Failure)
{
	return ThisType { { Failure } };
}

template<typename TFailureType>
TVSPResult<TFailureType, void> TVSPResult<TFailureType, void>::MakeFailure(TFailureType&& Failure)
{
	return ThisType { { MoveTemp(Failure) } };
}

template<typename TFailureType>
TVSPResult<TFailureType, void>::TVSPResult() : TVSPResult(TOptional<TFailureType> { TFailureType {} })
{
}

template<typename TFailureType>
bool TVSPResult<TFailureType, void>::IsSuccess() const
{
	return !Failure.IsSet();
}

template<typename TFailureType>
bool TVSPResult<TFailureType, void>::IsFailure() const
{
	return Failure.IsSet();
}

template<typename TFailureType>
const TFailureType& TVSPResult<TFailureType, void>::GetFailure() const
{
	static TFailureType Dummy;
	VSPCheckReturnF(IsFailure(), TEXT("Expected failure, but is success"), Dummy);
	return Failure.GetValue();
}

template<typename TFailureType>
void TVSPResult<TFailureType, void>::Apply(
	const TFunction<void(TFailureType)>& FailureFunc,
	const TFunction<void()>& ResultValueFunc) const
{
	if (IsSuccess())
		ResultValueFunc();
	else
		FailureFunc(GetFailure());
}

template<typename TFailureType>
TVSPResult<TFailureType, void>::TVSPResult(TOptional<TFailureType>&& InFailure) : Failure(InFailure)
{
}

// ---------------

template<typename TResultValueType>
TVSPResult<void, TResultValueType> TVSPResult<void, TResultValueType>::MakeSuccess(const TResultValueType& Result)
{
	return ThisType { { Result } };
}

template<typename TResultValueType>
TVSPResult<void, TResultValueType> TVSPResult<void, TResultValueType>::MakeSuccess(TResultValueType&& Result)
{
	return ThisType { { Result } };
}

template<typename TResultValueType>
TVSPResult<void, TResultValueType> TVSPResult<void, TResultValueType>::MakeFailure()
{
	return ThisType { {} };
}

template<typename TResultValueType>
TVSPResult<void, TResultValueType>::TVSPResult() : TVSPResult(TOptional<TResultValueType> {})
{
}

template<typename TResultValueType>
bool TVSPResult<void, TResultValueType>::IsSuccess() const
{
	return Result.IsSet();
}

template<typename TResultValueType>
bool TVSPResult<void, TResultValueType>::IsFailure() const
{
	return !Result.IsSet();
}

template<typename TResultValueType>
const TResultValueType& TVSPResult<void, TResultValueType>::GetResult() const
{
	static TResultValueType Dummy;
	VSPCheckReturnF(IsSuccess(), TEXT("Expected success, but is failure"), Dummy);
	return Result.GetValue();
}

template<typename TResultValueType>
void TVSPResult<void, TResultValueType>::Apply(
	const TFunction<void()>& FailureFunc,
	const TFunction<void(TResultValueType)>& ResultValueFunc) const
{
	if (IsSuccess())
		ResultValueFunc(GetResult());
	else
		FailureFunc();
}

template<typename TResultValueType>
TVSPResult<void, TResultValueType>::TVSPResult(TOptional<TResultValueType>&& InResult) : Result(InResult)
{
}

// -----------------------------------

inline FString ToString(const TVSPResult<void, void>& Result, const int32 Offset)
{
	const FString OffsetString { FString::ChrN(Offset, '\t') };
	return Result.IsSuccess() ? FString::Printf(TEXT("%sSuccess"), *OffsetString)
							  : FString::Printf(TEXT("%sFailure"), *OffsetString);
}

template<typename TFailureType>
inline FString ToString(const TVSPResult<TFailureType, void>& Result, const int32 Offset)
{
	const FString OffsetString { FString::ChrN(Offset, '\t') };
	return Result.IsSuccess() ? FString::Printf(TEXT("%sSuccess"), *OffsetString)
							  : FString::Printf(
								  TEXT("%sFailure:\n") TEXT("%s"),
								  *OffsetString,
								  *TryToString(Result.GetFailure(), Offset + 1));
}

template<typename TResultValueType>
FString ToString(const TVSPResult<void, TResultValueType>& Result, const int32 Offset)
{
	const FString OffsetString { FString::ChrN(Offset, '\t') };
	return Result.IsSuccess()
		? FString::Printf(TEXT("%sSuccess:\n") TEXT("%s"), *OffsetString, *TryToString(Result.GetResult(), Offset + 1))
		: FString::Printf(TEXT("%sFailure"), *OffsetString);
}

template<typename TFailureType, typename TResultValueType>
FString ToString(const TVSPResult<TFailureType, TResultValueType>& Result, const int32 Offset)
{
	const FString OffsetString { FString::ChrN(Offset, '\t') };
	return Result.IsSuccess()
		? FString::Printf(TEXT("%sSuccess:\n") TEXT("%s"), *OffsetString, *TryToString(Result.GetResult(), Offset + 1))
		: FString::Printf(
			TEXT("%sFailure:\n") TEXT("%s"),
			*OffsetString,
			*TryToString(Result.GetFailure(), Offset + 1));
}
