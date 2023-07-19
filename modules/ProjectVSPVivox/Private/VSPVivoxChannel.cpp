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
#include "VSPVivoxChannel.h"

#include "Async/Async.h"
#include "ChannelId.h"
#include "ILoginSession.h"
#include "ProjectVSPVivox.h"
#include "VSPVivox.h"
#include "VSPVivoxToken.h"

FChannelTextMessage::FChannelTextMessage(const FString& DisplayName, const IChannelTextMessage& Msg)
	: ReceivedTime(Msg.ReceivedTime())
	, Message(Msg.Message())
	, SenderId(Msg.Sender().Name())
	, SenderDisplayName(DisplayName)
	, Language(Msg.Language())
{
}

const FString& UVSPVivoxChannel::GetChannelName() const
{
	return ChannelName;
}

ChannelType UVSPVivoxChannel::GetChannelType() const
{
	return Type;
}

bool UVSPVivoxChannel::IsConnected() const
{
	if (!VivoxChannelId.IsValid())
		return false;

	auto VSPVivox = UVSPVivox::Get();
	if (!::IsValid(VSPVivox) || !VSPVivox->IsInitialized())
		return false;

	auto VivoxVoiceClient = VSPVivox->GetVivoxVoiceClient();
	if (!VivoxVoiceClient)
		return false;

	auto LoginSession = VSPVivox->GetLoginSession();
	if (!LoginSession)
		return false;
	if (LoginSession->State() != LoginState::LoggedIn)
		return false;

	auto ChannelSession = GetVivoxChannelSession();
	if (!ChannelSession)
		return false;

	return ChannelSession->ChannelState() == ConnectionState::Connected;
}

TArray<FString> UVSPVivoxChannel::GetParticipants() const
{
	TArray<FString> Result;

	auto ChannelSession = GetVivoxChannelSession();
	VSPvCheckReturn(ChannelSession, Result);

	for (auto Participant : ChannelSession->Participants())
	{
		VSPvCheckReturn(Participant.Value, Result);
		auto PlayerId = Participant.Value->Account().Name();
		auto Index = Result.AddUnique(PlayerId);
		if (Index != Result.Num() - 1)
			VSPV_LOG(Warning, "Channel '{}' has duplicates of PlayerId '{}'", *ToString(), *PlayerId);
	}

	return Result;
}

IParticipant* UVSPVivoxChannel::GetParticipant(FString PlayerId) const
{
	auto ChannelSession = GetVivoxChannelSession();
	VSPvCheckReturn(ChannelSession, nullptr);

	auto Participant = ChannelSession->Participants().Find(PlayerId);
	if (!Participant)
		return nullptr;
	VSPvCheckReturn(*Participant, nullptr);
	return *Participant;
}

void UVSPVivoxChannel::MuteParticipant(FString PlayerId, bool IsMuted)
{
	auto Participant = GetParticipant(PlayerId);
	VSPvCheckReturn(Participant);

	Participant->BeginSetLocalMute(IsMuted);
}

bool UVSPVivoxChannel::IsParticipantMuted(FString PlayerId)
{
	auto Participant = GetParticipant(PlayerId);
	VSPvCheckReturn(Participant, true);

	return Participant->LocalMute();
}

void UVSPVivoxChannel::SetVolumeParticipant(FString PlayerId, float Value) const
{
	IParticipant* Participant = GetParticipant(PlayerId);
	VSPvCheckReturn(Participant);

	Participant->SetLocalVolumeAdjustment(Value);
}

int32 UVSPVivoxChannel::GetVolumeParticipant(FString PlayerId) const
{
	IParticipant* Participant = GetParticipant(PlayerId);
	VSPvCheckReturn(Participant, 0);

	return Participant->LocalVolumeAdjustment();
}

bool UVSPVivoxChannel::IsInTransmission() const
{
	auto LoginSession = GetVivoxLoginSession();
	VSPvCheckReturn(LoginSession, false);

	return LoginSession->GetTransmissionMode() != TransmissionMode::None;
}

