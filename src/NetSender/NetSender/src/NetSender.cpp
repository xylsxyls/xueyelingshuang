#include "NetSender.h"
#include "CSystem/CSystemAPI.h"

NetSender::NetSender():
m_currentProcessPid(0)
{
	m_currentProcessPid = (int32_t)CSystem::currentProcessPid();
}

NetSender& NetSender::instance()
{
	static NetSender s_netSender;
	return s_netSender;
}

void NetSender::sendServer(int32_t serverId, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 8);
	*(int32_t*)(&sendBuffer[0]) = serverId;
	*((int32_t*)(&sendBuffer[0]) + 1) = m_currentProcessPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 2), buffer, length);
	ProcessWork::instance().send("NetServerManager1.0",
		sendBuffer.c_str(),
		sendBuffer.size(),
		MessageType::MESSAGE);
}

void NetSender::sendClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 8);
	*(int32_t*)(&sendBuffer[0]) = connectId;
	*((int32_t*)(&sendBuffer[0]) + 1) = clientPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 2), buffer, length);
	ProcessWork::instance().send("NetServerManager1.0",
		sendBuffer.c_str(),
		sendBuffer.size(),
		MessageType::SERVER_MESSAGE);
}

void NetSender::postServer(int32_t serverId, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 8);
	*(int32_t*)(&sendBuffer[0]) = serverId;
	*((int32_t*)(&sendBuffer[0]) + 1) = m_currentProcessPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 2), buffer, length);
	ProcessWork::instance().post("NetServerManager1.0",
		sendBuffer.c_str(),
		sendBuffer.size(),
		MessageType::MESSAGE);
}

void NetSender::postClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 8);
	*(int32_t*)(&sendBuffer[0]) = connectId;
	*((int32_t*)(&sendBuffer[0]) + 1) = clientPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 2), buffer, length);
	ProcessWork::instance().post("NetServerManager1.0",
		sendBuffer.c_str(),
		sendBuffer.size(),
		MessageType::SERVER_MESSAGE);
}

void NetSender::initClientReceive(ClientReceiveCallback* callback)
{
	ProcessWork::instance().addProcessReceiveCallback(callback);
}

void NetSender::initClient(int32_t serverId, const std::string& serverName, const std::string& initInfo)
{
	//客户端将自己的进程号名字及要发的服务进程名和初始化附带信息发送给服务端
	ProtoMessage message;
	message[CLIENT_PID] = m_currentProcessPid;
	message[CLIENT_NAME] = CSystem::GetCurrentExeFullName();
	message[SERVER_ID] = serverId;
	message[SERVER_NAME] = serverName;
	message[CLIENT_INIT_INFO] = initInfo;
	std::string strMessage = message.toString();
	ProcessWork::instance().send("NetServerManager1.0",
		strMessage.c_str(),
		strMessage.length(),
		MessageType::CLIENT_INIT);
}

void NetSender::clientInitResponse(int32_t connectId, int32_t clientPid, const std::string& responseInfo)
{
	ProtoMessage message;
	message[CONNECT_ID] = connectId;
	message[CLIENT_PID] = clientPid;
	message[CLIENT_INIT_RESPONSE_INFO] = responseInfo;
	std::string strMessage = message.toString();
	ProcessWork::instance().send("NetServerManager1.0",
		strMessage.c_str(),
		strMessage.length(),
		MessageType::CLIENT_INIT_RESPONSE);
}

void NetSender::initServer(const std::string& initInfo)
{
	ProcessWork::instance().send("NetServerManager1.0",
		initInfo.c_str(),
		initInfo.length(),
		MessageType::SERVER_INIT);
}