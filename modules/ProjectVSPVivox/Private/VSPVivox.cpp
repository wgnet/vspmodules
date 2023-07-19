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
#include "VSPVivox.h"

#include "Async/Async.h"
#include "ProjectVSPVivox.h"
#include "VSPVivoxToken.h"
#include "VivoxConfig.h"
#include "VivoxCore.h"

#if WITH_EDITOR
	#include "Editor.h"
#endif

VSP_DEFINE_FILE_CATEGORY(VSPVivoxLog);

static const FString DefaultSystemDeviceName = TEXT("Default System Device");
static const FString DefaultCommunicationDeviceName = TEXT("Default Communication Device");
static const FString NoDevices = TEXT("No Devices");

struct UVSPVivox::FVoiceRoutingCallbackHolder
{
	static void Callback(
		void* CallbackHandle,
		const char* SessionGroupHandle,
		const char* InitialTargetUri,
		short* PCMFrames,
		int PCMFrameCount,
		int AudioFrameRate,
		int ChannelsPerFrame,
		int IsSilence)
	{
		if (UVSPVivox* VSPVivox = static_cast<UVSPVivox*>(CallbackHandle))
		{
			VSPVivox->VoiceRoutingCallback.ExecuteIfBound(
				static_cast<int16*>(PCMFrames),
				static_cast<int32>(PCMFrameCount),
				static_cast<int32>(AudioFrameRate),
				static_cast<int32>(ChannelsPerFrame),
				static_cast<bool>(IsSilence));
		}
		else
		{
			VSP_LOG(Warning, "FVoiceRoutingCallback Unsuccessful cast to VSPVivox");
		}
	}
};

UVSPVivox* UVSPVivox::Get()
{
	auto ProjectVSPVivox = FProjectVSPVivox::Get();
	VSPvCheckReturn(ProjectVSPVivox, nullptr);
	return ProjectVSPVivox->GetVSPVivox();
}

bool UVSPVivox::Initialize(const FString& PlayerId, const FString& DisplayName)
{
	// check vivox dlls existence
	static FName VivoxCoreName = TEXT("VivoxCore");
	static FName VivoxVoiceName = TEXT("VivoxVoice");
	auto VivoxCoreModule = FModuleManager::GetModulePtr<FVivoxCoreModule>(VivoxCoreName);
	VSPCheckReturnF(VivoxCoreModule, TEXT("VivoxCore module does not exists"), false);
	auto VivoxCoreModuleNum = IModularFeatures::Get().GetModularFeatureImplementationCount(VivoxVoiceName);
	VSPCheckReturnF(
		VivoxCoreModuleNum == 1,
		*FString::Printf(
			TEXT(
				"VivoxCore module num (%i) != 1. Check for VivoxCore plugin and its version. get_binaries might help."),
			VivoxCoreModuleNum),
		false);
	auto VivoxCoreModularFeature = IModularFeatures::Get().GetModularFeatureImplementation(VivoxVoiceName, 0);
	VSPCheckReturnF(
		VivoxCoreModularFeature,
		TEXT(
			"VivoxCore modular features are not registered. Check for VivoxCore plugin and its version. get_binaries might help."),
		false);

	auto VivoxVoiceClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxVoiceClient, false);

	VSPV_LOG(
		Log,
		"PlayerId '{}'. VivoxServer '{}'. VivoxDomain '{}'. VivoxIssuer '{}'",
		*PlayerId,
		*VivoxServer,
		*VivoxDomain,
		*VivoxIssuer);

	// check settings
	VSPvCheckReturn(!VivoxServer.IsEmpty(), false);
	VSPvCheckReturn(!VivoxDomain.IsEmpty(), false);
	VSPvCheckReturn(!VivoxIssuer.IsEmpty(), false);
	VSPvCheckReturn(!PlayerId.IsEmpty(), false);

	// init voice client
	VivoxConfig Config;
	Config.SetCallbackHandle(this);

	Config.SetLogLevel((vx_log_level)VivoxLogLevel);

	VSPV_LOG(Display, "VoiceRouting is enabled {}", (int32)bVoiceRoutingEnabled);

	if (bVoiceRoutingEnabled)
		Config.SetOnAudioUnitBeforeRecvAudioRendered(FVoiceRoutingCallbackHolder::Callback);

	auto Status = VivoxVoiceClient->Initialize(Config);
	if (Status == VxErrorAlreadyInitialized)
	{
		VSPV_LOG(Warning, "Vivox has been already initialized");
	}
	else if (Status != VxErrorSuccess)
	{
		VSPV_LOG(
			Error,
			"Initialize failed: {} ({}). Start reconnection process.",
			ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)),
			Status);
		Reconnect();
		return false;
	}

	// init variables
	VivoxAccountId = AccountId(VivoxIssuer, PlayerId, VivoxDomain, DisplayName);

	// bind callbacks
	auto& LoginSession = VivoxVoiceClient->GetLoginSession(VivoxAccountId);
	BindLoginHandlers(true, LoginSession);

	IAudioDevices& ClientInputDevices = VivoxVoiceClient->AudioInputDevices();
	ClientInputDevices.EventAfterDeviceAvailableAdded.AddUObject(this, &ThisClass::OnInputDeviceAdded);
	ClientInputDevices.EventBeforeAvailableDeviceRemoved.AddUObject(this, &ThisClass::OnInputDeviceRemoved);
	ClientInputDevices.EventEffectiveDeviceChanged.AddUObject(this, &ThisClass::OnEffectiveInputDeviceChanged);

	return IsInitialized();
}

