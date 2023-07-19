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

#include "FSM.h"
#include "Net/UnrealNetwork.h"
#include "VSPCheck.h"
#include "Utility/Network/Network.h"


namespace FFSMLocal
{
	static constexpr int8 INVALID_STATE_ID = -42;
}


UFSM::UFSM()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	CurrentStateID = FFSMLocal::INVALID_STATE_ID;
	NetMode = EMachineNetMode::None;
	SetIsReplicatedByDefault(false);
}

void UFSM::AddStateInner(int8 StateID, FState* State)
{
	VSPCheck(!HasBegunPlay());
	VSPCheck(!States.Find(StateID));
	VSPCheck(StateID != FFSMLocal::INVALID_STATE_ID);
	FStateInfo StateInfo;
	StateInfo.State = State;
	States.Add(StateID, StateInfo);
}

void UFSM::AddTransitionInner(int8 FromStateID, int8 ToStateID, FTransition* Transition)
{
	VSPCheck(!HasBegunPlay());
	FStateInfo* StateInfo = States.Find(FromStateID);
	VSPCheckReturn(StateInfo);
	StateInfo->Transitions.Add(TTuple<int8, FTransition*>(ToStateID, Transition));
}

void UFSM::Setup(EMachineNetMode MachineNetMode, int8 StateID, bool ManualTick)
{
	VSPCheck(!HasBegunPlay());
	NetMode = MachineNetMode;
	SetIsReplicated(NetMode != EMachineNetMode::None);

	VSPCheck(!HasBegunPlay());
	VSPCheck(States.Find(StateID) != nullptr);
	VSPCheckF(CurrentStateID == FFSMLocal::INVALID_STATE_ID, TEXT("duplicate UFSM::SetStartingState call"));
	CurrentStateID = StateID;

	PrimaryComponentTick.bStartWithTickEnabled = !ManualTick;
	SetComponentTickEnabled(!ManualTick);
}

void UFSM::ForceTransitionInner(int8 ToStateID, bool bReEnter)
{
	VSPCheck(NetMode != EMachineNetMode::Synchronized || FNetworkUtility::HasAuthority(this));

	FStateInfo* PrevStateInfo = States.Find(CurrentStateID);
	VSPCheckReturn(PrevStateInfo);
	FStateInfo* NewStateInfo = States.Find(ToStateID);
	VSPCheckReturn(NewStateInfo);


	if (PrevStateInfo->State->NetMode == NewStateInfo->State->NetMode)
	{
		switch (PrevStateInfo->State->NetMode)
		{
		case EStateNetMode::None:
			break;
		case EStateNetMode::OwnClient:
			ClientSwitchState(ToStateID);
			break;
		case EStateNetMode::AllClients:
			NetMulticastSwitchState(ToStateID);
			break;
		}
	}
	else
	{
		switch (PrevStateInfo->State->NetMode)
		{
		case EStateNetMode::None:
			break;
		case EStateNetMode::OwnClient:
			ClientEndState(CurrentStateID);
			break;
		case EStateNetMode::AllClients:
			NetMulticastEndState(CurrentStateID);
			break;
		}

		switch (NewStateInfo->State->NetMode)
		{
		case EStateNetMode::None:
			break;
		case EStateNetMode::OwnClient:
			ClientBeginState(ToStateID);
			break;
		case EStateNetMode::AllClients:
			NetMulticastBeginState(ToStateID);
			break;
		}
	}

	const bool bSameState = CurrentStateID == ToStateID;
	if (!bSameState || bReEnter)
	{
		PrevStateInfo->State->End();
		CurrentStateID = ToStateID;
		NewStateInfo->State->Begin(false);
	}
}

void UFSM::SetupReplication(EMachineNetMode MachineNetMode)
{
	VSPCheck(!HasBegunPlay());
	NetMode = MachineNetMode;
	SetIsReplicated(NetMode != EMachineNetMode::None);
}

void UFSM::SetStartingStateInner(int8 StateID)
{
	VSPCheck(!HasBegunPlay());
	VSPCheck(States.Find(StateID) != nullptr);
	VSPCheckF(CurrentStateID == FFSMLocal::INVALID_STATE_ID, TEXT("duplicate UFSM::SetStartingState call"));
	CurrentStateID = StateID;
}

void UFSM::ManualTick(float DeltaTime)
{
	TickState(DeltaTime);
}

void UFSM::SuspendTransitions()
{
	VSPCheck(!bTransitionsSuspended);
	bTransitionsSuspended = true;
}

void UFSM::ResumeTransitions()
{
	VSPCheck(bTransitionsSuspended);
	bTransitionsSuspended = false;

	while (!CachedTransitions.IsEmpty())
	{
		FCachedTransitionInfo TransitionToPerform;
		const bool bDequeueSuccessful = CachedTransitions.Dequeue(TransitionToPerform);
		VSPCheck(bDequeueSuccessful);

		switch (TransitionToPerform.TransitionType)
		{
		case ETransitionType::BeginState:
			BeginState(TransitionToPerform.ToStateID);
			break;
		case ETransitionType::EndState:
			EndState(TransitionToPerform.ToStateID);
			break;
		case ETransitionType::SwitchState:
			SwitchState(TransitionToPerform.ToStateID);
			break;
		default:
			VSPNoEntry();
		}
	}
}