void UVSPVivoxChannel::StartTransmission() const
{
	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox);
	auto LoginSession = GetVivoxLoginSession();
	VSPvCheckReturn(LoginSession);

	if (IsInTransmission())
	{
		VSPV_LOG(Log, "Channel is in transmission. PlayerId '{}'. Channel '{}'", *VSPVivox->GetPlayerId(), *ToString());
	}
	else
	{
		VSPV_LOG(Verbose, "PlayerId '{}'. Channel '{}'", *VSPVivox->GetPlayerId(), *ToString());
	}
	CachedTransmissionMode = TransmissionMode::Single;
	LoginSession->SetTransmissionMode(TransmissionMode::Single, VivoxChannelId);
}

void UVSPVivoxChannel::StopTransmission() const
{
	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox);
	auto LoginSession = GetVivoxLoginSession();
	VSPvCheckReturn(LoginSession);

	if (!IsInTransmission())
	{
		VSPV_LOG(
			Log,
			"Channel is not in transmission. PlayerId '{}'. Channel '{}'",
			*VSPVivox->GetPlayerId(),
			*ToString());
	}
	else
	{
		VSPV_LOG(Verbose, "PlayerId '{}'. Channel '{}'", *VSPVivox->GetPlayerId(), *ToString());
	}
	CachedTransmissionMode = TransmissionMode::None;
	LoginSession->SetTransmissionMode(TransmissionMode::None, VivoxChannelId);
}

void UVSPVivoxChannel::Update3DPosition(AActor* Mumbler) const
{
	VSPvCheckReturn(Mumbler);

	CachedPosition.SetValue(Mumbler->GetActorLocation());
	CachedForwardVector.SetValue(Mumbler->GetActorForwardVector());
	CachedUpVector.SetValue(Mumbler->GetActorUpVector());

	// Return If there's no change from cached values.
	if (!CachedPosition.IsChanged() && !CachedForwardVector.IsChanged() && !CachedUpVector.IsChanged())
	{
		return;
	}

	// Set new position and orientation in connected positional channel.
	VSPV_LOG(
		VeryVerbose,
		"ChannelName '{}'. Position {}. ForwardVector {}. UpVector {}",
		*GetChannelName(),
		*CachedPosition.GetValue().ToCompactString(),
		*CachedForwardVector.GetValue().ToCompactString(),
		*CachedUpVector.GetValue().ToCompactString());

	auto ChannelSession = GetVivoxChannelSession();
	VSPvCheckReturn(ChannelSession);
	ChannelSession->Set3DPosition(
		CachedPosition.GetValue(),
		CachedPosition.GetValue(),
		CachedForwardVector.GetValue(),
		CachedUpVector.GetValue());
}

void UVSPVivoxChannel::SendMessage(const FString& Message) const
{
	VSPV_LOG(Verbose, "Message '{}'. Channel '{}'", *Message, *ToString());
	auto ChannelSession = GetVivoxChannelSession();
	VSPvCheckReturn(ChannelSession);
	ChannelSession->BeginSendText(Message);
}

ILoginSession* UVSPVivoxChannel::GetVivoxLoginSession() const
{
	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox, nullptr);
	return VSPVivox->GetLoginSession();
}

IChannelSession* UVSPVivoxChannel::GetVivoxChannelSession() const
{
	auto LoginSession = GetVivoxLoginSession();
	VSPvCheckReturn(LoginSession, nullptr);
	return &LoginSession->GetChannelSession(VivoxChannelId);
}

void UVSPVivoxChannel::Init(FString InChannelName, ChannelType InType, EChannelCommunicationType InCommunicationType)
{
	VSPvCheckReturn(!IsConnected());

	ChannelName = InChannelName;
	Type = InType;
	CommunicationType = InCommunicationType;

	static int32 DebugInstanceCount = 0;
	DebugInstanceId = DebugInstanceCount++;

	VSPV_LOG(Log, "Channel '{}'", *ToString());
}

