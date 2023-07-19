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

#include "Containers/Queue.h"

namespace VSPPerfUtils
{
	enum class EWalkStatus
	{
		Continue,
		Break,
		Stop
	};

	/* Recursive Breadth-First walk, Return false if interrupted by Func returned EWalkStatus::Stop */
	template <class T, class ChildContainerType>
	void RecursiveBfsWalk(T* Root, ChildContainerType T::* ChildContainer, TFunction<EWalkStatus(T*)> Func)
	{
		TQueue<T*> Queue;
		Queue.Enqueue(Root);
		while (!Queue.IsEmpty())
		{
			T* Top = *Queue.Peek();
			switch (Func(Top))
			{
			case EWalkStatus::Stop:
				return;
			case EWalkStatus::Continue:
				for (TSharedPtr<T> Child : Top->*ChildContainer)
					if (Child)
						Queue.Enqueue(Child.Get());
			case EWalkStatus::Break:
			default:
				break;
			}
			Queue.Pop();
		}
	}

	/* Recursive Breadth-First walk, Return false if interrupted by Func returned EWalkStatus::Stop */
	template <class T, class ChildContainerType>
	bool RecursiveBfsWalk(TSharedPtr<T>& Root,
	                      ChildContainerType T::* ChildContainer,
	                      TFunction<EWalkStatus(TSharedPtr<T>&)> Func)
	{
		if (!Root.IsValid())
			return true;

		TQueue<TSharedPtr<T>*> Queue;
		Queue.Enqueue(&Root);
		while (!Queue.IsEmpty())
		{
			TSharedPtr<T>* Top;
			Queue.Dequeue(Top);
			if (Top && *Top)
			{
				switch (Func(*Top))
				{
				case EWalkStatus::Stop:
					return false;
				case EWalkStatus::Continue:
					for (TSharedPtr<T>& Child : Top->Get()->*ChildContainer)
						if (Child)
							Queue.Enqueue(&Child);
				case EWalkStatus::Break:
				default:
					break;
				}
			}
		}
		return true;
	}
}
