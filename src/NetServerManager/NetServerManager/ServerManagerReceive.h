#pragma once
#include "NetWork/NetWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"

namespace std
{
	class mutex;
}

class ServerManagerReceive : public ServerCallback
{
public:
	ServerManagerReceive();

public:
	virtual void clientConnected(uv_tcp_t* client);

	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length, int32_t protocolId);

	std::string eraseServerName(ProtoMessage& message);

	void addClientPtr(ProtoMessage& message, uv_tcp_t* sender);
};