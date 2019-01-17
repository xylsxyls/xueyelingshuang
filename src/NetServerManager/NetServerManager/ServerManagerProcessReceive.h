#pragma once
#include "ProcessWork/ProcessWorkAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"

typedef struct uv_tcp_s uv_tcp_t;
class NetServer;
namespace std
{
	class mutex;
}

class ServerManagerProcessReceive : public ReceiveCallback
{
public:
	ServerManagerProcessReceive();

public:
	virtual void receive(char* buffer, int32_t length, int32_t sendPid, int32_t protocalId);

	void setServer(NetServer* netServer);

	void setClientName(ProtoMessage& message, const std::string& serverName);

	uv_tcp_t* getClientPtr(ProtoMessage& message);

protected:
	NetServer* m_netServer;
};