void UFSM::BeginPlay()
{
	Super::BeginPlay();
	VSPCheck(States.Num() > 0);
	VSPCheckF(CurrentStateID != FFSMLocal::INVALID_STATE_ID, TEXT("did you forget to call UFSM::SetStartingState?"));
	FStateInfo* StateInfo = States.Find(CurrentStateID);
	VSPCheckReturn(StateInfo);

	if (FNetworkUtility::HasAuthority(this) || NetMode == EMachineNetMode::None)
	{
		StateInfo->State->Begin(false);
		bValidClientState = true;
	}
	else if (
		NetMode == EMachineNetMode::Autonomous || StateInfo->State->NetMode == EStateNetMode::AllClients
		|| (StateInfo->State->NetMode == EStateNetMode::OwnClient && FNetworkUtility::IsClientOwnership(this)))
	{
		StateInfo->State->Begin(true);
		bValidClientState = true;
	}

	bool bSynchronized = NetMode == EMachineNetMode::Synchronized;
	bool bHasReplicatedStates = false;
	for (TPair<int8, FStateInfo> const& It : States)
	{
		if (It.Value.State->NetMode != EStateNetMode::None)
			bHasReplicatedStates = true;
	}
	if (!bSynchronized && bHasReplicatedStates)
		VSPNoEntryF(TEXT("non-synchronized fsm has replicated states"));
	if (bSynchronized && !bHasReplicatedStates)
		VSPNoEntryF(TEXT("syncrhonized fsm has no replicated states"));
}

void UFSM::EndPlay(EEndPlayReason::Type const EndPlayReason)
{
	FStateInfo* StateInfo = States.Find(CurrentStateID);
	VSPCheckReturn(StateInfo);
	if (NetMode != EMachineNetMode::Synchronized || FNetworkUtility::HasAuthority(this)
		|| (bValidClientState
			&& (StateInfo->State->NetMode == EStateNetMode::AllClients
				|| (StateInfo->State->NetMode == EStateNetMode::OwnClient
					&& FNetworkUtility::IsClientOwnership(this)))))
	{
		StateInfo->State->End();
		bValidClientState = false;
	}
	Super::EndPlay(EndPlayReason);
}

void UFSM::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	TickState(DeltaTime);
}

void UFSM::TickState(float DeltaTime)
{
	FStateInfo* StateInfo = States.Find(CurrentStateID);
	VSPCheckReturn(StateInfo);

	if (NetMode != EMachineNetMode::Synchronized || FNetworkUtility::HasAuthority(this))
	{
		StateInfo->State->Tick(DeltaTime);
		for (TTuple<int8, FTransition*> const& Transition : StateInfo->Transitions)
		{
			if (Transition.Get<1>()->Tick())
			{
				ForceTransitionInner(Transition.Get<0>(), false);
				break;
			}
		}
	}
	else
	{
		if (bValidClientState)
			StateInfo->State->Tick(DeltaTime);
	}

}

void UFSM::BeginState(int8 StateID)
{
	FStateInfo* StateInfo = States.Find(StateID);
	VSPCheckReturn(StateInfo);
	StateInfo->State->Begin(false);
	CurrentStateID = StateID;
	bValidClientState = true;
}

void UFSM::EndState(int8 StateID)
{
	FStateInfo* StateInfo = States.Find(StateID);
	VSPCheckReturn(StateInfo);
	VSPCheck(bValidClientState);
	StateInfo->State->End();
	bValidClientState = false;
}

void UFSM::SwitchState(int8 StateID)
{
	FStateInfo* StateInfo = States.Find(CurrentStateID);
	VSPCheckReturn(StateInfo);
	if (bValidClientState)
		StateInfo->State->End();
	CurrentStateID = StateID;

	StateInfo = States.Find(StateID);
	VSPCheckReturn(StateInfo);
	StateInfo->State->Begin(false);

	bValidClientState = true;
}

void UFSM::CacheTransitionWhileSuspended(int8 ToStateID, ETransitionType TransitionType)
{
	VSPCheck(bTransitionsSuspended);
	CachedTransitions.Enqueue({ ToStateID, TransitionType });
}

void UFSM::ClientBeginState_Implementation(int8 StateID)
{
	if (!FNetworkUtility::HasAuthority(this))
	{
		if (bTransitionsSuspended)
			CacheTransitionWhileSuspended(StateID, ETransitionType::BeginState);
		else
			BeginState(StateID);
	}
}

void UFSM::ClientEndState_Implementation(int8 StateID)
{
	if (!FNetworkUtility::HasAuthority(this))
	{
		if (bTransitionsSuspended)
			CacheTransitionWhileSuspended(StateID, ETransitionType::EndState);
		else
			EndState(StateID);
	}
}


void UFSM::ClientSwitchState_Implementation(int8 StateID)
{
	if (!FNetworkUtility::HasAuthority(this))
	{
		if (bTransitionsSuspended)
			CacheTransitionWhileSuspended(StateID, ETransitionType::SwitchState);
		else
			SwitchState(StateID);
	}
}

void UFSM::NetMulticastBeginState_Implementation(int8 StateID)
{
	if (!FNetworkUtility::HasAuthority(this))
	{
		if (bTransitionsSuspended)
			CacheTransitionWhileSuspended(StateID, ETransitionType::BeginState);
		else
			BeginState(StateID);
	}
}

void UFSM::NetMulticastEndState_Implementation(int8 StateID)
{
	if (!FNetworkUtility::HasAuthority(this))
	{
		if (bTransitionsSuspended)
			CacheTransitionWhileSuspended(StateID, ETransitionType::EndState);
		else
			EndState(StateID);
	}
}


void UFSM::NetMulticastSwitchState_Implementation(int8 StateID)
{
	if (!FNetworkUtility::HasAuthority(this))
	{
		if (bTransitionsSuspended)
			CacheTransitionWhileSuspended(StateID, ETransitionType::SwitchState);
		else
			SwitchState(StateID);
	}
}


void UFSM::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME_CONDITION(UFSM, CurrentStateID, COND_InitialOnly);
}