void UVSPVivoxChannel::JoinChannel(FString BackendJoinToken)
{
	VSPvCheckReturn(!IsConnected());
	VSPvCheckReturn(!ChannelName.IsEmpty());
	VSPvCheckReturn(!bStartedConnecting);
	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox);
	auto LoginSession = GetVivoxLoginSession();
	VSPvCheckReturn(LoginSession);

	VSPV_LOG(Log, "PlayerId '{}'. Channel '{}'", *VSPVivox->GetPlayerId(), *ToString());

	// 3D properties
	auto Properties3D = Channel3DProperties(
		VSPVivox->Vivox3DPropertyAudibleDistance,
		VSPVivox->Vivox3DPropertyConversationalDistance,
		VSPVivox->Vivox3DPropertyAudioFadeIntensityByDistance,
		VSPVivox->Vivox3DPropertyAudioFadeModel);
	if (!Properties3D.IsValid())
		VSPV_LOG(Error, "Invalid Properties3D '{}'", *Properties3D.ToString());
	else
		VSPV_LOG(Verbose, "Properties3D '{}'", *Properties3D.ToString());

	// It's perfectly safe to add 3D properties to any channel type (they don't have any effect if the channel type is not Positional)
	auto Channel = ChannelId(VSPVivox->VivoxIssuer, ChannelName, VSPVivox->VivoxDomain, Type, Properties3D);
	auto& ChannelSession = LoginSession->GetChannelSession(Channel);

	FString JoinToken = BackendJoinToken;
	if (JoinToken.IsEmpty())
	{
		FVSPVivoxToken::GenerateClientJoinToken(ChannelSession, JoinToken);
	}
	VSPV_LOG(Verbose, "JoinToken '{}'", *JoinToken);

	bStartedConnecting = true;
	BindChannelSessionHandlers(false, ChannelSession);
	BindChannelSessionHandlers(true, ChannelSession);
	VSPVivox->OnDisconnect.RemoveAll(this);
	VSPVivox->OnDisconnect.AddUObject(this, &ThisClass::OnDisconnect);

	IChannelSession::FOnBeginConnectCompletedDelegate OnBeginConnectCompleteCallback;
	OnBeginConnectCompleteCallback.BindLambda(
		[This = MakeWeakObjectPtr(this), LoginSession, &ChannelSession](VivoxCoreError Status)
		{
			VSPvCheckReturn(This);
			VSPvCheckReturn(!This->IsConnected());
			VSPvCheckReturn(This->bStartedConnecting);

			This->bStartedConnecting = false;

			if (VxErrorSuccess != Status)
			{
				VSPV_LOG(
					Error,
					"Join failure for {}: {} ({})",
					*ChannelSession.Channel().Name(),
					ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)),
					Status);
				This->BindChannelSessionHandlers(false, ChannelSession); // Unbind handlers if we fail to join.
				LoginSession->DeleteChannelSession(
					ChannelSession.Channel()); // Disassociate this ChannelSession from the LoginSession.
				This->OnStateChanged.Broadcast(This.Get(), ConnectionState::Disconnected, true);
			}
			else
			{
				VSPV_LOG(Log, "Join success for '{}'. ", *ChannelSession.Channel().Name());
				VSPvCheckReturn(ChannelSession.Channel().IsValid());
				This->VivoxChannelId = ChannelSession.Channel();
				This->OnStateChanged.Broadcast(This.Get(), ConnectionState::Connected, false);
			}
		});

	// set switchTransition to true to avoid crush on start of PIE game
	bool bHasVoice = CommunicationType == EChannelCommunicationType::Voice;
	bool bHasText = CommunicationType == EChannelCommunicationType::Text;
	bool bHasSwitchTransmission = bHasText && !bHasVoice;
	auto Status = ChannelSession.BeginConnect(
		bHasVoice,
		bHasText,
		bHasSwitchTransmission,
		JoinToken,
		OnBeginConnectCompleteCallback);
	if (Status != VxErrorSuccess)
	{
		VSPV_LOG(
			Error,
			"BeginConnect call failed for channel '{}'. Error: {} ({})",
			*ToString(),
			ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)),
			Status);
		OnStateChanged.Broadcast(this, ConnectionState::Disconnected, true);
	}

	// timeout for vivox
	if (JoinTimeoutDelegateHandle.IsValid())
		FTicker::GetCoreTicker().RemoveTicker(JoinTimeoutDelegateHandle);
	JoinTimeoutDelegateHandle = FTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(
			this,
			[this](float TimeDelta)
			{
				VSPV_LOG(Error, "Join process stopped by timeout");

				JoinTimeoutDelegateHandle.Reset();
				OnStateChanged.Broadcast(this, ConnectionState::Disconnected, true);
				return false;
			}),
		TimeoutForJoinToVivoxS);
}

void UVSPVivoxChannel::OnDisconnect()
{
	VSPV_LOG(Log, "Channel '{}'", *ToString());

	if (auto LoginSession = GetVivoxLoginSession())
	{
		if (VivoxChannelId.IsValid())
			BindChannelSessionHandlers(false, LoginSession->GetChannelSession(VivoxChannelId));
	}

	LeaveChannel();

	VivoxChannelId = {};
}

