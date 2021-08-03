#include "NetServer.h"
#include "NetWorkHelper.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "RunLoopTask.h"
#include "ServerTask.h"
#include "HeadTask.h"

NetServer::NetServer():
m_isListen(false),
m_receiveThreadId(0),
m_headThreadId(0),
m_loopThreadId(0),
m_firstMessageLength(0)
{
	m_head[0] = 0;
	m_head[1] = 0;
}

NetServer::~NetServer()
{
	
}

void NetServer::setFirstMessageLength(int32_t length)
{
	m_firstMessageLength = length;
}

void NetServer::listen(int32_t port)
{
	if (m_isListen)
	{
		return;
	}

	m_loopThreadId = CTaskThreadManager::Instance().Init();

	std::shared_ptr<ServerTask> spServerTask(new ServerTask);
	spServerTask->setParam(port, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId)->PostTask(spServerTask);
	
	m_isListen = true;
}

void NetServer::close()
{
	if (m_headThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_headThreadId);
		m_headThreadId = 0;
	}

	if (m_receiveThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
		m_receiveThreadId = 0;
	}

	if (m_loopThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_loopThreadId);
		m_loopThreadId = 0;
	}

	m_head[0] = 0;
	m_head[1] = 0;
	m_isListen = false;
}

void NetServer::sendFirstMessage(uv_tcp_t* client, const char* buffer, int32_t length)
{
	if (client == nullptr || (buffer == nullptr && length != 0))
	{
		return;
	}
	LibuvTcp::send(NetWorkHelper::send(this, client, buffer, length, MessageType::FIRST_MESSAGE));
}

void NetServer::sendFirstMessage(uv_tcp_t* client, const std::string& message)
{
	sendFirstMessage(client, message.c_str(), message.size());
}

void NetServer::send(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type)
{
	if (client == nullptr || (buffer == nullptr && length != 0))
	{
		return;
	}
	if (m_head[0] == 0)
	{
		printf("server head number error\n");
	}
	LibuvTcp::send(NetWorkHelper::send(this, client, buffer, length, type, m_head[0]));
}

void NetServer::send(uv_tcp_t* client, const std::string& message, MessageType type)
{
	send(client, message.c_str(), message.size(), type);
}

void NetServer::head()
{
	if (m_headThreadId == 0)
	{
		m_headThreadId = CTaskThreadManager::Instance().Init();
	}

	std::shared_ptr<HeadTask> spHeadTask(new HeadTask);
	spHeadTask->setParam(10000, this);
	CTaskThreadManager::Instance().GetThreadInterface(m_headThreadId)->PostTask(spHeadTask);
}

bool NetServer::onFirstReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type)
{
	return false;
}

void NetServer::onClientConnected(uv_tcp_t* client)
{
	int x = 3;
}

void NetServer::onClientDisconnected(uv_tcp_t* client)
{
	int x = 3;
}

void NetServer::onHeart()
{

}

int32_t NetServer::headNumber()
{
	return 0;
}

void NetServer::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
	NetWorkHelper::receive(sender, buffer, length, m_receiveAreaMap[sender], &m_receiveQueue, &m_receiveSemaphore, this);
}

void NetServer::onReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type)
{

}

void NetServer::uvClientConnected(uv_tcp_t* client)
{
	m_allClient.push_back(client);
	onClientConnected(client);
}

void NetServer::uvClientDisconnected(uv_tcp_t* client)
{
	onClientDisconnected(client);
}

void NetServer::uvDisconnectedClear(uv_tcp_t* tcp)
{
	auto it = std::find(m_allClient.begin(), m_allClient.end(), tcp);
	if (it != m_allClient.end())
	{
		m_allClient.erase(it);
	}
	auto itArea = m_receiveAreaMap.find(tcp);
	if (itArea != m_receiveAreaMap.end())
	{
		m_receiveAreaMap.erase(itArea);
	}
}

void NetServer::loop()
{
	if (m_loopThreadId == 0)
	{
		m_loopThreadId = CTaskThreadManager::Instance().Init();
	}
	std::shared_ptr<RunLoopTask> spRunLoopTask(new RunLoopTask);
	spRunLoopTask->setParam(this);
	CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId)->PostTask(spRunLoopTask);
}

void NetServer::asyncClose()
{
	if (m_headThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_headThreadId);
		m_headThreadId = 0;
	}

	if (m_receiveThreadId != 0)
	{
		CTaskThreadManager::Instance().Uninit(m_receiveThreadId);
		m_receiveThreadId = 0;
	}

	if (m_loopThreadId != 0)
	{
		CTaskThreadManager::Instance().GetThreadInterface(m_loopThreadId)->StopAllTask();
	}

	m_head[0] = 0;
	m_head[1] = 0;
	m_isListen = false;
}

