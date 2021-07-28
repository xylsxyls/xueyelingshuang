#include "NetWorkHelper.h"
#include "WorkTask.h"
#ifdef __unix__
#include <string.h>
#endif

char* NetWorkHelper::send(uv_tcp_t* dest, const char* buffer, int32_t length, int32_t type)
{
#if defined _WIN64 || defined __x86_64__
	int32_t ptrSize = 8;
#else
	int32_t ptrSize = 4;
#endif // _WIN64
	char* text = (char*)::malloc(length + ptrSize + 8);
#if defined _WIN64 || defined __x86_64__
	*(int64_t*)text = (int64_t)dest;
#else
	*(int32_t*)text = (int32_t)dest;
#endif // _WIN64

	*((int32_t*)(text + ptrSize)) = length + 4;
	*((int32_t*)(text + ptrSize + 4)) = type;
	if (buffer != nullptr)
	{
		::memcpy(text + ptrSize + 8, buffer, length);
	}
	return text;
}

void NetWorkHelper::receive(uv_tcp_t* sender,
	const char* buffer,
	int32_t length,
	std::string& receiveArea,
	uint32_t receiveThreadId,
	LibuvTcp* libuvTcp)
{
	int32_t vernier = 0;
	char* tagBuffer = nullptr;
	int32_t tagLength = 0;

	//����������ֵ
	if (!receiveArea.empty())
	{
		//�������ﳤ��С��4
		if (receiveArea.size() < 4)
		{
			//ʣ��ֵ�޷�����������4���ֽ�
			if (receiveArea.size() + length < 4)
			{
				//��ӽ�������
				receiveArea.append(buffer, length);
				return;
			}
			//ʣ��ֵ��������������4���ֽ�
			vernier = 4 - receiveArea.size();
			receiveArea.append(buffer, vernier);
		}
		//�������ﳤ�ȴ��ڵ���4
		//�������С
		tagLength = *(int32_t*)&receiveArea[0];
		//ʣ��ֵ�޷�����һ����
		if (length - vernier + (int32_t)(receiveArea.size() - 4) < tagLength)
		{
			//��ӽ�������
			receiveArea.append(buffer + vernier, length - vernier);
			return;
		}
		//ʣ��ֵ��������һ����
		int32_t addSize = tagLength + 4 - receiveArea.size();
		receiveArea.append(buffer + vernier, addSize);
		vernier += addSize;
		//receive
		std::shared_ptr<WorkTask> spWorkTask(new WorkTask);
		//�������СΪ0�����ָ��
		if (tagLength == 0)
		{
			spWorkTask->setParam(sender, nullptr, 0, libuvTcp);
		}
		else
		{
			char* allocBuffer = (char*)::malloc(tagLength + 1);
			allocBuffer[tagLength] = 0;
			::memcpy(allocBuffer, &receiveArea[4], tagLength);
			spWorkTask->setParam(sender, allocBuffer, tagLength, libuvTcp);
		}
		CTaskThreadManager::Instance().GetThreadInterface(receiveThreadId)->PostTask(spWorkTask);
		//��ջ�����
		receiveArea.clear();
	}
	//�������ǿյ�
	while (true)
	{
		//ʣ��ֵ����С��4
		if (length - vernier < 4)
		{
			//��ӽ�������
			receiveArea.append(buffer + vernier, length - vernier);
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
			receiveArea.append(buffer + vernier - 4, length - vernier + 4);
			return;
		}
		//ʣ��ֵ��������һ����
		std::shared_ptr<WorkTask> spWorkTask(new WorkTask);
		//�������СΪ0�����ָ��
		if (tagLength == 0)
		{
			spWorkTask->setParam(sender, nullptr, 0, libuvTcp);
		}
		else
		{
			char* allocBuffer = (char*)::malloc(tagLength + 1);
			allocBuffer[tagLength] = 0;
			::memcpy(allocBuffer, buffer + vernier, tagLength);
			spWorkTask->setParam(sender, allocBuffer, tagLength, libuvTcp);
		}
		CTaskThreadManager::Instance().GetThreadInterface(receiveThreadId)->PostTask(spWorkTask);
		vernier += tagLength;
	}
}