void UVSPVivox::Uninitialize()
{
	if (!VivoxAccountId.IsValid())
		return;

	VSPV_LOG(Log, "");

	DoLogout();

	IClient* VivoxClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxClient);

	IAudioDevices& ClientInputDevices = VivoxClient->AudioInputDevices();
	ClientInputDevices.EventAfterDeviceAvailableAdded.RemoveAll(this);
	ClientInputDevices.EventBeforeAvailableDeviceRemoved.RemoveAll(this);
	ClientInputDevices.EventEffectiveDeviceChanged.RemoveAll(this);

	// uninit
	VivoxClient->Uninitialize();
}

bool UVSPVivox::IsInitialized() const
{
	return VivoxAccountId.IsValid();
}

bool UVSPVivox::IsVoiceRoutingEnabled() const
{
	return bVoiceRoutingEnabled;
}

void UVSPVivox::SetVoiceRoutingCallback(const FVoiceRoutingCallbackDelegate& Callback)
{
	if (!bVoiceRoutingEnabled)
		return;

	VSPV_LOG(Log, "");

	VoiceRoutingCallback = Callback;
}

bool UVSPVivox::IsVoiceRoutingCallbackBounded() const
{
	return VoiceRoutingCallback.IsBound();
}

int32 UVSPVivox::GetChannelsCount() const
{
	return Channels.Num();
}

const AccountId& UVSPVivox::GetAccountId() const
{
	return VivoxAccountId;
}

FString UVSPVivox::GetPlayerId() const
{
	VSPvCheckReturn(IsInitialized(), FString());
	return VivoxAccountId.Name();
}

FString UVSPVivox::GetDisplayName() const
{
	VSPvCheckReturn(IsInitialized(), FString());
	return VivoxAccountId.DisplayName();
}

