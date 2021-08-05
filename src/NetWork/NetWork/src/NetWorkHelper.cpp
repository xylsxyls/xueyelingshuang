#include "NetWorkHelper.h"
#include "ReceiveNetTask.h"
#ifdef __unix__
#include <string.h>
#endif
#include "Semaphore/SemaphoreAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "NetServer.h"
#include "NetClient.h"
#include "HeadTask.h"
#include "LibuvTcp/LibuvTcpAPI.h"

char* NetWorkHelper::send(LibuvTcp* libuvTcp, uv_tcp_t* dest, const char* buffer, int32_t length, int32_t type, int32_t head)
{
#if defined _WIN64 || defined __x86_64__
	int32_t ptrSize = 8;
#else
	int32_t ptrSize = 4;
#endif // _WIN64
	char* text = libuvTcp->memoryForSend(ptrSize + (head == -1 ? 8 : 12) + length);
#if defined _WIN64 || defined __x86_64__
	*(int64_t*)text = (int64_t)dest;
#else
	*(int32_t*)text = (int32_t)dest;
#endif // _WIN64

	*((int32_t*)(text + ptrSize)) = length + (head == -1 ? 4 : 8);
	*((int32_t*)(text + ptrSize + 4)) = type;
	if (head != -1)
	{
		*((int32_t*)(text + ptrSize + 8)) = head;
	}
	if (buffer != nullptr)
	{
		::memcpy(text + ptrSize + (head == -1 ? 8 : 12), buffer, length);
	}
	return text;
}

void NetWorkHelper::receive(uv_tcp_t* sender,
	const char* buffer,
	int32_t length,
	Area& receiveArea,
	LockFreeQueue<char*>* receiveQueue,
	Semaphore* receiveSemaphore,
	LibuvTcp* libuvTcp)
{
	//����Ѿ����յ���һ��������ûͨ���򷵻�
	if (receiveArea.m_isReceiveFirst && !receiveArea.m_isFirstPass)
	{
		return;
	}

	int32_t vernier = 0;
	char* tagBuffer = nullptr;
	int32_t tagLength = 0;
	MessageType type = INIT;
	int32_t head = 0;
#if defined _WIN64 || defined __x86_64__
	int32_t ptrSize = 8;
#else
	int32_t ptrSize = 4;
#endif

	NetClient* netClient = nullptr;
	NetServer* netServer = nullptr;
	if (libuvTcp->isClient())
	{
		netClient = (NetClient*)libuvTcp;
	}
	else
	{
		netServer = (NetServer*)libuvTcp;
	}

	//�����δ�յ���һ����
	if (!receiveArea.m_isReceiveFirst)
	{
		//���յ�8���ֽ�
		if (!necessaryReceive(buffer, length, 8, vernier, receiveArea.m_area))
		{
			return;
		}

		//���������Ѿ���8���ֽ���
		//�������С��tagLength��ʾtype+�����ܴ�С
		tagLength = *(int32_t*)(&receiveArea.m_area[0]);
		int32_t firstMessageLength = tagLength - 4;
		type = (MessageType)*(int32_t*)(&receiveArea.m_area[4]);
		if (type != FIRST_MESSAGE || firstMessageLength != (netClient != nullptr ? netClient->m_firstMessageLength : netServer->m_firstMessageLength))
		{
			receiveArea.m_isReceiveFirst = true;
			receiveArea.m_isFirstPass = false;

			if (libuvTcp->isClient())
			{
				netClient->onFirstReceive(nullptr, firstMessageLength, type);
				netClient->asyncClose();
			}
			else
			{
				netServer->onFirstReceive(sender, nullptr, firstMessageLength, type);
				libuvTcp->close(sender);
			}
			return;
		}

		//���յ�һ��������
		if (!necessaryReceive(buffer, length, tagLength + 4, vernier, receiveArea.m_area))
		{
			return;
		}

		//���������Ѿ���һ��������
		receiveArea.m_isReceiveFirst = true;
		if (libuvTcp->isClient())
		{
			receiveArea.m_isFirstPass = netClient->onFirstReceive(&receiveArea.m_area[8], tagLength - 4, type);
			if (!receiveArea.m_isFirstPass)
			{
				netClient->asyncClose();
			}
			else
			{
				if (netClient->m_isSendHeart)
				{
					netClient->heart();
				}
			}
		}
		else
		{
			receiveArea.m_isFirstPass = netServer->onFirstReceive(sender, &receiveArea.m_area[8], tagLength - 4, type);
			if (!receiveArea.m_isFirstPass)
			{
				libuvTcp->close(sender);
			}
			else
			{
				netServer->send(sender, (const char*)&(netServer->m_head[0]), 4, MessageType::HEAD);
			}
		}

		receiveArea.m_area.clear();
	}

	while (true)
	{
		//necessary������һ�ν���������
		//���յ�12���ֽ�
		if (!necessaryReceive(buffer, length, 12, vernier, receiveArea.m_area))
		{
			return;
		}

		//�������Ѿ���12���ֽ���
		//�������С��tagLength��ʾtype+head+�����ܴ�С
		tagLength = *(int32_t*)(&receiveArea.m_area[0]);
		type = (MessageType)(*(int32_t*)(&receiveArea.m_area[4]));
		head = *(int32_t*)(&receiveArea.m_area[8]);
		//���ֿ������ӵİ�ͷ�쳣�����ǰ���н�������ȫ��������������ᷢ����������
		if (netServer != nullptr &&
			type != MessageType::HEAD &&
			head != netServer->m_head[0] &&
			head != netServer->m_head[1])
		{
			receiveArea.m_area.clear();
			return;
		}
		if (netClient != nullptr &&
			type != MessageType::HEAD &&
			netClient->m_head[0] != 0 &&
			head != netClient->m_head[0] &&
			head != netClient->m_head[1])
		{
			receiveArea.m_area.clear();
			return;
		}

		//��ͷͨ��
		//���յ�һ��������
		if (!necessaryReceive(buffer, length, tagLength + 4, vernier, receiveArea.m_area))
		{
			return;
		}
		//receive
		char* allocBuffer = (char*)::malloc(ptrSize + 4 + (tagLength == 8 ? tagLength : (tagLength + 1)));
		*(uv_tcp_t**)allocBuffer = sender;
		::memcpy(allocBuffer + ptrSize, &receiveArea.m_area[0], 12);
		if (tagLength != 8)
		{
			::memcpy(allocBuffer + ptrSize + 12, &receiveArea.m_area[12], tagLength - 8 + 1);
		}
		receiveQueue->push(allocBuffer);
		receiveSemaphore->signal();
		//��ջ�����
		receiveArea.m_area.clear();
	}
}

bool NetWorkHelper::necessaryReceive(const char* buffer,
	int32_t length,
	int32_t receiveCount,
	int32_t& vernier,
	std::string& receiveArea)
{
	if ((int32_t)receiveArea.size() >= receiveCount)
	{
		return true;
	}

	if ((int32_t)receiveArea.size() + length - vernier < receiveCount)
	{
		if (length - vernier != 0)
		{
			receiveArea.append(buffer + vernier, length - vernier);
		}
		else if (length - vernier < 0)
		{
			printf("necessary error\n");
		}
		return false;
	}

	int32_t appendCount = receiveCount - receiveArea.size();
	receiveArea.append(buffer + vernier, appendCount);
	vernier += appendCount;
	
	return true;
}