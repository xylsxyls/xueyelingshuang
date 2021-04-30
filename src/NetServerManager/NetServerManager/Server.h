#pragma once
#include "NetWork/NetWorkAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "Variant/VariantAPI.h"
#include <map>

class Server : public NetServer
{
public:
	virtual void onClientConnected(uv_tcp_t* client);

	virtual void onReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type);

protected:
	std::map<uv_tcp_t*, std::map<int32_t, std::map<int32_t, Variant>>> m_clientInfoMap;
	
};