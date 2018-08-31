#include "NetServer.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "ServerCallback.h"
#include "D:\\SendToMessageTest.h"
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include "WorkTask.h"
#include "SendTask.h"
#include "NetWorkThreadManager.h"

std::mutex g_netServerMu;

class ServerCallbackBase : public ReceiveCallback
{
public:
	ServerCallbackBase();

public:
	virtual void receive(uv_tcp_t* sender, char* buffer, int32_t length);

	virtual void clientConnected(uv_tcp_t* client);

	void setCallback(ServerCallback* callback);

public:
	ServerCallback* m_callback;
	std::map<uv_tcp_t*, std::string> m_area;
};

ServerCallbackBase::ServerCallbackBase() :
m_callback(nullptr)
{
	m_callback = new ServerCallback;
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

	//����������ֵ
	if (!senderArea.empty())
	{
		//�������ﳤ��С��4
		if (senderArea.size() < 4)
		{
			//ʣ��ֵ�޷�����������4���ֽ�
			if (senderArea.size() + length < 4)
			{
				//��ӽ�������
				senderArea.append(buffer, length);
				return;
			}
			//ʣ��ֵ��������������4���ֽ�
			vernier = 4 - senderArea.size();
			senderArea.append(buffer, vernier);
		}
		//�������ﳤ�ȴ��ڵ���4
		//�������С
		tagLength = *(int32_t*)&senderArea[0];
		//ʣ��ֵ�޷�����һ����
		if (length - vernier < tagLength)
		{
			//��ӽ�������
			senderArea.append(buffer + vernier, length - vernier);
			return;
		}
		//ʣ��ֵ��������һ����
		int32_t addSize = tagLength + 4 - senderArea.size();
		senderArea.append(buffer + vernier, addSize);
		vernier += addSize;
		//receive
		//�������СΪ0�����ָ��
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
		//��ջ�����
		senderArea.clear();
	}
	//�������ǿյ�
	while (true)
	{
		//ʣ��ֵ����С��4
		if (length - vernier < 4)
		{
			//��ӽ�������
			senderArea.append(buffer + vernier, length - vernier);
			return;
		}
		//ʣ��ֵ���ȴ��ڵ���4
		//�������С
		tagLength = *(int32_t*)(buffer + vernier);
		vernier += 4;
		//ʣ��ֵ�޷�����һ����
		if (length - vernier < tagLength)
		{
			//��ӽ�������
			senderArea.append(buffer + vernier - 4, length - vernier + 4);
			return;
		}
		//ʣ��ֵ��������һ����
		//�������СΪ0�����ָ��
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
m_serverCallbackBase(nullptr),
m_sendThreadId(0)
{
	m_libuvTcp = new LibuvTcp;
	m_serverCallbackBase = new ServerCallbackBase;
	NetWorkThreadManager::instance().init(m_libuvTcp->m_coreCount);
	m_sendThreadId = NetWorkThreadManager::instance().giveSendThreadId();
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
	//std::unique_lock<std::mutex> lock(g_netServerMu);
	if (m_libuvTcp == nullptr || dest == nullptr)
	{
		return;
	}

	char* text = (char*)::malloc(length + 8);
	*(int32_t*)text = (int32_t)dest;
	*((int32_t*)(text + 4)) = length;
	::memcpy(text + 8, buffer, length);

	std::shared_ptr<SendTask> spTask;
	SendTask* task = new SendTask;
	task->setLibuvTcp(m_libuvTcp);
	task->setParam(text);
	spTask.reset(task);
	NetWorkThreadManager::instance().postSendTaskToThreadPool(m_sendThreadId, spTask, 1);
}