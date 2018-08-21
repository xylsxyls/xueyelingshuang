#include "ConsoleTest.h"
#include "NetWork/NetWorkAPI.h"

#define SEND_COUNT 1000000

class ClientReceive : public ClientCallback
{
public:
	void serverConnected(uv_tcp_t* server)
	{
		int32_t count = SEND_COUNT;
		while (count-- != 0)
		{
			m_netClient->send("121243", 6);
		}
	}

	void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		return;
	}
};


int32_t main()
{
	ClientReceive clientReceive1;
	NetClient client1;
	client1.connect("10.1.22.116", 7000, &clientReceive1);

	ClientReceive clientReceive2;
	NetClient client2;
	client2.connect("10.1.22.116", 7000, &clientReceive2);

	ClientReceive clientReceive3;
	NetClient client3;
	client3.connect("10.1.22.116", 7000, &clientReceive3);

	ClientReceive clientReceive4;
	NetClient client4;
	client4.connect("10.1.22.116", 7000, &clientReceive4);

	getchar();
	return 0;
}