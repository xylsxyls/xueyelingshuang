#pragma once
#include "NetWork/NetWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class Client : public NetClient
{
public:
	virtual bool onFirstReceive(const char* buffer, int32_t length, MessageType type);

	virtual void onFirstHead();

	virtual void onReceive(const char* buffer, int32_t length, MessageType type);

	virtual void onServerConnected();

	virtual void onServerNotFind();

	virtual void onServerDisconnected();
};