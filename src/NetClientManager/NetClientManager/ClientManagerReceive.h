#pragma once
#include "NetWork/NetWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

class ClientManagerReceive : public ClientCallback
{
public:
	virtual void serverConnected(uv_tcp_t* server);

	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId);

	std::string removeClientName(ProtoMessage& message);

	void addClientIdMap(uv_tcp_t* server, int32_t clientPid);

	int32_t findClientId(uv_tcp_t* server);

private:
	std::map<uv_tcp_t*, int32_t> m_clientIdMap;
};