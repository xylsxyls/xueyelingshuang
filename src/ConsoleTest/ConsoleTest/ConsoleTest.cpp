#include <stdio.h>
#include "LibuvTcp/LibuvTcpAPI.h"

class ClientReceive : public ReceiveCallback
{
public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		buffer[length] = 0;
		printf("%s\n", buffer);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		m_libuvTcp->send(server, "12123", 5);
		m_libuvTcp->send(server, "12123", 5);
		m_libuvTcp->send(server, "12123", 5);
	}
};

int main(int argc, char **argv)
{
	ClientReceive receive;
	LibuvTcp client;
	client.initClient("10.1.22.116", 7000, &receive);
	client.loop();
	return 0;
}