#include <stdio.h>
#include "NetWork/NetWorkAPI.h"
#include <windows.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "CSystem/CSystemAPI.h"
#include "D:\\SendToMessageTest.h"

#define SEND_COUNT 1000000

std::atomic<int> calc = 0;

class ClientReceiveCallback : public ClientCallback
{
public:
	ClientReceiveCallback() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		//RCSend("%d", CSystem::systemThreadId());
		++calc;
		char* test = (char*)malloc(length + 1);
		memcpy(test, buffer, length);
		test[length] = 0;
		if (length != 6 || std::string(test) != "121243")
		{
			printf("length = %d,%s\n", length, test);
			for (int32_t index = -5; index < 12; ++index)
			{
				printf("%d,", buffer[index]);
			}
			printf("\n");
			getchar();
		}
		free(test);

		if (calc % 300000 == 0)
		{
			printf("x = %d,time = %d\n", calc, ::GetTickCount());
		}
		//m_netClient->send("1212434", 7);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		int32_t count = SEND_COUNT;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netClient->send("121243", 6);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	std::atomic<int> m_x;
};

class ClientReceiveCallback2 : public ClientCallback
{
public:
	ClientReceiveCallback2() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		//RCSend("%d", CSystem::systemThreadId());
		++calc;
		char* test = (char*)malloc(length + 1);
		memcpy(test, buffer, length);
		test[length] = 0;
		if (length != 6 || std::string(test) != "121243")
		{
			printf("length = %d,%s\n", length, test);
			for (int32_t index = -5; index < 12; ++index)
			{
				printf("%d,", buffer[index]);
			}
			printf("\n");
			getchar();
		}
		free(test);

		if (calc % 300000 == 0)
		{
			printf("x = %d,time = %d\n", calc, ::GetTickCount());
		}
		//m_netClient->send("1212434", 7);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		int32_t count = SEND_COUNT;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netClient->send("575798", 6);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	std::atomic<int> m_x;
};

class ClientReceiveCallback3 : public ClientCallback
{
public:
	ClientReceiveCallback3() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		//RCSend("%d", CSystem::systemThreadId());
		++calc;
		char* test = (char*)malloc(length + 1);
		memcpy(test, buffer, length);
		test[length] = 0;
		if (length != 6 || std::string(test) != "121243")
		{
			printf("length = %d,%s\n", length, test);
			for (int32_t index = -5; index < 12; ++index)
			{
				printf("%d,", buffer[index]);
			}
			printf("\n");
			getchar();
		}
		free(test);

		if (calc % 300000 == 0)
		{
			printf("x = %d,time = %d\n", calc, ::GetTickCount());
		}
		//m_netClient->send("1212434", 7);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		int32_t count = SEND_COUNT;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netClient->send("ababdc", 6);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	std::atomic<int> m_x;
};

class ClientReceiveCallback4 : public ClientCallback
{
public:
	ClientReceiveCallback4() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		//RCSend("%d", CSystem::systemThreadId());
		++calc;
		char* test = (char*)malloc(length + 1);
		memcpy(test, buffer, length);
		test[length] = 0;
		if (length != 6 || std::string(test) != "121243")
		{
			printf("length = %d,%s\n", length, test);
			for (int32_t index = -5; index < 12; ++index)
			{
				printf("%d,", buffer[index]);
			}
			printf("\n");
			getchar();
		}
		free(test);

		if (calc % 300000 == 0)
		{
			printf("x = %d,time = %d\n", calc, ::GetTickCount());
		}
		//m_netClient->send("1212434", 7);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		int32_t count = SEND_COUNT;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netClient->send("efefhg", 6);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	std::atomic<int> m_x;
};

class ClientReceiveCallback5 : public ClientCallback
{
public:
	ClientReceiveCallback5() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		//RCSend("%d", CSystem::systemThreadId());
		++calc;
		char* test = (char*)malloc(length + 1);
		memcpy(test, buffer, length);
		test[length] = 0;
		if (length != 6 || std::string(test) != "121243")
		{
			printf("length = %d,%s\n", length, test);
			for (int32_t index = -5; index < 12; ++index)
			{
				printf("%d,", buffer[index]);
			}
			printf("\n");
			getchar();
		}
		free(test);

