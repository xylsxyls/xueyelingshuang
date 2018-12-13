#pragma once
#include "NetWork/NetWorkAPI.h"

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

	void setConnectedMap(std::map<std::string, uv_tcp_t*>* connectedMap);

	void setMutex(std::mutex* mapMutex);

protected:
	std::map<std::string, uv_tcp_t*>* m_connectedMap;
	std::mutex* m_mapMutex;
};