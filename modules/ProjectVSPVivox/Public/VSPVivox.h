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

#include "AccountId.h"
#include "VSPVivoxChannel.h"

#include "VSPVivox.generated.h"

enum class LoginState : uint8;
enum class EAudioFadeModel : uint8;
class IClient;
class IAudioDevice;
class UVSPVivoxChannel;


DECLARE_DELEGATE_FiveParams(FVoiceRoutingCallbackDelegate, int16*, int32, int32, int32, bool);
DECLARE_MULTICAST_DELEGATE_OneParam(FOnShouldReconectChannelAssync, UVSPVivoxChannel*);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnVivoxLogginSucces);

UCLASS(Config = Game)
class PROJECTVSPVIVOX_API UVSPVivox : public UObject
{
	friend UVSPVivoxChannel;
	GENERATED_BODY()
public:
	//- delegates ---------------------------------------------------------------------------
	DECLARE_MULTICAST_DELEGATE(FOnReconnect);
	FOnReconnect OnDisconnect;
	FOnReconnect OnReconnect;

	DECLARE_MULTICAST_DELEGATE(FOnInputDevicesUpdated)
	FOnInputDevicesUpdated OnInputDevicesUpdated;

	//- functions ---------------------------------------------------------------------------
	static UVSPVivox* Get();

	bool Initialize(const FString& PlayerId, const FString& DisplayName);
	void Uninitialize();
	bool IsInitialized() const;

	bool IsVoiceRoutingEnabled() const;
	void SetVoiceRoutingCallback(const FVoiceRoutingCallbackDelegate& Callback);
	bool IsVoiceRoutingCallbackBounded() const;
	int32 GetChannelsCount() const;

	const AccountId& GetAccountId() const;
	FString GetPlayerId() const;
	FString GetDisplayName() const;

	UVSPVivoxChannel* JoinChannel(
		const FString& ChannelName,
		ChannelType Type,
		EChannelCommunicationType CommunicationType,
		FString BackendJoinToken);

	FString GetVivoxKey() const;

	void SetInputVolume(float Value) const;
	void SetOutputVolume(float Value) const;

	void RefreshInputDevices() const;
	int32 GetInputDevices(TArray<FString>& Devices);
	void SetInputDevice(const FString& DeviceName);

	void Reconnect();
	LoginState GetLoginState() const;
	bool Login(const FString BackendLoginToken = TEXT(""));
	bool IsLoggedIn();
	void Logout();
	int32 RemoveChannel(UVSPVivoxChannel* Channel);
	FOnShouldReconectChannelAssync OnShouldReconectChannelAsync;
	FOnVivoxLogginSucces OnLoginSuccess;

protected:
	//- functions ---------------------------------------------------------------------------
	virtual void BeginDestroy() override;

private:
	//- types -------------------------------------------------------------------------------
	struct FVoiceRoutingCallbackHolder;

	//- constants ---------------------------------------------------------------------------
	UPROPERTY(Config)
	FString VivoxServer;

	UPROPERTY(Config)
	FString VivoxDomain;

	UPROPERTY(Config)
	FString VivoxIssuer;

	UPROPERTY(Config)
	FString VivoxKey;

	UPROPERTY(Config)
	int32 VivoxLogLevel =
		1; // logLevel: -1 = no logging, 0 = errors only, 1 = warnings, 2 = info, 3 = debug, 4 = trace, 5 = log all

	UPROPERTY(Config)
	int32 Vivox3DPropertyAudibleDistance = 1;

	UPROPERTY(Config)
	int32 Vivox3DPropertyConversationalDistance = 1;

	UPROPERTY(Config)
	double Vivox3DPropertyAudioFadeIntensityByDistance = 1;

	UPROPERTY(Config)
	EAudioFadeModel Vivox3DPropertyAudioFadeModel;

	UPROPERTY(Config)
	float TimeoutBeforeStartLogoutFromVivoxS = 15;

	UPROPERTY(Config)
	bool bVoiceRoutingEnabled = false;

	UPROPERTY(Config)
	float TimeoutForReconnectS = 10;

	UPROPERTY(Config)
	float TimeoutForLoginToVivoxS = 30;


	//- variables ---------------------------------------------------------------------------
	AccountId VivoxAccountId;

	UPROPERTY()
	TArray<UVSPVivoxChannel*> Channels;
	FDelegateHandle LogoutDelegateHandle;
	FDelegateHandle ReconnectDelegateHandle;
	FDelegateHandle LoginTimeoutDelegateHandle;

	FVoiceRoutingCallbackDelegate VoiceRoutingCallback;

	TArray<const IAudioDevice*> InputDevices;

	//- functions ---------------------------------------------------------------------------
	static IClient* GetVivoxVoiceClient();
	ILoginSession* GetLoginSession() const;

	void LoginSessionStateChanged(LoginState State);
	void LoginComplete(int32 Status);

	void OnInputDeviceAdded(const IAudioDevice& NewDevice);
	void OnInputDeviceRemoved(const IAudioDevice& OldDevice);
	void OnEffectiveInputDeviceChanged(const IAudioDevice& Device);
	void AddChannel(UVSPVivoxChannel* Channel);
	UVSPVivoxChannel* GetChannelByIndex(int32 ChannelIndex);
	void PrintCurrentChannelsInfo() const;

	UFUNCTION()
	void ChannelStateChanged(UVSPVivoxChannel* Channel, ConnectionState State, bool bUnexpectedChange);

	UFUNCTION()
	void DoLogout();
	void BindLoginHandlers(bool DoBind, ILoginSession& LoginSession);
};
