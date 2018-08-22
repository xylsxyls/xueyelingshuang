#include "NetClient.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "ClientCallback.h"
#include "D:\\SendToMessageTest.h"
#include <map>
#include "SendTask.h"
#include "WorkTask.h"
#include "NetWorkThreadManager.h"
#include "CSystem/CSystemAPI.h"

std::atomic<int> netClientCalc = 0;

class ClientCallbackBase : public ReceiveCallback
{
public:
	ClientCallbackBase();

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	virtual void serverConnected(uv_tcp_t* server);

	void setCallback(ClientCallback* callback);

protected:
	ClientCallback* m_callback;
	std::map<uv_tcp_t*, std::string> m_area;
};

ClientCallbackBase::ClientCallbackBase():
m_callback(nullptr)
{
	//m_callback = new ClientCallback;
}

void ClientCallbackBase::receive(uv_tcp_t* sender, char* buffer, int32_t length)
{
	if (m_callback == nullptr)
	{
		return;
	}

	int32_t vernier = 0;
	char* tagBuffer = nullptr;
	int32_t tagLength = 0;
	std::string& senderArea = m_area[sender];

	//缓冲区里有值
	if (!senderArea.empty())
	{
		//缓冲区里长度小于4
		if (senderArea.size() < 4)
		{
			//剩余值无法填满缓冲区4个字节
			if (senderArea.size() + length < 4)
			{
				//添加进缓冲区
				senderArea.append(buffer, length);
				return;
			}
			//剩余值可以填满缓冲区4个字节
			vernier = 4 - senderArea.size();
			senderArea.append(buffer, vernier);
		}
		//缓冲区里长度大于等于4
		//计算包大小
		tagLength = *(int32_t*)&senderArea[0];
		//剩余值无法填满一个包
		if (length - vernier < tagLength)
		{
			//添加进缓冲区
			senderArea.append(buffer + vernier, length - vernier);
			return;
		}
		//剩余值可以填满一个包
		int32_t addSize = tagLength + 4 - senderArea.size();
		senderArea.append(buffer + vernier, addSize);
		vernier += addSize;
		//receive
		//如果包大小为0则给空指针
		if (tagLength == 0)
		{
			std::shared_ptr<WorkTask> spTask;
			WorkTask* task = new WorkTask;
			task->setCallback(m_callback);
			task->setParam(sender, nullptr, 0);
			spTask.reset(task);
			NetWorkThreadManager::instance().postWorkTaskToThreadPool(spTask);
		}
		else
		{
			char* allocBuffer = (char*)::malloc(tagLength + 1);
			allocBuffer[tagLength] = 0;
			::memcpy(allocBuffer, &senderArea[4], tagLength);
			std::shared_ptr<WorkTask> spTask;
			WorkTask* task = new WorkTask;
			task->setCallback(m_callback);
			task->setParam(sender, allocBuffer, tagLength);
			spTask.reset(task);
			NetWorkThreadManager::instance().postWorkTaskToThreadPool(spTask);
		}
		//清空缓冲区
		senderArea.clear();
	}
	//缓冲区是空的
	while (true)
	{
		//剩余值长度小于4
		if (length - vernier < 4)
		{
			//添加进缓冲区
			senderArea.append(buffer + vernier, length - vernier);
			return;
		}
		//剩余值长度大于等于4
		//计算包大小
		tagLength = *(int32_t*)(buffer + vernier);
		vernier += 4;
		//剩余值无法填满一个包
		if (length - vernier < tagLength)
		{
			//添加进缓冲区
			senderArea.append(buffer + vernier - 4, length - vernier + 4);
			return;
		}
		//剩余值可以填满一个包
		//如果包大小为0则给空指针
		if (tagLength == 0)
		{
			std::shared_ptr<WorkTask> spTask;
			WorkTask* task = new WorkTask;
			task->setCallback(m_callback);
			task->setParam(sender, nullptr, 0);
			spTask.reset(task);
			NetWorkThreadManager::instance().postWorkTaskToThreadPool(spTask);
		}
		else
		{
			char* allocBuffer = (char*)::malloc(tagLength + 1);
			allocBuffer[tagLength] = 0;
			::memcpy(allocBuffer, buffer + vernier, tagLength);
			std::shared_ptr<WorkTask> spTask;
			WorkTask* task = new WorkTask;
			task->setCallback(m_callback);
			task->setParam(sender, allocBuffer, tagLength);
			spTask.reset(task);
			NetWorkThreadManager::instance().postWorkTaskToThreadPool(spTask);
		}
		//if (ccc % 200000 == 0)
		//{
		//	printf("ccc = %d\n", (int)ccc);
		//
		//	int32_t index = -1;
		//	while (index++ != m_vecWorkThreadId.size() - 1)
		//	{
		//		int32_t threadId = m_vecWorkThreadId[index];
		//		int32_t count = CTaskThreadManager::Instance().GetThreadInterface(threadId)->GetWaitTaskCount();
		//		printf("threadId[%d].size = %d\n", threadId, count);
		//	}
		//}
		vernier += tagLength;
	}
	return;
}