//#include "ConsoleTest.h"
//#include <stdint.h>
//#include <stdio.h>
//#include "NetWork/NetWorkAPI.h"
//#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
//#include "CStringManager/CStringManagerAPI.h"
//
//BOOL CALLBACK ConsoleHandler(DWORD eve)
//{
//	if (eve == CTRL_CLOSE_EVENT)
//	{
//		//关闭退出事件
//		//RCSend("close ConsoleTest1");
//	}
//	return FALSE;
//}
//
//int32_t consoleCloseResult = ::SetConsoleCtrlHandler(ConsoleHandler, TRUE);
//
//std::mutex g_mu;
//
//class Client : public NetClient
//{
//public:
//	virtual void onReceive(char* buffer, int32_t length, int32_t type)
//	{
//		std::unique_lock<std::mutex> lock(g_mu);
//		RCSend("client = %s", buffer);
//		int x = 3;
//	}
//	virtual void onServerConnected()
//	{
//		int x = 3;
//		//send("server connected", 16, CorrespondParam::CLIENT_INIT);
//		//RCSend("onServerConnected");
//	}
//
//	virtual void onHeart()
//	{
//		RCSend("heart");
//	}
//};
//
//class Client2 : public NetClient
//{
//public:
//	virtual void onReceive(char* buffer, int32_t length, int32_t type)
//	{
//		printf("client2 = %s\n", buffer);
//		int x = 3;
//	}
//	virtual void onServerConnected()
//	{
//		int x = 3;
//		//send("server connected", 16, CorrespondParam::CLIENT_INIT);
//		//RCSend("onServerConnected");
//	}
//};
//
//class Server : public NetServer
//{
//public:
//	virtual void onReceive(uv_tcp_t* client, char* buffer, int32_t length, int32_t type)
//	{
//		{
//			std::unique_lock<std::mutex> lock(g_mu);
//			std::string str = CStringManager::Format("server length = %d, c = ", length);
//			int32_t index = -1;
//			while (index++ != length - 1)
//			{
//				char c = buffer[index];
//				str = str + CStringManager::Format("%d", (int32_t)c) + " ";
//			}
//			str = CStringManager::Format((str + "buffer = %s").c_str(), buffer);
//			if (CStringManager::Find(str, "0 0 0 2 0 0 0 ") != -1)
//			{
//				//::MessageBox(0, 0, 0, 0);
//			}
//			RCSend("%d_%s", type, str.c_str());
//		}
//
//		send(client, "server received", 15, CorrespondParam::CLIENT_INIT);
//		int x = 3;
//	}
//	virtual void onClientConnected(uv_tcp_t* client)
//	{
//		int x = 3;
//		//RCSend("onClientConnected");
//		//send(client, "client connected", 16, CorrespondParam::CLIENT_INIT);
//	}
//
//	virtual void onHeart()
//	{
//		RCSend("heart");
//	}
//};
//
//class Task : public CTask
//{
//public:
//	void DoTask()
//	{
//		int x = m_count;
//		if (x != 1)
//		{
//			int x = 3;
//		}
//		while (x-- != 0)
//		{
//			std::string str = CStringManager::Format("x = %d, i = %d", x, m_i);
//			m_client->send(str.c_str(), str.length());
//		}
//	}
//
//	void setParam(NetClient* client, int32_t i, int32_t count)
//	{
//		m_client = client;
//		m_i = i;
//		m_count = count;
//	}
//
//private:
//	NetClient* m_client;
//	int32_t m_i;
//	int32_t m_count;
//};
//
//int32_t main()
//{
//	Server server;
//	server.listen(7000);
//
//	int32_t count = 50;
//	std::vector<Client*> vecClient;
//	std::vector<uint32_t> vecThreadId;
//	int32_t index = -1;
//	while (index++ != count - 1)
//	{
//		Client* client = new Client;
//		client->connect("127.0.0.1", 7000, true);
//		vecClient.push_back(client);
//	}
//	Sleep(1000);
//
//
//	index = -1;
//	while (index++ != count - 1)
//	{
//		uint32_t threadId = CTaskThreadManager::Instance().Init();
//		vecThreadId.push_back(threadId);
//		std::shared_ptr<Task> spTask(new Task);
//		spTask->setParam(vecClient[index], index + 1, 10000);
//		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index])->PostTask(spTask);
//	}
//
//	int a = 0;
//	scanf("%d", &a);
//	index = -1;
//	while (index++ != count - 1)
//	{
//		std::shared_ptr<Task> spTask(new Task);
//		spTask->setParam(vecClient[index], index + 1, 1);
//		CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index])->PostTask(spTask);
//		//RCSend("wait = %d, id = %d", CTaskThreadManager::Instance().GetThreadInterface(vecThreadId[index])->GetWaitTaskCount(), vecThreadId[index]);
//	}
//
//	while (true)
//	{
//		Sleep(1000);
//	}
//	return 0;
//}