UVSPVivoxChannel* UVSPVivox::JoinChannel(
	const FString& ChannelName,
	ChannelType Type,
	EChannelCommunicationType CommunicationType,
	FString BackendJoinToken)
{
	if (!IsInitialized())
	{
		VSPvCheckReturn(GIsEditor || FApp::IsStandalone(), nullptr);
		return nullptr;
	}

	VSPV_LOG(
		Log,
		"PlayerId '{}'. ChannelName '{}'. ChannelType {}",
		*GetPlayerId(),
		*ChannelName,
		*UEnum::GetValueAsString(Type));

	// stop logout process
	if (LogoutDelegateHandle.IsValid())
	{
		FTicker::GetCoreTicker().RemoveTicker(LogoutDelegateHandle);
		LogoutDelegateHandle.Reset();
	}

	// create new one
	auto Channel = NewObject<UVSPVivoxChannel>(this);
	Channel->Init(ChannelName, Type, CommunicationType);
	AddChannel(Channel);
	Channel->OnStateChanged.AddUObject(this, &ThisClass::ChannelStateChanged);

	// check login state. skip login if it is already has been started
	switch (GetLoginState())
	{
	case LoginState::LoggingIn:
	{
		VSPV_LOG(Verbose, "Is logging in.");
		return Channel;
	}
	case LoginState::LoggedIn:
	{
		VSPV_LOG(Verbose, "Already logged in.");
		Channel->JoinChannel(BackendJoinToken);
		return Channel;
	}
	}

	// login process has not been started. do it
	if (!Login())
		return nullptr;

	return Channel;
}

FString UVSPVivox::GetVivoxKey() const
{
	return VivoxKey;
}

void UVSPVivox::SetInputVolume(float Value) const
{
	if (!IsInitialized())
	{
		VSPV_LOG(Warning, "Vivox is not initialized yet");
		return;
	}

	VSPV_LOG(Log, "Value {:.3f}", Value);

	auto VivoxVoiceClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxVoiceClient);

	auto VivoxValue = Value * 100 - 50;
	auto Status = VivoxVoiceClient->AudioInputDevices().SetVolumeAdjustment(VivoxValue);
	if (Status != VxErrorSuccess)
		VSPV_LOG(Error, "Vivox error on change {} ({})", ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
}

void UVSPVivox::SetOutputVolume(float Value) const
{
	if (!IsInitialized())
	{
		VSPV_LOG(Warning, "Vivox is not initialized yet");
		return;
	}

	VSPV_LOG(Log, "Value {:.3f}", Value);

	auto VivoxVoiceClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxVoiceClient);

	auto VivoxValue = Value * 100 - 50;
	auto Status = VivoxVoiceClient->AudioOutputDevices().SetVolumeAdjustment(VivoxValue);
	if (Status != VxErrorSuccess)
		VSPV_LOG(Error, "Vivox error on change {} ({})", ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)), Status);
}

void UVSPVivox::RefreshInputDevices() const
{
	if (!IsInitialized())
	{
		VSPV_LOG(Warning, "Vivox is not initialized yet");
		return;
	}

	VSPV_LOG(Log, "");

	IClient* VivoxClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxClient);

	VivoxClient->AudioInputDevices().Refresh();
}

int32 UVSPVivox::GetInputDevices(TArray<FString>& Devices)
{
	if (!IsInitialized())
	{
		VSPV_LOG(Warning, "Vivox is not initialized yet");
		return INDEX_NONE;
	}

	IClient* VivoxClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxClient, INDEX_NONE);

	IAudioDevices& ClientInputDevices = VivoxClient->AudioInputDevices();
	const IAudioDevice& EffectiveDevice = ClientInputDevices.EffectiveDevice();

	VSPvCheck(Devices.Num() == 0);

	if (EffectiveDevice.Id() == ClientInputDevices.NullDevice().Id())
	{
		Devices.Add(NoDevices);
		return 0;
	}

	int32 CurrentDeviceIndex = INDEX_NONE;
	for (int32 Index = 0; Index < InputDevices.Num(); ++Index)
	{
		if (InputDevices[Index]->Id() == EffectiveDevice.Id())
			CurrentDeviceIndex = Index;

		Devices.Add(InputDevices[Index]->Name());
	}

	return CurrentDeviceIndex;
}

