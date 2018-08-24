#include "ConsoleTest.h"
#include "NetWork/NetWorkAPI.h"
#include <windows.h>

#define SEND_COUNT 1000000

class ClientReceive : public ClientCallback
{
public:
	ClientReceive() :
		m_buf(nullptr)
	{

	}

public:
	void serverConnected(uv_tcp_t* server)
	{
		int32_t count = SEND_COUNT;
		while (count-- != 0)
		{
			m_netClient->send(m_buf, 6);
		}
	}

	void setText(char* buf)
	{
		m_buf = buf;
	}

	void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		return;
	}
	char* m_buf;
};


int32_t main()
{
	//Sleep(3000);
	
	ClientReceive clientReceive1;
	char* buf1 = new char[6];
	memcpy(buf1, "121243", 6);
	clientReceive1.setText(buf1);
	NetClient client1;
	client1.connect("10.1.22.116", 7000, &clientReceive1);

	ClientReceive clientReceive2;
	char* buf2 = new char[6];
	memcpy(buf2, "575798", 6);
	clientReceive2.setText(buf2);
	NetClient client2;
	client2.connect("10.1.22.116", 7000, &clientReceive2);
	
	ClientReceive clientReceive3;
	char* buf3 = new char[6];
	memcpy(buf3, "ababdc", 6);
	clientReceive3.setText(buf3);
	NetClient client3;
	client3.connect("10.1.22.116", 7000, &clientReceive3);
	
	ClientReceive clientReceive4;
	char* buf4 = new char[6];
	memcpy(buf4, "efefhg", 6);
	clientReceive4.setText(buf4);
	NetClient client4;
	client4.connect("10.1.22.116", 7000, &clientReceive4);

	getchar();
	return 0;
}