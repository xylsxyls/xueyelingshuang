#include "NetServer.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "ServerCallback.h"
#include "D:\\SendToMessageTest.h"
#include "NetHelper.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"

class ServerCallbackBase : public ReceiveCallback
{
public:
	ServerCallbackBase(int32_t coreCount);

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	virtual void clientConnected(uv_tcp_t* client);

	void setCallback(ServerCallback* callback);

	uint32_t getCurrentWorkThreadId();

public:
	ServerCallback* m_callback;
	std::map<uv_tcp_t*, std::string> m_area;

	std::vector<uint32_t> m_vecWorkThreadId;
	int32_t m_threadIDCounter;
};

class WorkTask : public CTask
{
public:
	WorkTask():
		m_client(nullptr),
		m_buffer(nullptr),
		m_length(0),
		m_callback(nullptr)
	{

	}

public:
	void setCallback(ServerCallback* callback)
	{
		m_callback = callback;
	}

	void setParam(uv_tcp_t* client, char* buffer, int32_t length)
	{
		m_client = client;
		m_buffer = buffer;
		m_length = length;
	}

	virtual void DoTask()
	{
		m_callback->receive(m_client, m_buffer, m_length);
		if (m_buffer == nullptr)
		{
			return;
		}
		::free(m_buffer);
	}

private:
	uv_tcp_t* m_client;
	char* m_buffer;
	int32_t m_length;
	ServerCallback* m_callback;
};

ServerCallbackBase::ServerCallbackBase(int32_t coreCount) :
m_callback(nullptr),
m_threadIDCounter(-1)
{
	m_callback = new ServerCallback;
	int32_t index = -1;
	while (index++ != coreCount - 1)
	{
		m_vecWorkThreadId.push_back(CTaskThreadManager::Instance().Init());
	}
}

uint32_t ServerCallbackBase::getCurrentWorkThreadId()
{
	return m_vecWorkThreadId[(++m_threadIDCounter) % m_vecWorkThreadId.size()];
}

void ServerCallbackBase::receive(uv_tcp_t* sender, char* buffer, int32_t length)
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
			while (!CTaskThreadManager::Instance().GetThreadInterface(getCurrentWorkThreadId())->TryPostTask(spTask, 1));
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
			while (!CTaskThreadManager::Instance().GetThreadInterface(getCurrentWorkThreadId())->TryPostTask(spTask, 1));
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
			while (!CTaskThreadManager::Instance().GetThreadInterface(getCurrentWorkThreadId())->TryPostTask(spTask, 1));
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
			while (!CTaskThreadManager::Instance().GetThreadInterface(getCurrentWorkThreadId())->TryPostTask(spTask, 1));
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

void ServerCallbackBase::clientConnected(uv_tcp_t* client)
{
	if (m_callback == nullptr)
	{
		return;
	}
	m_callback->clientConnected(client);
	m_area[client] = std::string();
}

void ServerCallbackBase::setCallback(ServerCallback* callback)
{
	m_callback = callback;
}

NetServer::NetServer() :
m_libuvTcp(nullptr),
m_serverCallbackBase(nullptr)
{
	m_libuvTcp = new LibuvTcp;
	m_serverCallbackBase = new ServerCallbackBase(m_libuvTcp->m_coreCount);
}

void NetServer::listen(int32_t port, ServerCallback* callback)
{
	if (m_libuvTcp == nullptr || m_serverCallbackBase == nullptr || callback == nullptr)
	{
		return;
	}
	m_serverCallbackBase->setCallback(callback);
	callback->setNetServer(this);
	m_libuvTcp->initServer(port, m_serverCallbackBase);
	m_libuvTcp->serverLoop();
}

void NetServer::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr || dest == nullptr)
	{
		return;
	}

	char* text = (char*)::malloc(length + 4);
	*(int32_t*)text = length;
	memcpy(text + 4, buffer, length);
	m_libuvTcp->send(dest, text, length + 4);
	::free(text);
}