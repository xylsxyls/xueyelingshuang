#include "ConsoleTest1.h"
#include <stdio.h>
#include "NetWork/NetWorkAPI.h"
#include <windows.h>
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ServerReceiveCallback : public ServerCallback
{
public:
	ServerReceiveCallback() :m_x(0){}
public:
	virtual void receive(uv_tcp_t* client, char* buffer, int32_t length)
	{
		return;
		++m_x;
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
		
		if (m_x % 300000 == 0)
		{
			printf("x = %d,time = %d\n", m_x, ::GetTickCount());
		}
		//((char*)buffer)[length] = 0;
		//m_netServer->send("receive", 7, client);
		return;
		
		
	}

	virtual void clientConnected(uv_tcp_t* client)
	{
		printf("client connected\n");
		int32_t count = 300000;
		int begin = ::GetTickCount();
		printf("begin = %d\n", ::GetTickCount());
		while (count-- != 0)
		{
			m_netServer->send("121243", 6, client);
		}
		printf("time = %d", ::GetTickCount() - begin);
	}

	int m_x;
};

class ServerTask1 : public CTask
{
public:
	virtual void DoTask()
	{
		ServerReceiveCallback receive;
		NetServer server;
		server.listen(7000, &receive);
	}
};

class ServerTask2 : public CTask
{
public:
	virtual void DoTask()
	{
		ServerReceiveCallback receive;
		NetServer server;
		server.listen(7001, &receive);
	}
};

class ServerTask3 : public CTask
{
public:
	virtual void DoTask()
	{
		ServerReceiveCallback receive;
		NetServer server;
		server.listen(7002, &receive);
	}
};

class ServerTask4 : public CTask
{
public:
	virtual void DoTask()
	{
		ServerReceiveCallback receive;
		NetServer server;
		server.listen(7003, &receive);
	}
};

int main()
{
	auto userId = CTaskThreadManager::Instance().Init();
	auto userId2 = CTaskThreadManager::Instance().Init();
	auto userId3 = CTaskThreadManager::Instance().Init();
	auto userId4 = CTaskThreadManager::Instance().Init();
	auto thread = CTaskThreadManager::Instance().GetThreadInterface(userId);
	auto thread2 = CTaskThreadManager::Instance().GetThreadInterface(userId2);
	auto thread3 = CTaskThreadManager::Instance().GetThreadInterface(userId3);
	auto thread4 = CTaskThreadManager::Instance().GetThreadInterface(userId4);
	std::shared_ptr<CTask> spTask;
	spTask.reset(new ServerTask1);
	thread->PostTask(spTask,1);
	spTask.reset(new ServerTask2);
	thread2->PostTask(spTask, 1);
	spTask.reset(new ServerTask3);
	thread3->PostTask(spTask, 1);
	spTask.reset(new ServerTask4);
	thread4->PostTask(spTask, 1);
	getchar();
	return 0;
}