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

void ClientCallbackBase::serverConnected(uv_tcp_t* server)
{
	if (m_callback == nullptr || m_callback->m_netClient == nullptr)
	{
		return;
	}
	m_callback->m_netClient->setServer(server);
	m_callback->serverConnected(server);
}

void ClientCallbackBase::setCallback(ClientCallback* callback)
{
	m_callback = callback;
}

NetClient::NetClient() :
m_libuvTcp(nullptr),
m_clientCallbackBase(nullptr),
m_server(nullptr)
{
	m_libuvTcp = new LibuvTcp;
	m_clientCallbackBase = new ClientCallbackBase;
	NetWorkThreadManager::instance().init(m_libuvTcp->m_coreCount);
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
	m_libuvTcp->clientLoop();
}

void NetClient::send(char* buffer, int32_t length, uv_tcp_t* dest)
{
	if (m_libuvTcp == nullptr)
	{
		return;
	}
	if (dest == nullptr)
	{
		dest = m_server;
	}
	if (m_server == nullptr)
	{
		return;
	}

	std::shared_ptr<SendTask> spTask;
	SendTask* task = new SendTask;
	task->setLibuvTcp(m_libuvTcp);
	task->setParam(m_libuvTcp->getText(dest, buffer, length));
	spTask.reset(task);

	NetWorkThreadManager::instance().postWorkTaskToThreadPool(spTask);
	//RCSend("m_threadId = %d", m_threadId);
	//CTaskThreadManager::Instance().GetThreadInterface(m_threadId)->PostTask(spTask, 1);
	//m_libuvTcp->send(dest, text, length + 4);
	//::free(text);
}

void NetClient::setServer(uv_tcp_t* server)
{
	m_server = server;
}