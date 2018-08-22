#include "ConsoleTest1.h"
#include <stdio.h>
#include "NetWork/NetWorkAPI.h"
#include <windows.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "D:\\SendToMessageTest.h"

std::atomic<int> calc = 0;

class ServerReceiveCallback : public ServerCallback
{
public:
	ServerReceiveCallback() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		++calc;
		if (calc % 200000 == 0)
		{
			RCSend("x = %d,time = %d,threadId = %d\n", calc, ::GetTickCount(), CSystem::SystemThreadId());
		}

		if (length != 6 || 
			(std::string(buffer) != "121243" &&
			std::string(buffer) != "575798" &&
			std::string(buffer) != "ababdc" &&
			std::string(buffer) != "efefhg"))
		{
			RCSend("length = %d,%s\n", length, buffer);
			for (int32_t index = -5; index < 12; ++index)
			{
				RCSend("%d,", buffer[index]);
			}
			RCSend("\n");
			getchar();
		}
		
		
		//((char*)buffer)[length] = 0;
		//m_netServer->send("receive", 7, client);
		return;
	}

	virtual void clientConnected(uv_tcp_t* client)
	{
		return;
		RCSend("client connected\n");
		int32_t count = 1200000;
		int begin = ::GetTickCount();
		RCSend("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netServer->send("121243", 6, client);
		}
		RCSend("time = %d", ::GetTickCount() - begin);
	}

	int m_x;
};

int main()
{
	ServerReceiveCallback receive;
	NetServer server;
	server.listen(7000, &receive);
	getchar();
	return 0;
}