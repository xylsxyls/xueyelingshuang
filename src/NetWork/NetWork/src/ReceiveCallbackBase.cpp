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
		vernier += tagLength;
	}
	return;
}

void ReceiveCallbackBase::setCallback(CallbackBase* callback)
{
	m_callback = callback;
}