void UVSPVivox::SetInputDevice(const FString& DeviceName)
{
	if (!IsInitialized())
	{
		VSPV_LOG(Warning, "Vivox is not initialized yet");
		return;
	}

	VSPV_LOG(Log, "DeviceName {}", *DeviceName);

	if (DeviceName == NoDevices)
		return;

	IClient* VivoxClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxClient);

	IAudioDevices& ClientInputDevices = VivoxClient->AudioInputDevices();

	for (const IAudioDevice* Device : InputDevices)
	{
		if (Device->Name() == DeviceName)
		{
			VivoxCoreError Status = ClientInputDevices.SetActiveDevice(*Device);
			if (Status != VxErrorSuccess)
				VSPV_LOG(
					Error,
					"Vivox error on change {} ({})",
					ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)),
					Status);
		}
	}
}

void UVSPVivox::BeginDestroy()
{
	VSPV_LOG(Log, "");
	Uninitialize();

	UObject::BeginDestroy();
}

IClient* UVSPVivox::GetVivoxVoiceClient()
{
	auto VivoxCore = static_cast<FVivoxCoreModule*>(FModuleManager::Get().LoadModule(TEXT("VivoxCore")));
	VSPvCheckReturn(VivoxCore, nullptr);
	return &VivoxCore->VoiceClient();
}

ILoginSession* UVSPVivox::GetLoginSession() const
{
	auto VivoxVoiceClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxVoiceClient, nullptr);
	VSPvCheckReturn(VivoxAccountId.IsValid(), nullptr);

	return &VivoxVoiceClient->GetLoginSession(VivoxAccountId);
}

LoginState UVSPVivox::GetLoginState() const
{
	auto VivoxVoiceClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxVoiceClient, LoginState::LoggedOut);

	if (!VivoxAccountId.IsValid())
		return LoginState::LoggedOut;

	auto& LoginSession = VivoxVoiceClient->GetLoginSession(VivoxAccountId);
	return LoginSession.State();
}

bool UVSPVivox::Login(const FString BackendLoginToken)
{
	auto VivoxVoiceClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxVoiceClient, false);
	VSPvCheckReturn(IsInitialized(), false);

	VSPV_LOG(Verbose, "Start logging in");

	auto& LoginSession = VivoxVoiceClient->GetLoginSession(VivoxAccountId);

	// check current login session state
	if (GetLoginState() == LoginState::LoggedIn)
	{
		VSPV_LOG(Log, "VivoxVoiceClient has already logged in for '{}'", *VivoxAccountId.ToString());
		return true;
	}
	if (GetLoginState() != LoginState::LoggedOut)
	{
		VSPV_LOG(
			Log,
			"VivoxVoiceClient login state is '{}' for '{}'. Do logout.",
			*UEnum::GetValueAsString(LoginSession.State()),
			*VivoxAccountId.ToString());
		LoginSession.Logout();
	}

	// make a token
	FString LoginToken;
	if (BackendLoginToken.IsEmpty())
	{
		FVSPVivoxToken::GenerateClientLoginToken(LoginSession, LoginToken);
		VSPV_LOG(Log, "Empty token, generate token to client side");
	}
	else
	{
		LoginToken = BackendLoginToken;
	}

	if (LoginToken.IsEmpty())
	{
		VSPV_LOG(Error, "Login token for player is {} cannot be generated.", *GetPlayerId());
		Reconnect();
		return false;
	}

	VSPV_LOG(Log, "Logging in. VoiceServer '{}'. PlayerId '{}'. Token '{}'", *VivoxServer, *GetPlayerId(), *LoginToken);

	// login
	ILoginSession::FOnBeginLoginCompletedDelegate OnBeginLoginCompleteCallback;
	OnBeginLoginCompleteCallback.BindUObject(this, &ThisClass::LoginComplete);
	auto Status = LoginSession.BeginLogin(VivoxServer, LoginToken, OnBeginLoginCompleteCallback);
	if (Status != VxErrorSuccess)
	{
		VSPV_LOG(
			Error,
			"Login call failed for {}: {} ({}). Start reconnection process.",
			*GetPlayerId(),
			ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)),
			Status);
		Reconnect();
		return false;
	}

	// timeout for vivox
	if (LoginTimeoutDelegateHandle.IsValid())
		FTicker::GetCoreTicker().RemoveTicker(LoginTimeoutDelegateHandle);
	LoginTimeoutDelegateHandle = FTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(
			this,
			[this](float TimeDelta)
			{
				VSPV_LOG(Error, "Login process stopped by timeout. Start reconnection process.");

				LoginTimeoutDelegateHandle.Reset();
				Reconnect();
				return false;
			}),
		TimeoutForLoginToVivoxS);

	return true;
}

