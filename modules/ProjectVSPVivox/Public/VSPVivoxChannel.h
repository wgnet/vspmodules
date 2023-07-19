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

#include "ChannelId.h"
#include "ConnectionState.h"
#include "UObject/Object.h"
#include "VSPVivoxChannel.generated.h"



class ILoginSession;
class UVSPVivox;
class IChannelTextMessage;
class IChannelConnectionState;
class IParticipant;
class IChannelSession;
enum class TransmissionMode : uint8;


class FCachedVector
{
public:
	const FVector& GetValue()
	{
		bChanged = false;
		return Value;
	}

	void SetValue(const FVector& NewValue)
	{
		if ((Value - NewValue).IsNearlyZero())
			return;

		Value = NewValue;
		bChanged = true;
	}

	bool IsChanged() const
	{
		return bChanged;
	}

private:
	bool bChanged = false;
	FVector Value = FVector::ZeroVector;
};

USTRUCT()
struct PROJECTVSPVIVOX_API FChannelTextMessage
{
	GENERATED_BODY()

	FChannelTextMessage() = default;
	FChannelTextMessage(const FString& DisplayName, const IChannelTextMessage& Message);

	UPROPERTY()
	FDateTime ReceivedTime = { 0 };

	UPROPERTY()
	FString Message;

	UPROPERTY()
	FString SenderId;

	UPROPERTY()
	FString SenderDisplayName;

	UPROPERTY()
	FString Language;
};

UENUM()
enum class EChannelCommunicationType : uint8
{
	Voice,
	Text
};

UCLASS(Config = Game)
class PROJECTVSPVIVOX_API UVSPVivoxChannel : public UObject
{
	friend class UVSPVivox;
	GENERATED_BODY()
public:
	//- delegates ---------------------------------------------------------------------------
	DECLARE_MULTICAST_DELEGATE_ThreeParams(FOnStateChanged, UVSPVivoxChannel* Channel, ConnectionState State, bool bUnexpectedChange);
	FOnStateChanged OnStateChanged;
	DECLARE_MULTICAST_DELEGATE_OneParam(FOnTeamChanged, UVSPVivoxChannel* Channel);
	FOnTeamChanged OnTeamChanged;
	DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMessageReceived, UVSPVivoxChannel* Channel, const FChannelTextMessage& Message);
	FOnMessageReceived OnMessageReceived;


	//- functions ---------------------------------------------------------------------------
	void LeaveChannel();

	const FString& GetChannelName() const;
	ChannelType GetChannelType() const;
	bool IsConnected() const;

	TArray<FString> GetParticipants() const;
	IParticipant* GetParticipant(FString PlayerId) const;
	void MuteParticipant(FString PlayerId, bool IsMuted);
	bool IsParticipantMuted(FString PlayerId);
	void SetVolumeParticipant(FString PlayerId, float Value) const;
	int GetVolumeParticipant(FString PlayerId) const;
	bool IsInTransmission() const;
	void StartTransmission() const;
	void StopTransmission() const;
	void Update3DPosition(AActor* Mumbler) const;

	void SendMessage(const FString& Message) const;

	FString ToString() const;
	void JoinChannel(FString BackendJoinToken = "");

private:
	//- constants ---------------------------------------------------------------------------
	UPROPERTY(Config)
	float TimeoutForJoinToVivoxS = 30;


	//- constants ---------------------------------------------------------------------------
	FString ChannelName;
	ChannelType Type = ChannelType::NonPositional;
	EChannelCommunicationType CommunicationType = EChannelCommunicationType::Voice;
	bool bStartedConnecting = false;
	bool bStartedDisconnecting = false;
	ChannelId VivoxChannelId;

	mutable FCachedVector CachedPosition;
	mutable FCachedVector CachedForwardVector;
	mutable FCachedVector CachedUpVector;
	mutable TransmissionMode CachedTransmissionMode;

	int32 DebugInstanceId = 0;

	FDelegateHandle JoinTimeoutDelegateHandle;


	//- functions ---------------------------------------------------------------------------
	ILoginSession* GetVivoxLoginSession() const;
	IChannelSession* GetVivoxChannelSession() const;

	void Init(FString InChannelName, ChannelType InType, EChannelCommunicationType InCommunicationType);

	UFUNCTION()
	void OnDisconnect();

	void BindChannelSessionHandlers(bool DoBind, IChannelSession& ChannelSession);
	void AudioStateChanged(const IChannelConnectionState& State);
	void TextStateChanged(const IChannelConnectionState& State);
	void ChannelStateChanged(const IChannelConnectionState& State);
	void ChannelParticipantAdded(const IParticipant& Participant);
	void ChannelParticipantRemoved(const IParticipant& Participant);
	void ChannelParticipantUpdated(const IParticipant& Participant);
	void ChannelTextMessageReceived(const IChannelTextMessage& Message);

	void CrutchToFixVoiceTransmissionAfterJoinNewChannel() const;
};
