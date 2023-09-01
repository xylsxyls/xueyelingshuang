#include "ClearTask.h"
#include "Semaphore/SemaphoreAPI.h"

ClearTask::ClearTask():
m_vecTextMessage(nullptr),
m_semaphore(nullptr)
{

}

ClearTask::~ClearTask()
{
	m_semaphore->signal();
}

void ClearTask::DoTask()
{
	m_vecTextMessage->clear();
}

void ClearTask::setParam(std::vector<std::shared_ptr<TextMessage>>* vecTextMessage, Semaphore* semaphore)
{
	m_vecTextMessage = vecTextMessage;
	m_semaphore = semaphore;
}
