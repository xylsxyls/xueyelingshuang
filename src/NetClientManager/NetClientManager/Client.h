#pragma once
#include "NetWork/NetWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class Client : public NetClient
{
public:
	virtual void onServerConnected();

	virtual void onReceive(const char* buffer, int32_t length, MessageType type);
};