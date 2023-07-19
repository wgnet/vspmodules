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

#include <iterator>

#include "Containers/UnrealString.h"
#include "Templates/UnrealTypeTraits.h"

namespace _Detail_V_S_P_F_M_T // dirty namespace to prevent Rider's code completion then typing VSPF.. and use tab
{
	template<class ContainerT>
	class BackInsertIterator
	{
		using ElementT = typename ContainerT::ElementType;

	public:
		// making std happy
		using iterator_category = std::output_iterator_tag;
		using value_type = void;
		using difference_type = void;
		using pointer = void;
		using reference = void;

		using container_type = ContainerT;

	public:
		explicit BackInsertIterator(ContainerT& Container);

		BackInsertIterator& operator=(const ElementT Value);

		BackInsertIterator& operator*();

		BackInsertIterator& operator++();

		BackInsertIterator operator++(int);

	protected:
		ContainerT* Container;
	};


	//------------------------------------------------------------------------------------------------------------------

	template<class ContainerT>
	BackInsertIterator<ContainerT>::BackInsertIterator(ContainerT& Container) : Container(&Container)
	{
	}

	template<class ContainerT>
	BackInsertIterator<ContainerT>& BackInsertIterator<ContainerT>::operator=(const ElementT Value)
	{
		static_assert(
			TIsSame<ContainerT, FString>::Value && TIsSame<ElementT, TCHAR>::Value,
			"Designed only for FString and TCHAR");

		Container->AppendChar(Value);
		return *this;
	}

	template<class ContainerT>
	BackInsertIterator<ContainerT>& BackInsertIterator<ContainerT>::operator*()
	{
		return *this;
	}

	template<class ContainerT>
	BackInsertIterator<ContainerT>& BackInsertIterator<ContainerT>::operator++()
	{
		return *this;
	}

	template<class ContainerT>
	BackInsertIterator<ContainerT> BackInsertIterator<ContainerT>::operator++(int)
	{
		return *this;
	}


	template<class ContainerT>
	inline BackInsertIterator<ContainerT> BackInserter(ContainerT& Container)
	{
		return (_Detail_V_S_P_F_M_T::BackInsertIterator<ContainerT>(Container));
	}
}
