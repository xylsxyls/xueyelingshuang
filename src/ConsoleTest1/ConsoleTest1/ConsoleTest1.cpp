#include "ConsoleTest1.h"
#include <stdio.h>

void ServerReceive::receive(uv_tcp_t* client, char* buffer, int32_t length)
{
	((char*)buffer)[length] = 0;
	printf("%s\n", buffer);
	m_libuvTcp->send(client, "receive", 7);
}

void ServerReceive::clientConnected(uv_tcp_t* server)
{
	printf("client connected\n");
}

int main()
{
	ServerReceive receive;
	LibuvTcp server;
	server.initServer(7000, &receive);
	server.loop();
	return 0;
}