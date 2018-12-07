#include "ClientManagerReceive.h"

void ClientManagerReceive::serverConnected(uv_tcp_t* server)
{
	RCSend("serverConnected = %d", server);
}

void ClientManagerReceive::receive(uv_tcp_t* client, char* buffer, int32_t length)
{
	RCSend("receive = %d, buffer = %s, length = %d", client, buffer, length);
}