bool UVSPVivox::IsLoggedIn()
{
	return GetLoginState() == LoginState::LoggedIn || GetLoginState() == LoginState::LoggingIn;
}

void UVSPVivox::Logout()
{
	if (!IsInitialized())
		return;

	VSPV_LOG(Log, "PlayerId '{}'", *GetPlayerId());

	if (LogoutDelegateHandle.IsValid())
		FTicker::GetCoreTicker().RemoveTicker(LogoutDelegateHandle);
	LogoutDelegateHandle = FTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(
			this,
			[this](float TimeDelta)
			{
				DoLogout();
				return false;
			}),
		TimeoutBeforeStartLogoutFromVivoxS);
}

void UVSPVivox::Reconnect()
{
	auto PlayerId = GetPlayerId();
	auto DisplayName = GetDisplayName();

	if (ReconnectDelegateHandle.IsValid())
	{
		VSPV_LOG(Log, "We have already started reconnection process");
		return;
	}

	VSPV_LOG(
		Warning,
		"Disconnect before reconnect. PlayerId '{}'. DisplayName '{}'. TimeoutForReconnectS {:.2f}",
		*PlayerId,
		*DisplayName,
		TimeoutForReconnectS);

	OnDisconnect.Broadcast();

	Channels.Empty();
	Uninitialize();

	// make a pause to not DDOS attack Vivox
	ReconnectDelegateHandle = FTicker::GetCoreTicker().AddTicker(
		FTickerDelegate::CreateWeakLambda(
			this,
			[this, PlayerId, DisplayName](float TimeDelta)
			{
				VSPV_LOG(Warning, "Do reconnect.");

				ReconnectDelegateHandle.Reset(); // following initializer might start reconnection process again.
				if (Initialize(PlayerId, DisplayName))
					OnReconnect.Broadcast();
				return false;
			}),
		TimeoutForReconnectS);
}

void UVSPVivox::LoginSessionStateChanged(LoginState State)
{
	switch (State)
	{
	case LoginState::LoggedOut:
	{
		VSPV_LOG(Error, "Unexpectedly Logged Out by Network. Start reconnection process.");
		AsyncTask(
			ENamedThreads::GameThread,
			[This = MakeWeakObjectPtr(this)]()
			{
				VSPvCheckReturn(This.IsValid());
				This->Reconnect();
			});
	}
	break;
	default:
	{
		VSPV_LOG(Verbose, "LoginSession State Change: {}", *UEnum::GetValueAsString(State));
	}
	break;
	}
}

