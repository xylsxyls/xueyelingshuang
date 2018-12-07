#include "ServerManagerReceive.h"

void ServerManagerReceive::clientConnected(uv_tcp_t* client)
{
	RCSend("clientConnected = %d", client);
}

void ServerManagerReceive::receive(uv_tcp_t* client, char* buffer, int32_t length)
{
	RCSend("receive = %d, buffer = %s, length = %d", client, buffer, length);
}

