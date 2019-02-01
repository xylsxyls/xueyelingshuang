#include "LogReceive.h"
#include "CSystem/CSystemAPI.h"
#include "ScreenTask.h"
#include "LogTask.h"
#include "NetTask.h"
#include "LogManager/LogManagerAPI.h"
#include "NetSender/NetSenderAPI.h"

LogReceive::LogReceive():
m_netClientManagerPid(0)
{
	m_netClientManagerPid = CSystem::processPid("NetClientManager1.0.exe");
	auto screenThreadId = CTaskThreadManager::Instance().Init();
	auto logThreadId = CTaskThreadManager::Instance().Init();
	auto netThreadId = CTaskThreadManager::Instance().Init();
	m_screenThread = CTaskThreadManager::Instance().GetThreadInterface(screenThreadId);
	m_logThread = CTaskThreadManager::Instance().GetThreadInterface(logThreadId);
	m_netThread = CTaskThreadManager::Instance().GetThreadInterface(netThreadId);
}

void LogReceive::receive(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId)
{
	if (sendPid == m_netClientManagerPid)
	{
		//从网络端接收
		receiveFromNet(buffer, length, protocolId);
	}
	else
	{
		//发送给网络端
		sendToNet(buffer, length, sendPid, protocolId);
	}
}

void LogReceive::receiveFromNet(char* buffer, int32_t length, CorrespondParam::ProtocolId protocolId)
{
	//printf("receive from net\n");
	switch (protocolId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT, length = %d\n", length);
		return;
	}
	case CorrespondParam::PROTO_MESSAGE:
	{
		std::string strBuffer(buffer, length);

		//m_message.clear();
		//m_message.from(strBuffer);
		//std::map<std::string, Variant> messageMap;
		//m_message.getMap(messageMap);
		//if ((int32_t)messageMap[LOG_IS_SEND_SCREEN] == (int32_t)true)
		//{
		//	RCSend("NET %s %s", messageMap[LOG_LOGIN_NAME].toString().c_str(), messageMap[LOG_BUFFER].toString().c_str());
		//}

		ScreenTask* screenTask = new ScreenTask;
		screenTask->setParam(true, strBuffer);
		m_screenThread->PostTask(std::shared_ptr<ScreenTask>(screenTask));

		//if ((int32_t)messageMap[LOG_IS_WRITE_LOG] == (int32_t)true)
		//{
		//	LogManager::instance().print((LogManager::LogLevel)(int32_t)messageMap[LOG_LEVEL],
		//		messageMap[LOG_FILE_NAME].toString(),
		//		messageMap[LOG_FUN_NAME].toString(),
		//		messageMap[LOG_PROCESS_NAME].toString(),
		//		messageMap[LOG_INT_DATE_TIME].toString(),
		//		(int32_t)(messageMap[LOG_THREAD_ID]),
		//		"NET %s %s",
		//		messageMap[LOG_LOGIN_NAME].toString().c_str(),
		//		messageMap[LOG_BUFFER].toString().c_str());
		//}

		LogTask* logTask = new LogTask;
		logTask->setParam(true, strBuffer);
		m_logThread->PostTask(std::shared_ptr<LogTask>(logTask));

		break;
	}
	default:
		break;
	}
}

void LogReceive::sendToNet(char* buffer, int32_t length, int32_t sendPid, CorrespondParam::ProtocolId protocolId)
{
	std::string processName = getSenderName(sendPid);
	std::string strBuffer(buffer, length);

	//std::map<std::string, Variant> messageMap;
	//ProtoMessage message;
	//message.from(strBuffer);
	//message.getMap(messageMap);
	//if ((int32_t)messageMap[LOG_IS_SEND_SCREEN] == (int32_t)true)
	//{
	//	//if (m_isNet)
	//	//{
	//	//	RCSend("NET %s %s", messageMap[LOG_LOGIN_NAME].toString().c_str(), messageMap[LOG_BUFFER].toString().c_str());
	//	//	return;
	//	//}
	//	RCSend("%s", messageMap[LOG_BUFFER].toString().c_str());
	//}

	//m_message.clear();
	//m_message.from(strBuffer);
	//std::map<std::string, Variant> messageMap;
	//m_message.getMap(messageMap);
	//if ((int32_t)messageMap[LOG_IS_SEND_SCREEN] == (int32_t)true)
	//{
	//	RCSend("%s", messageMap[LOG_BUFFER].toString().c_str());
	//}

	ScreenTask* screenTask = new ScreenTask;
	screenTask->setParam(false, strBuffer);
	m_screenThread->PostTask(std::shared_ptr<ScreenTask>(screenTask));
	
	//if ((int32_t)messageMap[LOG_IS_WRITE_LOG] == (int32_t)true)
	//{
	//	LogManager::instance().print((LogManager::LogLevel)(int32_t)messageMap[LOG_LEVEL],
	//		messageMap[LOG_FILE_NAME].toString(),
	//		messageMap[LOG_FUN_NAME].toString(),
	//		processName,
	//		messageMap[LOG_INT_DATE_TIME].toString(),
	//		(int32_t)(messageMap[LOG_THREAD_ID]),
	//		"%s",
	//		messageMap[LOG_BUFFER].toString().c_str());
	//}

	LogTask* logTask = new LogTask;
	logTask->setParam(false, strBuffer, processName);
	m_logThread->PostTask(std::shared_ptr<LogTask>(logTask));
	
	//if ((int32_t)messageMap[LOG_IS_SEND_NET] == (int32_t)true)
	//{
	//	//printf("send to netclient\n");
	//	m_message[LOG_PROCESS_NAME] = processName;
	//	std::string strMessage = m_message.toString();
	//	NetSender::instance().send(strMessage.c_str(), strMessage.length(), protocolId);
	//}
	
	NetTask* netTask = new NetTask;
	netTask->setParam(strBuffer, processName, protocolId);
	m_netThread->PostTask(std::shared_ptr<NetTask>(netTask));
}

std::string LogReceive::getSenderName(int32_t sendPid)
{
	auto it = m_sendMap.find(sendPid);
	if (it != m_sendMap.end())
	{
		return it->second;
	}
	std::string processName = CSystem::processName(sendPid);
	m_sendMap[sendPid] = processName;
	return processName;
}