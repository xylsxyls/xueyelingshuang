#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"

typedef struct uv_tcp_s uv_tcp_t;
class NetServer;

class ServerManagerProcessReceive : public ReceiveCallback
{
public:
	ServerManagerProcessReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId);

	void setServer(NetServer* netServer);

	uv_tcp_t* getClientPtr(const char* buffer, int32_t length);

protected:
	NetServer* m_netServer;
};