void UVSPVivoxChannel::LeaveChannel()
{
	auto VSPVivox = UVSPVivox::Get();
	if (!IsValid(VSPVivox) || !VSPVivox->IsInitialized())
		return;

	VSPV_LOG(Log, "PlayerId '{}'. Channel '{}'", *VSPVivox->GetPlayerId(), *ToString());

	if (IsConnected())
	{
		VSPvCheckReturn(!bStartedDisconnecting);
		bStartedDisconnecting = true;

		auto LoginSession = GetVivoxLoginSession();
		if (!LoginSession)
		{
			VSPV_LOG(Log, "Login session has been already destroyed");
		}
		else
		{
			auto ChannelSession = LoginSession->ChannelSessions().Find(VivoxChannelId);
			if (!ChannelSession)
			{
				VSPV_LOG(Warning, "Vivox does not know the channel");
			}
			else
			{
				VSPV_LOG(Log, "Disconnecting from channel {}", *VivoxChannelId.Name());
				LoginSession->DeleteChannelSession(VivoxChannelId);
			}
		}
	}
}

FString UVSPVivoxChannel::ToString() const
{
	const FString& TypeName = UEnum::GetValueAsString(CommunicationType);
	return FString::Printf(
		TEXT("%s. Type %s. %s. IsConnected %i. DebugInstanceId %i"),
		*ChannelName,
		*UEnum::GetValueAsString(Type),
		*TypeName,
		IsConnected(),
		DebugInstanceId);
}

void UVSPVivoxChannel::BindChannelSessionHandlers(bool DoBind, IChannelSession& ChannelSession)
{
	VSPV_LOG(Verbose, "DoBind {}", (int32)DoBind);

	if (DoBind)
	{
		ChannelSession.EventAudioStateChanged.AddUObject(this, &ThisClass::AudioStateChanged);
		ChannelSession.EventTextStateChanged.AddUObject(this, &ThisClass::TextStateChanged);
		ChannelSession.EventChannelStateChanged.AddUObject(this, &ThisClass::ChannelStateChanged);
		ChannelSession.EventAfterParticipantAdded.AddUObject(this, &ThisClass::ChannelParticipantAdded);
		ChannelSession.EventBeforeParticipantRemoved.AddUObject(this, &ThisClass::ChannelParticipantRemoved);
		ChannelSession.EventAfterParticipantUpdated.AddUObject(this, &ThisClass::ChannelParticipantUpdated);
		if (CommunicationType == EChannelCommunicationType::Text)
			ChannelSession.EventTextMessageReceived.AddUObject(this, &ThisClass::ChannelTextMessageReceived);
	}
	else
	{
		ChannelSession.EventAudioStateChanged.RemoveAll(this);
		ChannelSession.EventTextStateChanged.RemoveAll(this);
		ChannelSession.EventChannelStateChanged.RemoveAll(this);
		ChannelSession.EventAfterParticipantAdded.RemoveAll(this);
		ChannelSession.EventBeforeParticipantRemoved.RemoveAll(this);
		ChannelSession.EventAfterParticipantUpdated.RemoveAll(this);
		if (CommunicationType == EChannelCommunicationType::Text)
			ChannelSession.EventTextMessageReceived.RemoveAll(this);
	}
}

void UVSPVivoxChannel::AudioStateChanged(const IChannelConnectionState& State)
{
	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox);
	VSPvCheckReturn(!VivoxChannelId.IsValid() || State.ChannelSession().Channel() == VivoxChannelId);

	VSPV_LOG(
		Verbose,
		"PlayerId '{}'. Channel '{}'. State '{}'",
		*VSPVivox->GetPlayerId(),
		*ToString(),
		*UEnum::GetValueAsString(State.State()));
}

void UVSPVivoxChannel::TextStateChanged(const IChannelConnectionState& State)
{
	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox);
	VSPvCheckReturn(!VivoxChannelId.IsValid() || State.ChannelSession().Channel() == VivoxChannelId);

	VSPV_LOG(
		Verbose,
		"PlayerId '{}'. Channel '{}'. State '{}'",
		*VSPVivox->GetPlayerId(),
		*ToString(),
		*UEnum::GetValueAsString(State.State()));
}