void ClientCallbackBase::serverConnected(uv_tcp_t* server)
{
	//RCSend("clientcallbackbase in,%d,%d,%d", m_callback, m_callback->m_netClient, m_callback->m_netClient->m_libuvTcp);
	if (m_callback == nullptr || m_callback->m_netClient == nullptr)
	{
		return;
	}
	m_callback->m_netClient->setServer(server);
	//RCSend("serverConnected(server)");
	m_callback->serverConnected(server);
	//RCSend("end serverConnected(server)");
}

void ClientCallbackBase::setCallback(ClientCallback* callback)
{
	//RCSend("m_callback = %d", callback);
	m_callback = callback;
}

NetClient::NetClient() :
m_libuvTcp(nullptr),
m_clientCallbackBase(nullptr),
m_server(nullptr),
m_sendThreadId(0)
{
	m_libuvTcp = new LibuvTcp;
	//RCSend("m_libuvTcp = %d,netClient = %d", m_libuvTcp,this);
	m_clientCallbackBase = new ClientCallbackBase;
	NetWorkThreadManager::instance().init(m_libuvTcp->m_coreCount);
	m_sendThreadId = NetWorkThreadManager::instance().giveSendThreadId();
	//m_sendThreadId = CTaskThreadManager::Instance().Init();
}

void NetClient::connect(const char* ip, int32_t port, ClientCallback* callback)
{
	if (m_libuvTcp == nullptr || m_clientCallbackBase == nullptr || callback == nullptr)
	{
		return;
	}
	m_clientCallbackBase->setCallback(callback);
	callback->setNetClient(this);
	m_libuvTcp->initClient(ip, port, m_clientCallbackBase);
	//RCSend("connect,m_libuvTcp = %d", m_libuvTcp);
	m_libuvTcp->clientLoop();
}

void NetClient::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr)
	{
		RCSend("netClient = %d,m_libuvTcp = %d,buffer = %d,length = %d,dest = %d", this, m_libuvTcp, buffer, length, dest);
		return;
	}
	if (dest == nullptr)
	{
		dest = m_server;
	}
	if (m_server == nullptr)
	{
		RCSend("m_server == nullptr");
		return;
	}

	char* text = (char*)::malloc(length + 4);
	*(int32_t*)text = length;
	memcpy(text + 4, buffer, length);

	std::shared_ptr<SendTask> spTask;
	SendTask* task = new SendTask;
	task->setLibuvTcp(m_libuvTcp);
	task->setParam(dest, text, length + 4);
	spTask.reset(task);

	++netClientCalc;
	if (netClientCalc % 200000 == 0)
	{
		RCSend("netClientCalc = %d\n", netClientCalc);
	}
	//if (netClientCalc > 3999900)
	//{
	//	printf("netClientCalc = %d,m_libuvTcp = %d\n", netClientCalc, m_libuvTcp);
	//}

	NetWorkThreadManager::instance().postSendTaskToThreadPool(m_sendThreadId, spTask, 1);
	//RCSend("m_threadId = %d", m_threadId);
	//CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spTask, 1);
	//m_libuvTcp->send(dest, text, length + 4);
	//::free(text);
}

void NetClient::setServer(uv_tcp_t* server)
{
	m_server = server;
}