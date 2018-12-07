#include "NetServerManager.h"
#include <stdint.h>
#include <stdio.h>
#include "NetWork/NetWorkAPI.h"
#include "ServerManagerReceive.h"

int32_t main()
{
	ServerManagerReceive receive;
	NetServer server;
	server.listen(5203, &receive);
	while (true) Sleep(1000);
	return 0;
}