void UVSPVivoxChannel::ChannelStateChanged(const IChannelConnectionState& State)
{
	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox);
	VSPvCheckReturn(!VivoxChannelId.IsValid() || State.ChannelSession().Channel() == VivoxChannelId);

	VSPV_LOG(
		Log,
		"PlayerId '{}'. Channel '{}'. State '{}'",
		*VSPVivox->GetPlayerId(),
		*ToString(),
		*UEnum::GetValueAsString(State.State()));

	bool bUnexpectedChange = false;

	switch (State.State())
	{
	case ConnectionState::Connected:
	{
		if (!JoinTimeoutDelegateHandle.IsValid())
		{
			VSPV_LOG(Warning, "Connection time to the Vivox channel has exceeded the timeout");
		}
		FTicker::GetCoreTicker().RemoveTicker(JoinTimeoutDelegateHandle);
		JoinTimeoutDelegateHandle.Reset();
	}
	break;
	case ConnectionState::Disconnected:
	{
		bUnexpectedChange = !bStartedDisconnecting;

		auto LoginSession = GetVivoxLoginSession();
		VSPvCheckReturn(LoginSession);
		if (VivoxChannelId.IsValid())
			BindChannelSessionHandlers(false, LoginSession->GetChannelSession(VivoxChannelId));

		if (bUnexpectedChange)
		{
			VSPV_LOG(
				Warning,
				"Channel disconnected unexpectedly. It seemd like we expected a problem of connection to Vivox backend.");
		}
		else
		{
			VSPV_LOG(Log, "Channel disconnected, cleared and unbinded");
		}

		VivoxChannelId = ChannelId();
	}
	break;
	}

	AsyncTask(
		ENamedThreads::GameThread,
		[This = MakeWeakObjectPtr(this), StateOfState = State.State(), bUnexpectedChange]()
		{
			VSPvCheckReturn(This);
			This->OnStateChanged.Broadcast(This.Get(), StateOfState, bUnexpectedChange);
		});
}

void UVSPVivoxChannel::ChannelParticipantAdded(const IParticipant& Participant)
{
	auto PlayerId = Participant.Account().Name();
	VSPV_LOG(Verbose, "PlayerId '{}'. Channel '{}'", *PlayerId, *ToString());

	OnTeamChanged.Broadcast(this);
}

void UVSPVivoxChannel::ChannelParticipantRemoved(const IParticipant& Participant)
{
	auto PlayerId = Participant.Account().Name();
	VSPV_LOG(Verbose, "PlayerId '{}'. Channel '{}'", *PlayerId, *ToString());

	OnTeamChanged.Broadcast(this);
}

void UVSPVivoxChannel::ChannelParticipantUpdated(const IParticipant& Participant)
{
	auto PlayerId = Participant.Account().Name();
	VSPV_LOG(
		Verbose,
		"PlayerId '{}'. SpeechDetected {}. Channel '{}'",
		*PlayerId,
		(int32)Participant.SpeechDetected(),
		*ToString());

	OnTeamChanged.Broadcast(this);
}

void UVSPVivoxChannel::ChannelTextMessageReceived(const IChannelTextMessage& Message)
{
	if (!IsConnected())
		return; // ChannelSession call EventTextMessageReceived doesn't mater channel connected or not

	const FString& SenderId = Message.Sender().Name();
	IParticipant* Participant = GetParticipant(SenderId);
	VSPCheckReturn(Participant);

	VSPV_LOG(Verbose, "SenderId '{}'. Message {}. Channel '{}'", *SenderId, *Message.Message(), *ToString());

	OnMessageReceived.Broadcast(this, { Participant->Account().DisplayName(), Message });
}

void UVSPVivoxChannel::CrutchToFixVoiceTransmissionAfterJoinNewChannel() const
{
	if (CommunicationType == EChannelCommunicationType::Voice && IsConnected())
	{
		auto LoginSession = GetVivoxLoginSession();
		VSPvCheckReturn(LoginSession);

		VSPV_LOG(
			Log,
			"ChannelName '{}'. TransmissionMode {}",
			*ChannelName,
			*UEnum::GetValueAsString(CachedTransmissionMode));
		LoginSession->SetTransmissionMode(CachedTransmissionMode, VivoxChannelId);
	}
}
