#include "LogReceive.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenTask.h"
#include "LogTask.h"
#include "NetTask.h"
#include "LogManager/LogManagerAPI.h"
#include "NetSender/NetSenderAPI.h"
#include "LogDeleteTask.h"

LogReceive::LogReceive():
m_netClientManagerPid(0),
m_screenThreadId(0),
m_logThreadId(0),
m_logDeleteThreadId(0),
m_netThreadId(0),
m_screenThread(nullptr),
m_logThread(nullptr),
m_logDeleteThread(nullptr),
m_netThread(nullptr),
m_needSendDeleteLog(false),
m_lastLogTime(0)
{
#ifdef _MSC_VER
	m_netClientManagerPid = CSystem::processFirstPid("NetClientManager1.0.exe");
#elif __unix__
	m_netClientManagerPid = CSystem::processFirstPid("NetClientManager1.0");
#endif

	m_screenThreadId = CTaskThreadManager::Instance().Init();
	m_logThreadId = CTaskThreadManager::Instance().Init();
	m_logDeleteThreadId = CTaskThreadManager::Instance().Init();
	m_netThreadId = CTaskThreadManager::Instance().Init();

	m_screenThread = CTaskThreadManager::Instance().GetThreadInterface(m_screenThreadId);
	m_logThread = CTaskThreadManager::Instance().GetThreadInterface(m_logThreadId);
	m_logDeleteThread = CTaskThreadManager::Instance().GetThreadInterface(m_logDeleteThreadId);
	m_netThread = CTaskThreadManager::Instance().GetThreadInterface(m_netThreadId);

	m_lastLogTime = CSystem::GetTickCount();

	std::shared_ptr<LogDeleteTask> spLogDeleteTask(new LogDeleteTask);
	spLogDeleteTask->setParam(&m_needSendDeleteLog, &m_lastLogTime, m_logThread);
	m_logDeleteThread->PostTask(spLogDeleteTask);
}

LogReceive::~LogReceive()
{
	CTaskThreadManager::Instance().Uninit(m_screenThreadId);
	CTaskThreadManager::Instance().Uninit(m_logDeleteThreadId);
	CTaskThreadManager::Instance().Uninit(m_logThreadId);
	CTaskThreadManager::Instance().Uninit(m_netThreadId);
}

void LogReceive::receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	if (sendPid == m_netClientManagerPid)
	{
		//从网络端接收
		receiveFromNet(buffer, length, protocolId);
	}
	else
	{
		//发送给网络端
		sendToNet(sendPid, buffer, length, protocolId);
	}
}

void LogReceive::receiveFromNet(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT process, length = %d\n", length);
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		std::string strBuffer(buffer, length);

		std::shared_ptr<ScreenTask> spScreenTask(new ScreenTask);
		spScreenTask->setParam(true, strBuffer);
		m_screenThread->PostTask(spScreenTask);

		std::shared_ptr<LogTask> spLogTask(new LogTask);
		spLogTask->setParam(true, strBuffer);
		m_logThread->PostTask(spLogTask);

		m_needSendDeleteLog = true;
		m_lastLogTime = CSystem::GetTickCount();
	}
	break;
	default:
		break;
	}
}

void LogReceive::sendToNet(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	std::string processName = getSenderName(sendPid);
	std::string strBuffer(buffer, length);

	std::shared_ptr<ScreenTask> spScreenTask(new ScreenTask);
	spScreenTask->setParam(false, strBuffer);
	m_screenThread->PostTask(spScreenTask);
	
	std::shared_ptr<LogTask> spLogTask(new LogTask);
	spLogTask->setParam(false, strBuffer, processName);
	m_logThread->PostTask(spLogTask);

	m_needSendDeleteLog = true;
	m_lastLogTime = CSystem::GetTickCount();
	
	std::shared_ptr<NetTask> spNetTask(new NetTask);
	spNetTask->setParam(strBuffer, processName, protocolId);
	m_netThread->PostTask(spNetTask);
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