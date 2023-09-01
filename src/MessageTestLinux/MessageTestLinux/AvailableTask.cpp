#include "AvailableTask.h"
#include "MessageTestLinux.h"
#include "Semaphore/SemaphoreAPI.h"

AvailableTask::AvailableTask():
m_messageTestLinux(nullptr),
m_vecTextMessage(nullptr),
m_layer(0),
m_semaphore(nullptr),
m_availCount(nullptr),
m_exit(false)
{

}

AvailableTask::~AvailableTask()
{
	m_semaphore->signal();
}

void AvailableTask::DoTask()
{
	if (m_layer < 0)
	{
		return;
	}

	int32_t index = -1;
	while (index++ != m_layer && !m_exit)
	{
		if (m_messageTestLinux->textAvailable((*m_vecTextMessage)[index]))
		{
			++(*m_availCount);
		}
	}
}

void AvailableTask::StopTask()
{
	m_exit = true;
}

void AvailableTask::setParam(MessageTestLinux* messageTestLinux, std::vector<std::shared_ptr<TextMessage>>* vecTextMessage,
	int32_t layer, Semaphore* semaphore, int32_t* availCount)
{
	m_messageTestLinux = messageTestLinux;
	m_vecTextMessage = vecTextMessage;
	m_layer = layer;
	m_semaphore = semaphore;
	m_availCount = availCount;
}
