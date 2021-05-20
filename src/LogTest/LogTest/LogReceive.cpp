#include "LogReceive.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenTask.h"
#include "LogTask.h"
#include "NetTask.h"
#include "LogManager/LogManagerAPI.h"
#include "NetSender/NetSenderAPI.h"
#include "LogDeleteTask.h"

LogReceive::LogReceive():
m_spScreenThread(nullptr),
m_spLogThread(nullptr),
m_spNetThread(nullptr),
m_lastLogTime(nullptr)
{
	
}

void LogReceive::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
	switch (type)
	{
	case LOGTEST_MESSAGE:
	{
		//·¢ËÍ¸øÍøÂç¶Ë
		std::string processName = getSenderName(sendPid);
		std::string strBuffer(buffer, length);

		std::shared_ptr<ScreenTask> spScreenTask(new ScreenTask);
		spScreenTask->setParam(false, strBuffer);
		m_spScreenThread->PostTask(spScreenTask);

		std::shared_ptr<LogTask> spLogTask(new LogTask);
		spLogTask->setParam(false, strBuffer, processName);
		m_spLogThread->PostTask(spLogTask);

		*m_lastLogTime = CSystem::GetTickCount();

		std::shared_ptr<NetTask> spNetTask(new NetTask);
		spNetTask->setParam(strBuffer, processName, m_loginName);
		m_spNetThread->PostTask(spNetTask);
	}
	break;
	default:
		break;
	}
}

std::string LogReceive::getSenderName(int32_t sendPid)
{
	auto it = m_sendMap.find(sendPid);
	if (it != m_sendMap.end())
	{
		return it->second;
	}
	std::string processName = CSystem::GetName(CSystem::processName(sendPid), 1);
	m_sendMap[sendPid] = processName;
	return processName;
}

void LogReceive::setThread(const std::shared_ptr<CTaskThread>& spScreenThread,
	const std::shared_ptr<CTaskThread>& spLogThread,
	const std::shared_ptr<CTaskThread>& spNetThread)
{
	m_spScreenThread = spScreenThread;
	m_spLogThread = spLogThread;
	m_spNetThread = spNetThread;
}

void LogReceive::setLastLogTime(std::atomic<int32_t>* lastLogTime)
{
	m_lastLogTime = lastLogTime;
}

void LogReceive::setLoginName(const std::string& loginName)
{
	m_loginName = loginName;
}