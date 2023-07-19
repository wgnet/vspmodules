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

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "type_traits"

#include "FSM.generated.h"


//--------------------------------------------------------
//--------------------------------------------------------

UCLASS()
class PROJECTVSP_API UFSM : public UActorComponent
{
	template<class T>
	static constexpr bool IsStateIdEnum =
		std::is_enum<T>::value && std::is_same<typename std::underlying_type<T>::type, int8>::value;

public:
	class FState;
	class FTransition;
	template<class OwnerClass>
	class TState;
	template<class OwnerClass>
	class TTransition;
	enum class EMachineNetMode;
	enum class EStateNetMode;

	UFSM();

	template<class StateIdT>
	void InitConstructor(EMachineNetMode MachineNetMode, StateIdT InitialStateId, bool ManualTick = false)
	{
		static_assert(IsStateIdEnum<StateIdT>, "StateId must be enum class : int8");
		Setup(MachineNetMode, static_cast<int8>(InitialStateId), ManualTick);
	}

	template<class StateIdT>
	void AddState(StateIdT StateId, FState* State)
	{
		static_assert(IsStateIdEnum<StateIdT>, "StateId must be enum class : int8");
		AddStateInner(static_cast<int8>(StateId), State);
	}

	template<class StateIdT>
	void AddTransition(StateIdT FromStateID, StateIdT ToStateID, FTransition* Transition)
	{
		static_assert(IsStateIdEnum<StateIdT>, "StateId must be enum class : int8");
		AddTransitionInner(static_cast<int8>(FromStateID), static_cast<int8>(ToStateID), Transition);
	}

	template<class StateIdT>
	void SetStartingState(StateIdT ToStateId)
	{
		static_assert(IsStateIdEnum<StateIdT>, "StateId must be enum class : int8");
		SetStartingStateInner(static_cast<int8>(ToStateId));
	}

	template<class StateIdT>
	void ForceTransition(StateIdT ToStateId, bool bReEnter = false)
	{
		static_assert(IsStateIdEnum<StateIdT>, "StateId must be enum class : int8");
		ForceTransitionInner(static_cast<int8>(ToStateId), bReEnter);
	}

	void SetupReplication(EMachineNetMode MachineNetMode);

	int32 GetCurrentStateID() const =
		delete; // current FSM state should never be used, all state-related logic should be implemented in state objects

	template<class StateIdT>
	void SetDebugInfo(
		TCHAR const* DebugSection,
		TCHAR const* StateMachineName,
		TMap<StateIdT, TCHAR const*> const& StateNames)
	{
		static_assert(IsStateIdEnum<StateIdT>, "StateId must be enum class : int8");
	}

	void ManualTick(float DeltaTime);

	void SuspendTransitions();
	void ResumeTransitions();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(EEndPlayReason::Type const EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
		override;

private:
	using UActorComponent::SetIsReplicated; // hide SetIsReplicated method, use SetNetMode instead

	void AddStateInner(int8 StateID, FState* State);
	void AddTransitionInner(int8 FromStateID, int8 ToStateID, FTransition* Transition);
	void Setup(EMachineNetMode MachineNetMode, int8 StateID, bool ManualTick);
	void TryToForceTransition(int8 ToStateID, bool bReEnter);
	void ForceTransitionInner(int8 ToStateID, bool bReEnter);
	void SetStartingStateInner(int8 StateID);

	struct FStateInfo
	{
		FState* State;
		TArray<TTuple<int8, FTransition*>, TInlineAllocator<8> > Transitions;
	};

	EMachineNetMode NetMode;
	TMap<int8, FStateInfo> States;

	UPROPERTY(Replicated)
	int8 CurrentStateID;
	bool bValidClientState;

	void TickState(float DeltaTime);
	void BeginState(int8 StateID);
	void EndState(int8 StateID);
	void SwitchState(int8 StateID);

	UFUNCTION(Client, Reliable)
	void ClientBeginState(int8 StateID);

	UFUNCTION(Client, Reliable)
	void ClientEndState(int8 StateID);

	UFUNCTION(Client, Reliable)
	void ClientSwitchState(int8 StateID);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastBeginState(int8 StateID);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastEndState(int8 StateID);

	UFUNCTION(NetMulticast, Reliable)
	void NetMulticastSwitchState(int8 StateID);

	enum class ETransitionType : uint8
	{
		Undefined,
		BeginState,
		EndState,
		SwitchState
	};

	struct FCachedTransitionInfo
	{
		int8 ToStateID = INDEX_NONE;
		ETransitionType TransitionType = ETransitionType::Undefined;
	};

	TQueue<FCachedTransitionInfo> CachedTransitions;

	bool bTransitionsSuspended = false;
	void CacheTransitionWhileSuspended(int8 ToStateID, ETransitionType TransitionType);

	FString DebugSectionName;
	FString DebugStateMachineName;
	TMap<int8, FString> DebugStateNames;

	friend class UAutomationUtils;

	GENERATED_BODY()
};


//--------------------------------------------------------
//--------------------------------------------------------

// FSM network propagation mode

enum class UFSM::EMachineNetMode
{
	None, // no network propagation, server-side or client-side only FSM
	Autonomous, // autonomous client-side state machine, only starting state will be synchronized with server on initial replication
	Synchronized // fully controlled by server-side state machine
};


//--------------------------------------------------------
//--------------------------------------------------------

// FSM state network propagation mode
enum class UFSM::EStateNetMode
{
	None, // no network propagation, server-side only
	OwnClient, // server-side and own client
	AllClients // server-side and all clients
};


//--------------------------------------------------------
//--------------------------------------------------------

class UFSM::FState
{
public:
	EStateNetMode NetMode; // should be const, but not with unreal engine :(

