#include "NetServerTest.h"
#include <stdio.h>
#include "NetWork/NetWorkAPI.h"
#include <windows.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "D:\\SendToMessageTest.h"

std::atomic<int> calc = 0;

std::atomic<int> calc1 = 0;
std::atomic<int> calc2 = 0;
std::atomic<int> calc3 = 0;
std::atomic<int> calc4 = 0;

class ServerReceiveCallback : public ServerCallback
{
public:
	ServerReceiveCallback() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length, int32_t protocolId)
	{
		//RCSend("client = %d, buffer = %s, length = %d, protocolId = %d", sender, buffer, length, protocolId);
		//m_netServer->send("1212434", 7, 2, sender);
		return;
		++calc;
		if (calc % 200000 == 0)
		{
			RCSend("server x = %d,time = %d,threadId = %d\n", calc, ::GetTickCount(), CSystem::SystemThreadId());
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
				RCSend("server calc1 = %d\n", calc1);
			}
			if (calc2 > 0 && calc2 % 1000000 == 0)
			{
				RCSend("server calc2 = %d\n", calc2);
			}
			if (calc3 > 0 && calc3 % 1000000 == 0)
			{
				RCSend("server calc3 = %d\n", calc3);
			}
			if (calc4 > 0 && calc4 % 1000000 == 0)
			{
				RCSend("server calc4 = %d\n", calc4);
			}
		}
		
		//((char*)buffer)[length] = 0;
		//m_netServer->send("receive", 7, client);
		return;
	}

	virtual void clientConnected(uv_tcp_t* client)
	{
		//return;
		RCSend("client connected\n");
		int32_t count = 1000000;
		int begin = ::GetTickCount();
		RCSend("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netServer->send("121243", 6, CorrespondParam::PROTO_MESSAGE, client);
		}
		RCSend("time = %d", ::GetTickCount() - begin);
		return;
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