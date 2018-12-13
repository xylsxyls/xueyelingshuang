#include "NetServerManager.h"
#include <stdint.h>
#include <stdio.h>
#include "NetWork/NetWorkAPI.h"
#include "ServerManagerReceive.h"
#include "ServerManagerProcessReceive.h"
#include <mutex>

int32_t main()
{
	std::mutex mapMutex;
	std::map<std::string, uv_tcp_t*> connectedMap;

	ServerManagerReceive serverManagerReceive;
	serverManagerReceive.setConnectedMap(&connectedMap);
	serverManagerReceive.setMutex(&mapMutex);

	NetServer server;
	server.listen(5203, &serverManagerReceive);

	ServerManagerProcessReceive serverManagerProcessReceive;
	serverManagerProcessReceive.setConnectedMap(&connectedMap);
	serverManagerProcessReceive.setMutex(&mapMutex);
	serverManagerProcessReceive.setServer(&server);
	ProcessClient::instance().initReceive(&serverManagerProcessReceive);

	while (true) Sleep(1000);
	return 0;
}