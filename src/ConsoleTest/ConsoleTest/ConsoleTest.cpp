#include "ConsoleTest.h"
#include "NetWork/NetWorkAPI.h"
#include <windows.h>
#include <atomic>
#include "D:\\SendToMessageTest.h"
#include "CSystem/CSystemAPI.h"

#define SEND_COUNT 1000000

std::atomic<int> calc = 0;

std::atomic<int> calc1 = 0;
std::atomic<int> calc2 = 0;
std::atomic<int> calc3 = 0;
std::atomic<int> calc4 = 0;

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
		//return;
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
		++calc;
		if (calc % 200000 == 0)
		{
			RCSend("client x = %d,time = %d,threadId = %d\n", calc, ::GetTickCount(), CSystem::SystemThreadId());
		}

		if (length != 6)
		{
			RCSend("length = %d,%s\n", length, buffer);
			return;
		}

		std::string buf = buffer;
		if (buf == "121243")
		{
			++calc1;
		}
		else if (buf == "575798")
		{
			++calc2;
		}
		else if (buf == "ababdc")
		{
			++calc3;
		}
		else if (buf == "efefhg")
		{
			++calc4;
		}

		if (calc % 200000 == 0)
		{
			if (calc1 > 0 && calc1 % 1000000 == 0)
			{
				RCSend("client calc1 = %d\n", calc1);
			}
			if (calc2 > 0 && calc2 % 1000000 == 0)
			{
				RCSend("client calc2 = %d\n", calc2);
			}
			if (calc3 > 0 && calc3 % 1000000 == 0)
			{
				RCSend("client calc3 = %d\n", calc3);
			}
			if (calc4 > 0 && calc4 % 1000000 == 0)
			{
				RCSend("client calc4 = %d\n", calc4);
			}
		}
		return;
	}
	char* m_buf;
};


int32_t main()
{
	//Sleep(3000);
	RCSend("beginTime = %d", ::GetTickCount());
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