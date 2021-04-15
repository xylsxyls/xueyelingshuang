#include "LogDeleteTask.h"
#include "CSystem/CSystemAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "CorrespondParam/CorrespondParamAPI.h"
#include "LogTask.h"

LogDeleteTask::LogDeleteTask():
m_logThread(nullptr),
m_lastLogTime(nullptr),
m_needSendDeleteLog(nullptr),
m_exit(false)
{

}

void LogDeleteTask::DoTask()
{
	while (!m_exit)
	{
		if ((*m_needSendDeleteLog) && (CSystem::GetTickCount() - *m_lastLogTime > 5000) && ((*m_lastLogTime) != 0))
		{
			ProtoMessage message;
			message[LOG_UNINIT] = (int32_t)true;
			std::string strMessage = message.toString();
			std::shared_ptr<LogTask> spLogTask(new LogTask);
			spLogTask->setParam(false, strMessage, "");
			m_logThread->PostTask(spLogTask);
			*m_needSendDeleteLog = false;
		}
		CSystem::Sleep(10);
	}
}

void LogDeleteTask::StopTask()
{
	m_exit = true;
}

void LogDeleteTask::setParam(std::atomic<bool>* needSendDeleteLog, std::atomic<int32_t>* lastLogTime, const std::shared_ptr<CTaskThread>& logThread)
{
	m_needSendDeleteLog = needSendDeleteLog;
	m_lastLogTime = lastLogTime;
	m_logThread = logThread;
}