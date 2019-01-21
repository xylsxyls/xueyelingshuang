#pragma once
#include "NetWork/NetWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ClientPackage.h"

class ServerManagerReceive : public ServerCallback
{
public:
	ServerManagerReceive();

public:
	virtual void clientConnected(uv_tcp_t* client);

	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	std::string eraseServerName(ProtoMessage& message);

	

	void addClientId(ProtoMessage& message, uv_tcp_t* sender);

private:
	
};