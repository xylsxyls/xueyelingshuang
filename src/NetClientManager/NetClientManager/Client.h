#pragma once
#include "NetWork/NetWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class Client : public NetClient
{
public:
	virtual void onServerConnected();

	virtual void onReceive(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);
};