#include "NetServer.h"
#include "NetWorkHelper.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

NetServer::NetServer():
m_receiveThread(nullptr),
m_receiveThreadId(0)
{

}

void NetServer::listen(int32_t port)
{
	initServer(port);
	m_receiveThreadId = CTaskThreadManager::Instance().Init();
	m_receiveThread = CTaskThreadManager::Instance().GetThreadInterface(m_receiveThreadId);
	loop();
}

void NetServer::send(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type)
{
	LibuvTcp::send(client, buffer, length, type);
}

void NetServer::onClientConnected(uv_tcp_t* client)
{

}

void NetServer::onHeart()
{

}

void NetServer::receive(uv_tcp_t* sender, const char* buffer, int32_t length)
{
	NetWorkHelper::receive(sender, buffer, length, m_receiveAreaMap[sender], m_receiveThread, this);
}

void NetServer::onReceive(uv_tcp_t* client, const char* buffer, int32_t length, MessageType type)
{

}

void NetServer::clientConnected(uv_tcp_t* client)
{
	m_vecClient.push_back(client);
	onClientConnected(client);
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