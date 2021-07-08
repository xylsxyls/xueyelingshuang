#include "SendTask.h"
#include "Semaphore/SemaphoreAPI.h"
#include "ProcessWork.h"
#include "CorrespondParam/CorrespondParamAPI.h"

SendTask::SendTask():
m_waitEndPost(nullptr),
m_postQueue(nullptr),
m_postSemaphore(nullptr),
m_exit(false)
{

}

void SendTask::DoTask()
{
	while (!m_exit)
	{
		m_postSemaphore->wait();
		if (m_exit || (*m_waitEndPost && m_postQueue->empty()))
		{
			break;
		}

		char* postBuffer = nullptr;
		m_postQueue->pop(&postBuffer);
		ProcessWork::instance().send(*(int32_t*)postBuffer,
			postBuffer + 12,
			*((int32_t*)postBuffer + 1),
			(MessageType)*((int32_t*)postBuffer + 2));

		delete[] postBuffer;
	}
}

void SendTask::StopTask()
{
	m_exit = true;
	m_postSemaphore->signal();
}

void SendTask::setParam(std::atomic<bool>* waitEndPost, LockFreeQueue<char*>* postQueue, Semaphore* postSemaphore)
{
	m_waitEndPost = waitEndPost;
	m_postQueue = postQueue;
	m_postSemaphore = postSemaphore;
}