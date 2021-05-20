#include "NetReceive.h"
#include "ScreenTask.h"
#include "LogTask.h"
#include "CSystem/CSystemAPI.h"
#include "Semaphore/SemaphoreAPI.h"

NetReceive::NetReceive():
m_semaphore(nullptr),
m_lastLogTime(nullptr),
m_spScreenThread(nullptr),
m_spLogThread(nullptr)
{

}

void NetReceive::clientInitResponse(int32_t serverId, const char* buffer, int32_t length)
{
	printf("CLIENT_INIT response, serverId = %d, buffer = %s, length = %d\n", serverId, buffer, length);
	m_semaphore->signal();
}

void NetReceive::ServerMessage(int32_t serverId, const char* buffer, int32_t length)
{
	std::string strBuffer(buffer, length);

	std::shared_ptr<ScreenTask> spScreenTask(new ScreenTask);
	spScreenTask->setParam(true, strBuffer);
	m_spScreenThread->PostTask(spScreenTask);

	std::shared_ptr<LogTask> spLogTask(new LogTask);
	spLogTask->setParam(true, strBuffer);
	m_spLogThread->PostTask(spLogTask);

	*m_lastLogTime = CSystem::GetTickCount();
}

void NetReceive::setInitResponseSemaphore(Semaphore* semaphore)
{
	m_semaphore = semaphore;
}

void NetReceive::setThread(const std::shared_ptr<CTaskThread>& spScreenThread, const std::shared_ptr<CTaskThread>& spLogThread)
{
	m_spScreenThread = spScreenThread;
	m_spLogThread = spLogThread;
}

void NetReceive::setLastLogTime(std::atomic<int32_t>* lastLogTime)
{
	m_lastLogTime = lastLogTime;
}