#include "ReceiveCallbackBase.h"
#include "NetWorkThreadManager.h"
#include <string>
#include "WorkTask.h"

ReceiveCallbackBase::ReceiveCallbackBase():
m_callback(nullptr)
{

}

void ReceiveCallbackBase::receive(uv_tcp_t* sender, char* buffer, int32_t length)
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

void ReceiveCallbackBase::setCallback(CallbackBase* callback)
{
	m_callback = callback;
}