	FState();

protected:
	virtual void Begin(bool bReplicated) = 0;
	virtual void End() = 0;
	virtual void Tick(float DeltaTime) = 0;

	friend UFSM;
};


//--------------------------------------------------------
//--------------------------------------------------------

class UFSM::FTransition
{
protected:
	virtual bool Tick() const = 0;

	friend UFSM;
};


//--------------------------------------------------------
//--------------------------------------------------------

template<class FOwnerClass>
class UFSM::TState : public UFSM::FState
{
public:
	using FBeginMethod = void (FOwnerClass::*)(bool bReplicated);
	using FMethod = void (FOwnerClass::*)();
	using FTickMethod = void (FOwnerClass::*)(float);

	TState();
	void Init(
		FOwnerClass* Owner,
		FMethod OwnerBeginMethod,
		FMethod OwnerEndMethod,
		FTickMethod OwnerTickMethod,
		EStateNetMode StateNetMode);
	void InitNet(
		FOwnerClass* Owner,
		FBeginMethod OwnerBeginMethod,
		FMethod OwnerEndMethod,
		FTickMethod OwnerTickMethod,
		EStateNetMode StateNetMode);

protected:
	FOwnerClass* Object;

	virtual void Begin(bool bReplicated) override;
	virtual void End() override;
	virtual void Tick(float DeltaTime) override;

private:
	FMethod BeginMethod;
	FBeginMethod ReplicatedBeginMethod;
	FMethod EndMethod;
	FTickMethod TickMethod;
};


//--------------------------------------------------------
//--------------------------------------------------------

template<class FOwnerClass>
class UFSM::TTransition : public FTransition
{
public:
	using FTickMethod = bool (FOwnerClass::*)() const;

	TTransition();
	void Init(FOwnerClass* Owner, FTickMethod OwnerTickMethod);

protected:
	virtual bool Tick() const override;

private:
	FOwnerClass* Object;
	FTickMethod TickMethod;
};


//--------------------------------------------------------
//--------------------------------------------------------

inline UFSM::FState::FState() : NetMode(UFSM::EStateNetMode::None)
{
}


//--------------------------------------------------------
//--------------------------------------------------------

template<class FOwnerClass>
inline UFSM::TState<FOwnerClass>::TState()
	: Object(nullptr)
	, BeginMethod(nullptr)
	, ReplicatedBeginMethod(nullptr)
	, EndMethod(nullptr)
	, TickMethod(nullptr)
{
}


template<class FOwnerClass>
inline void UFSM::TState<FOwnerClass>::Init(
	FOwnerClass* Owner,
	FMethod OwnerBeginMethod,
	FMethod OwnerEndMethod,
	FTickMethod OwnerTickMethod,
	EStateNetMode StateNetMode)
{
	Object = Owner;
	BeginMethod = OwnerBeginMethod;
	ReplicatedBeginMethod = nullptr;
	EndMethod = OwnerEndMethod;
	TickMethod = OwnerTickMethod;
	NetMode = StateNetMode;
}


template<class FOwnerClass>
inline void UFSM::TState<FOwnerClass>::InitNet(
	FOwnerClass* Owner,
	FBeginMethod OwnerReplicatedBeginMethod,
	FMethod OwnerEndMethod,
	FTickMethod OwnerTickMethod,
	EStateNetMode StateNetMode)
{
	Object = Owner;
	BeginMethod = nullptr;
	ReplicatedBeginMethod = OwnerReplicatedBeginMethod;
	EndMethod = OwnerEndMethod;
	TickMethod = OwnerTickMethod;
	NetMode = StateNetMode;
}


template<class FOwnerClass>
void UFSM::TState<FOwnerClass>::Begin(bool bReplicated)
{
	if (ReplicatedBeginMethod)
		(Object->*ReplicatedBeginMethod)(bReplicated);
	else if (BeginMethod)
		(Object->*BeginMethod)();
}


template<class FOwnerClass>
void UFSM::TState<FOwnerClass>::End()
{
	if (EndMethod)
		(Object->*EndMethod)();
}


template<class FOwnerClass>
void UFSM::TState<FOwnerClass>::Tick(float DeltaTime)
{
	if (TickMethod)
		(Object->*TickMethod)(DeltaTime);
}


//--------------------------------------------------------
//--------------------------------------------------------

template<class FOwnerClass>
inline UFSM::TTransition<FOwnerClass>::TTransition() : Object(nullptr)
													 , TickMethod(nullptr)
{
}


template<class FOwnerClass>
inline void UFSM::TTransition<FOwnerClass>::Init(FOwnerClass* Owner, FTickMethod OwnerTickMethod)
{
	Object = Owner;
	TickMethod = OwnerTickMethod;
}


template<class FOwnerClass>
bool UFSM::TTransition<FOwnerClass>::Tick() const
{
	return TickMethod ? (Object->*TickMethod)() : false;
}
