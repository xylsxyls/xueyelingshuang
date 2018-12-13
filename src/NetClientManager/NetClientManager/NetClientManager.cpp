#include "NetClientManager.h"
#include <stdint.h>
#include <stdio.h>
#include "ProcessClient/ProcessClientAPI.h"
#include "ProcessReceive.h"
#include "NetWork/NetWorkAPI.h"
#include "ClientManagerReceive.h"

int32_t main()
{
	ClientManagerReceive clientManagerReceive;
	NetClient client;
	client.connect("10.1.22.66", 5203, &clientManagerReceive);
	ProcessReceive processReceive;
	processReceive.setNetClient(&client);
	ProcessClient::instance().initReceive(&processReceive);

	while (true) Sleep(1000);
	return 0;
}