#pragma once
#include "ProcessClient/ProcessClientAPI.h"

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

	void setConnectedMap(std::map<std::string, uv_tcp_t*>* connectedMap);

	void setMutex(std::mutex* mapMutex);

protected:
	NetServer* m_netServer;
	std::map<std::string, uv_tcp_t*>* m_connectedMap;
	std::mutex* m_mapMutex;
};