		if (calc % 300000 == 0)
		{
			printf("x = %d,time = %d\n", calc, ::GetTickCount());
		}
		//m_netClient->send("1212434", 7);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		int32_t count = SEND_COUNT;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netClient->send("efefhg", 6);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	std::atomic<int> m_x;
};

class ClientReceiveCallback6 : public ClientCallback
{
public:
	ClientReceiveCallback6() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		//RCSend("%d", CSystem::systemThreadId());
		++calc;
		char* test = (char*)malloc(length + 1);
		memcpy(test, buffer, length);
		test[length] = 0;
		if (length != 6 || std::string(test) != "121243")
		{
			printf("length = %d,%s\n", length, test);
			for (int32_t index = -5; index < 12; ++index)
			{
				printf("%d,", buffer[index]);
			}
			printf("\n");
			getchar();
		}
		free(test);

		if (calc % 300000 == 0)
		{
			printf("x = %d,time = %d\n", calc, ::GetTickCount());
		}
		//m_netClient->send("1212434", 7);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		int32_t count = SEND_COUNT;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netClient->send("efefhg", 6);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	std::atomic<int> m_x;
};

class ClientReceiveCallback7 : public ClientCallback
{
public:
	ClientReceiveCallback7() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		//RCSend("%d", CSystem::systemThreadId());
		++calc;
		char* test = (char*)malloc(length + 1);
		memcpy(test, buffer, length);
		test[length] = 0;
		if (length != 6 || std::string(test) != "121243")
		{
			printf("length = %d,%s\n", length, test);
			for (int32_t index = -5; index < 12; ++index)
			{
				printf("%d,", buffer[index]);
			}
			printf("\n");
			getchar();
		}
		free(test);

		if (calc % 300000 == 0)
		{
			printf("x = %d,time = %d\n", calc, ::GetTickCount());
		}
		//m_netClient->send("1212434", 7);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		int32_t count = SEND_COUNT;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netClient->send("efefhg", 6);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	std::atomic<int> m_x;
};

class ClientReceiveCallback8 : public ClientCallback
{
public:
	ClientReceiveCallback8() :m_x(0){}

public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		//return;
		//RCSend("%d", CSystem::systemThreadId());
		++calc;
		char* test = (char*)malloc(length + 1);
		memcpy(test, buffer, length);
		test[length] = 0;
		if (length != 6 || std::string(test) != "121243")
		{
			printf("length = %d,%s\n", length, test);
			for (int32_t index = -5; index < 12; ++index)
			{
				printf("%d,", buffer[index]);
			}
			printf("\n");
			getchar();
		}
		free(test);

		if (calc % 300000 == 0)
		{
			printf("x = %d,time = %d\n", calc, ::GetTickCount());
		}
		//m_netClient->send("1212434", 7);
	}

	virtual void serverConnected(uv_tcp_t* server)
	{
		printf("server connected\n");
		int32_t count = SEND_COUNT;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netClient->send("efefhg", 6);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	std::atomic<int> m_x;
};

int main(int argc, char **argv)
{
	//Sleep(10000);
	ClientReceiveCallback receive;
	NetClient client;
	client.connect("10.1.22.116", 7000, &receive);

	ClientReceiveCallback2 receive2;
	NetClient client2;
	client2.connect("10.1.22.116", 7000, &receive2);

	ClientReceiveCallback3 receive3;
	NetClient client3;
	client3.connect("10.1.22.116", 7000, &receive3);

	ClientReceiveCallback4 receive4;
	NetClient client4;
	client4.connect("10.1.22.116", 7000, &receive4);

	//ClientReceiveCallback5 receive5;
	//NetClient client5;
	//client5.connect("10.1.22.116", 7000, &receive5);
	//
	//ClientReceiveCallback6 receive6;
	//NetClient client6;
	//client6.connect("10.1.22.116", 7000, &receive6);
	//
	//ClientReceiveCallback7 receive7;
	//NetClient client7;
	//client7.connect("10.1.22.116", 7000, &receive7);
	//
	//ClientReceiveCallback8 receive8;
	//NetClient client8;
	//client8.connect("10.1.22.116", 7000, &receive8);
	getchar();
	return 0;
}