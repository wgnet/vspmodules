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
#include "VSPVivoxToken.h"

#include "ProjectVSPVivox.h"
#include "VSPVivox.h"


void FVSPVivoxToken::GenerateClientLoginToken(const ILoginSession& LoginSession, FString& OutToken)
{
	VSPV_LOG(Verbose, "{{{{{{{{{{ TODO: Delete it");
	VSPV_LOG(
		Verbose,
		"IMPORTANT: in production, developers should NOT use the insecure client-side token generation methods.");
	VSPV_LOG(
		Verbose,
		"To generate secure access tokens, call GenerateClientLoginToken or a custom implementation from your game server.");
	VSPV_LOG(
		Verbose,
		"This is important not only to secure access to chat features, but tokens issued by the client could");
	VSPV_LOG(Verbose, "appear expired if the client's clock is set incorrectly, resulting in rejection.");

	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox);

	FTimespan TokenExpiration = FTimespan::FromSeconds(TOKEN_EXPIRATION_TIME_S);
	OutToken = LoginSession.GetLoginToken(VSPVivox->GetVivoxKey(), TokenExpiration);

	VSPV_LOG(Verbose, "LoginSessionId '{}'. Token '{}'", *LoginSession.LoginSessionId().Name(), *OutToken);
	VSPV_LOG(Verbose, "TODO: Delete it }}}}}}}}}}");
}

void FVSPVivoxToken::GenerateClientJoinToken(const IChannelSession& ChannelSession, FString& OutToken)
{
	VSPV_LOG(Verbose, "{{{{{{{{{{ TODO: Delete it");
	VSPV_LOG(
		Verbose,
		"IMPORTANT: in production, developers should NOT use the insecure client-side token generation methods.");
	VSPV_LOG(
		Verbose,
		"To generate secure access tokens, call GenerateClientJoinToken or a custom implementation from your game server.");
	VSPV_LOG(
		Verbose,
		"This is important not only to secure access to Chat features, but tokens issued by the client could");
	VSPV_LOG(Verbose, "appear expired if the client's clock is set incorrectly, resulting in rejection.");

	auto VSPVivox = UVSPVivox::Get();
	VSPvCheckReturn(VSPVivox);

	FTimespan TokenExpiration = FTimespan::FromSeconds(TOKEN_EXPIRATION_TIME_S);
	OutToken = ChannelSession.GetConnectToken(VSPVivox->GetVivoxKey(), TokenExpiration);

	VSPV_LOG(Verbose, "Channel '{}'. Token '{}'", *ChannelSession.Channel().Name(), *OutToken);
	VSPV_LOG(Verbose, "TODO: Delete it }}}}}}}}}}");
}