void UVSPVivox::LoginComplete(VivoxCoreError Status)
{
	if (VSPCheck(LoginTimeoutDelegateHandle.IsValid()))
		FTicker::GetCoreTicker().RemoveTicker(LoginTimeoutDelegateHandle);
	LoginTimeoutDelegateHandle.Reset();

	// check status
	if (Status != VxErrorSuccess)
	{
		VSPV_LOG(
			Error,
			"Login failure for {}: {} ({}). Start reconnection process.",
			*GetPlayerId(),
			ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)),
			Status);

		AsyncTask(
			ENamedThreads::GameThread,
			[This = MakeWeakObjectPtr(this)]()
			{
				VSPvCheckReturn(This.IsValid());
				This->Reconnect();
			});

		return;
	}

	VSPV_LOG(Log, "Login success for '{}'", *GetPlayerId());

	// process waiting channels
	AsyncTask(
		ENamedThreads::GameThread,
		[This = MakeWeakObjectPtr(this)]()
		{
			VSPvCheckReturn(This.IsValid());

			VSPV_LOG(Log, "Join waiting channels.");

			// do not use iterator, in some cases JoinChannel might start reconnection process
			for (int32 Index = 0; Index < This->Channels.Num(); ++Index)
			{
				auto Channel = This->Channels[Index];
				VSPvCheckReturn(Channel);
				VSPvCheckReturn(!Channel->IsConnected());
				//todo make request to recconect to channel manager
				// FString BackendJoinToken = TEXT("");
				// Channel->JoinChannel( BackendJoinToken );
				This->OnShouldReconectChannelAsync.Broadcast(Channel);
			}
			This->OnLoginSuccess.Broadcast();
		});
}

void UVSPVivox::OnInputDeviceAdded(const IAudioDevice& NewDevice)
{
	VSPV_LOG(Verbose, "Device {}", *NewDevice.Name());

	const bool bContains = InputDevices.ContainsByPredicate(
		[&NewDevice](const IAudioDevice* Device)
		{
			return NewDevice.Id() == Device->Id();
		});
	if (!bContains && NewDevice.Name() != DefaultSystemDeviceName && NewDevice.Name() != DefaultCommunicationDeviceName)
	{
		VSPV_LOG(Log, "New input device {} has been added", *NewDevice.Name());
		InputDevices.Add(&NewDevice);
		OnInputDevicesUpdated.Broadcast();
	}
}

void UVSPVivox::OnInputDeviceRemoved(const IAudioDevice& OldDevice)
{
	VSPV_LOG(Verbose, "Device {}", *OldDevice.Name());

	const int32 DeviceIndex = InputDevices.IndexOfByPredicate(
		[&OldDevice](const IAudioDevice* Device)
		{
			return OldDevice.Id() == Device->Id();
		});
	if (DeviceIndex != INDEX_NONE)
	{
		VSPV_LOG(Log, "Input device {} has been removed", *OldDevice.Name());
		InputDevices.RemoveAt(DeviceIndex);
		OnInputDevicesUpdated.Broadcast();
	}
	else
	{
		VSPV_LOG(Warning, "Try to remove input device {} which wasn't in InputDevices", *OldDevice.Name());
	}
}

void UVSPVivox::OnEffectiveInputDeviceChanged(const IAudioDevice& Device)
{
	if (Device.IsEmpty())
	{
		IClient* VivoxClient = GetVivoxVoiceClient();
		VSPvCheckReturn(VivoxClient);

		IAudioDevices& ClientInputDevices = VivoxClient->AudioInputDevices();
		VivoxCoreError Status = ClientInputDevices.SetActiveDevice(ClientInputDevices.SystemDevice());
		if (Status != VxErrorSuccess)
			VSPV_LOG(
				Error,
				"Vivox error on change {} ({})",
				ANSI_TO_TCHAR(FVivoxCoreModule::ErrorToString(Status)),
				Status);
	}
	else
	{
		VSPV_LOG(Log, "Effective input device {} has been changed", *Device.Name());
	}

	OnInputDevicesUpdated.Broadcast();
}

void UVSPVivox::AddChannel(UVSPVivoxChannel* Channel)
{
	VSPVCM_LOG(Verbose, "Add channel: {} to index: {}", *Channel->GetChannelName(), Channels.Num());
	Channels.AddUnique(Channel);
	PrintCurrentChannelsInfo();
}

