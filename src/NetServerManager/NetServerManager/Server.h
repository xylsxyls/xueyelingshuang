#pragma once
#include "NetWork/NetWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "Variant/VariantAPI.h"
#include <map>

class Server : public NetServer
{
public:
	virtual bool onFirstReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type);

	virtual void onReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type);

	virtual void onClientConnected(uv_tcp_t* client);

	virtual void onClientDisconnected(uv_tcp_t* client);

	virtual int32_t headNumber();
};