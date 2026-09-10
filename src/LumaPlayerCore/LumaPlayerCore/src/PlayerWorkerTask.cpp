#ifdef _MSC_VER
#pragma execution_character_set("utf-8")
#endif
#include "PlayerWorkerTask.h"
#include "PlayerEngine.h"

PlayerWorkerTask::PlayerWorkerTask(PlayerEngine* engine) :
CTask(kTaskId),
m_engine(engine),
m_exit(false)
{

}

PlayerWorkerTask::~PlayerWorkerTask()
{

}

void PlayerWorkerTask::DoTask()
{
	if (m_exit.load())
	{
		return;
	}
	if (m_engine != nullptr)
	{
		m_engine->workerLoop(this);
	}
}

void PlayerWorkerTask::StopTask()
{
	m_exit = true;
	wake();
}

void PlayerWorkerTask::wake()
{
	m_wakeup.signal();
}

bool PlayerWorkerTask::wait(int32_t timeoutMs)
{
	if (m_exit)
	{
		return false;
	}
	if (timeoutMs <= 0)
	{
		return true;
	}
	return m_wakeup.wait(timeoutMs);
}

bool PlayerWorkerTask::isExitRequested() const
{
	return m_exit;
}