int32 UVSPVivox::RemoveChannel(UVSPVivoxChannel* Channel)
{
	VSPCheckReturn(Channel, 0);
	int32 RemovedIndex = Channels.IndexOfByPredicate(
		[Channel](const UVSPVivoxChannel* ChannelInstance)
		{
			return Channel->GetChannelName() == ChannelInstance->GetChannelName();
		});
	const int32 Result = Channels.RemoveSingleSwap(Channel, false);
	VSPVCM_LOG(Verbose, "Remove channel: {} from index: {}", *Channel->GetChannelName(), RemovedIndex);
	PrintCurrentChannelsInfo();
	return Result;
}

UVSPVivoxChannel* UVSPVivox::GetChannelByIndex(int32 ChannelIndex)
{
	VSPVCM_LOG(Verbose, "Try get channel by Index {}", ChannelIndex);
	if (Channels.IsValidIndex(ChannelIndex))
	{
		return Channels[ChannelIndex];
	}
	VSPVCM_LOG(Verbose, "Can't get channel by index {}", ChannelIndex);
	return nullptr;
}

void UVSPVivox::PrintCurrentChannelsInfo() const
{
	VSPVCM_LOG(Verbose, "Channels updated, now we have {} channels", Channels.Num());
	if (Channels.Num() > 0)
	{
		VSPVCM_LOG(Verbose, "Existing Channels:");
		for (int32 i = 0; i < Channels.Num(); ++i)
		{
			VSPVCM_LOG(Verbose, "{} index: {}", *Channels[i]->GetChannelName(), i);
		}
	}
}

void UVSPVivox::ChannelStateChanged(UVSPVivoxChannel* Channel, ConnectionState State, bool bUnexpectedChange)
{
	VSPvCheckReturn(Channel);

	VSPV_LOG(
		Log,
		"Channel '{}'. State {}. bUnexpectedChange {}",
		*Channel->GetChannelName(),
		*UEnum::GetValueAsString(State),
		(int32)bUnexpectedChange);

	if (bUnexpectedChange)
	{
		VSPV_LOG(Error, "Something went wrong in Vivox channel workflow. Start reconnection process.");
		Reconnect();
		return;
	}

	if (State == ConnectionState::Disconnected)
	{
		Channel->OnStateChanged.RemoveAll(this);

		const int32 Num = RemoveChannel(Channel);
		if (Num == 0)
			VSPV_LOG(Warning, "Unknown channel");
	}
	else if (State == ConnectionState::Connected)
	{
		// This is ugly crutch has been done to fix an issue when existing vivox voice channel implicitly muted when new channel is created.
		for (auto& OtherChannel : Channels)
		{
			if (OtherChannel != Channel)
				OtherChannel->CrutchToFixVoiceTransmissionAfterJoinNewChannel();
		}
	}

	if (Channels.Num() == 0)
		Logout();
}

void UVSPVivox::DoLogout()
{
	if (!IsInitialized())
		return;

	auto VivoxVoiceClient = GetVivoxVoiceClient();
	VSPvCheckReturn(VivoxVoiceClient);

	VSPV_LOG(Log, "PlayerId '{}'", *GetPlayerId());

	// logout from vivox
	auto& LoginSession = VivoxVoiceClient->GetLoginSession(VivoxAccountId);
	BindLoginHandlers(false, LoginSession);
	LoginSession.Logout();

	LogoutDelegateHandle.Reset();
}

void UVSPVivox::BindLoginHandlers(bool DoBind, ILoginSession& LoginSession)
{
	VSPV_LOG(Verbose, "DoBind {}", (int32)DoBind);

	if (DoBind)
	{
		LoginSession.EventStateChanged.AddUObject(this, &ThisClass::LoginSessionStateChanged);
	}
	else
	{
		LoginSession.EventStateChanged.RemoveAll(this);
	}
}
