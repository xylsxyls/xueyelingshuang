#include "ReceiveNetTask.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include "NetClient.h"
#include "NetServer.h"
#include "Semaphore/SemaphoreAPI.h"

ReceiveNetTask::ReceiveNetTask():
m_receiveQueue(nullptr),
m_receiveSemaphore(nullptr),
m_libuvTcp(nullptr),
m_exit(false)
{
	
}

void ReceiveNetTask::DoTask()
{
#if defined _WIN64 || defined __x86_64__
	int32_t ptrSize = 8;
#else
	int32_t ptrSize = 4;
#endif // _WIN64
	while (!m_exit)
	{
		m_receiveSemaphore->wait();
		if (m_exit)
		{
			return;
		}
		char* buffer = nullptr;
		m_receiveQueue->pop(&buffer);
		if (buffer == nullptr)
		{
			printf("receive buffer empty\n");
		}

		uv_tcp_t* sender = *(uv_tcp_t**)buffer;
		int32_t length = *((int32_t*)(buffer + ptrSize));
		MessageType type = (MessageType)(*(int32_t*)(buffer + ptrSize + 4));

		if (m_libuvTcp->isClient())
		{
			if (type == MessageType::HEART)
			{
				((NetClient*)m_libuvTcp)->onHeart();
			}
			else if (type == MessageType::HEAD)
			{
				//只有第一个会放进来
				((NetClient*)m_libuvTcp)->onFirstHead();
			}
			else
			{
				((NetClient*)m_libuvTcp)->onReceive((length == 0 ? nullptr : (buffer + ptrSize + 12)), length - 8, type);
			}
		}
		else
		{
			if (type == MessageType::HEART)
			{
				((NetServer*)m_libuvTcp)->onHeart();
			}
			else
			{
				((NetServer*)m_libuvTcp)->onReceive(sender, (length == 0 ? nullptr : (buffer + ptrSize + 12)), length - 8, type);
			}
		}

		if (buffer == nullptr)
		{
			//加了type之后包大小不可能为0
			continue;
		}
		delete[] buffer;
	}

	while (!m_receiveQueue->empty())
	{
		char* buffer = nullptr;
		m_receiveQueue->pop(&buffer);
		if (buffer != nullptr)
		{
			delete[] buffer;
		}
	}
}

void ReceiveNetTask::StopTask()
{
	m_exit = true;
	m_receiveSemaphore->signal();
}

void ReceiveNetTask::setParam(LockFreeQueue<char*>* receiveQueue, Semaphore* receiveSemaphore, LibuvTcp* libuvTcp)
{
	m_receiveQueue = receiveQueue;
	m_receiveSemaphore = receiveSemaphore;
	m_libuvTcp = libuvTcp;
}