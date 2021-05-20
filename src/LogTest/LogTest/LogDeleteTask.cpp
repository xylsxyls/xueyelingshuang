#include "LogDeleteTask.h"
#include "CSystem/CSystemAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LogTask.h"

LogDeleteTask::LogDeleteTask():
m_spLogThread(nullptr),
m_lastLogTime(nullptr),
m_exit(false)
{

}

void LogDeleteTask::DoTask()
{
	int32_t lastLogTime = 0;
	while (!m_exit)
	{
		if ((*m_lastLogTime != lastLogTime) && (CSystem::GetTickCount() - *m_lastLogTime > 5000) && ((*m_lastLogTime) != 0))
		{
			ProtoMessage message;
			message[LOG_UNINIT] = (int32_t)true;
			std::string strMessage = message.toString();
			std::shared_ptr<LogTask> spLogTask(new LogTask);
			spLogTask->setParam(false, strMessage, "");
			m_spLogThread->PostTask(spLogTask);
			lastLogTime = *m_lastLogTime;
		}
		CSystem::Sleep(1);
	}
}

void LogDeleteTask::StopTask()
{
	m_exit = true;
}

void LogDeleteTask::setParam(std::atomic<int32_t>* lastLogTime, const std::shared_ptr<CTaskThread>& spLogThread)
{
	m_lastLogTime = lastLogTime;
	m_spLogThread = spLogThread;
}