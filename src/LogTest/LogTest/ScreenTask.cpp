#include "ScreenTask.h"
#ifdef __unix__
#include "SendToMessageTest.h"
#endif
#include "Semaphore/SemaphoreAPI.h"
#include "CStringManager/CStringManagerAPI.h"

ScreenTask::ScreenTask():
m_screenSemaphore(nullptr),
m_screenQueue(nullptr),
m_exit(false)
{

}

void ScreenTask::DoTask()
{
	while (!m_exit)
	{
		m_screenSemaphore->wait();
		if (m_exit)
		{
			return;
		}

		m_screenQueue->pop(&m_buffer);

		if (m_buffer.size() >= 3 && CStringManager::Right(m_buffer, 3) == "NET")
		{
			m_buffer.pop_back();
			m_buffer.pop_back();
			m_buffer.pop_back();
			if (m_message.ParseFromString(m_buffer))
			{
#ifdef _MSC_VER
				RCSend("NET %s %s %s", m_message.logloginname().c_str(), m_message.logprocessname().c_str(), m_message.logbuffer().c_str());
#elif __unix__
				RCSend(m_message.logpeopleid(), m_message.logthreadid(), "NET %s %s %s", m_message.logloginname().c_str(), m_message.logprocessname().c_str(), m_message.logbuffer().c_str());
#endif
			}
			continue;
		}
		if (m_message.ParseFromString(m_buffer))
		{
#ifdef _MSC_VER
			RCSend("LOG %s", m_message.logbuffer().c_str());
#elif __unix__
			RCSend(m_message.logpeopleid(), m_message.logthreadid(), m_message.logprocesspid(), "LOG %s", m_message.logbuffer().c_str());
#endif
		}
	}
}

void ScreenTask::StopTask()
{
	m_exit = true;
	m_screenSemaphore->signal();
}

void ScreenTask::setParam(Semaphore* screenSemaphore, LockFreeQueue<std::string>* screenQueue)
{
	m_screenSemaphore = screenSemaphore;
	m_screenQueue = screenQueue;
}