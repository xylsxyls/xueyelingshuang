#include "NetSender.h"
#include "CSystem/CSystemAPI.h"

#define NET_CLIENT_VERSION "1.1"
#define NET_SERVER_VERSION "1.1"

//发送类型由发送方决定
NetSender::NetSender():
m_currentProcessPid(0)
{
	m_currentProcessPid = (int32_t)CSystem::currentProcessPid();
	m_currentExeFullName = CSystem::GetCurrentExeFullName();
	m_netClientManagerName = std::string("NetClientManager") + NET_CLIENT_VERSION;
	m_netServerManagerName = std::string("NetServerManager") + NET_SERVER_VERSION;
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
	ProcessWork::instance().send(m_netClientManagerName,
		sendBuffer.c_str(),
		sendBuffer.size(),
		MessageType::MESSAGE);
}

void NetSender::sendServer(int32_t serverId, const std::string& message)
{
	sendServer(serverId, message.c_str(), message.size());
}

void NetSender::sendClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 8);
	*(int32_t*)(&sendBuffer[0]) = connectId;
	*((int32_t*)(&sendBuffer[0]) + 1) = clientPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 2), buffer, length);
	ProcessWork::instance().send(m_netServerManagerName,
		sendBuffer.c_str(),
		sendBuffer.size(),
		MessageType::SERVER_MESSAGE);
}

void NetSender::sendClient(int32_t connectId, int32_t clientPid, const std::string& message)
{
	sendClient(connectId, clientPid, message.c_str(), message.size());
}

void NetSender::postServer(int32_t serverId, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 8);
	*(int32_t*)(&sendBuffer[0]) = serverId;
	*((int32_t*)(&sendBuffer[0]) + 1) = m_currentProcessPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 2), buffer, length);
	ProcessWork::instance().post(m_netClientManagerName,
		sendBuffer.c_str(),
		sendBuffer.size(),
		MessageType::MESSAGE);
}

void NetSender::postServer(int32_t serverId, const std::string& message)
{
	postServer(serverId, message.c_str(), message.size());
}

void NetSender::postClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 8);
	*(int32_t*)(&sendBuffer[0]) = connectId;
	*((int32_t*)(&sendBuffer[0]) + 1) = clientPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 2), buffer, length);
	int32_t connectId2 = *(int32_t*)(&sendBuffer[0]);
	ProcessWork::instance().post(m_netServerManagerName,
		sendBuffer.c_str(),
		sendBuffer.size(),
		MessageType::SERVER_MESSAGE);
}

void NetSender::postClient(int32_t connectId, int32_t clientPid, const std::string& message)
{
	postClient(connectId, clientPid, message.c_str(), message.size());
}

std::string NetSender::netServerMessage(int32_t clientPid, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 4);
	*((int32_t*)(&sendBuffer[0])) = clientPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 1), buffer, length);
	return sendBuffer;
}

std::string NetSender::netServerMessage(int32_t clientPid, const std::string& message)
{
	return netServerMessage(clientPid, message.c_str(), message.size());
}

std::string NetSender::netClientMessage(int32_t serverId, const char* buffer, int32_t length)
{
	std::string sendBuffer;
	sendBuffer.resize(length + 8);
	*(int32_t*)(&sendBuffer[0]) = serverId;
	*((int32_t*)(&sendBuffer[0]) + 1) = m_currentProcessPid;
	::memcpy((char*)((int32_t*)(&sendBuffer[0]) + 2), buffer, length);
	return sendBuffer;
}

std::string NetSender::netClientMessage(int32_t serverId, const std::string& message)
{
	return netClientMessage(serverId, message.c_str(), message.size());
}

void NetSender::initClientReceive(ClientReceiveCallback* callback)
{
	ProcessWork::instance().addProcessReceiveCallback(callback);
}

void NetSender::initServerReceive(ServerReceiveCallback* callback)
{
	ProcessWork::instance().addProcessReceiveCallback(callback);
}

void NetSender::initClient(int32_t serverId, const std::string& serverName, const std::string& initInfo)
{
	//客户端将自己的进程号名字及要发的服务进程名和初始化附带信息发送给服务端
	ProtoMessage message;
	message[CLIENT_PID] = m_currentProcessPid;
	message[CLIENT_NAME] = m_currentExeFullName;
	message[SERVER_ID] = serverId;
#ifdef _MSC_VER
	message[SERVER_NAME] = serverName + ".exe";
#elif __unix__
	message[SERVER_NAME] = serverName;
#endif
	message[CLIENT_INIT_INFO] = initInfo;
	std::string strMessage = message.toString();
	ProcessWork::instance().send(m_netClientManagerName,
		strMessage.c_str(),
		strMessage.length(),
		MessageType::CLIENT_INIT);
}

void NetSender::sendClientInitResponse(int32_t connectId, int32_t clientPid, const std::string& responseInfo)
{
	ProtoMessage message;
	message[CONNECT_ID] = connectId;
	message[CLIENT_PID] = clientPid;
	message[CLIENT_INIT_RESPONSE_INFO] = responseInfo;
	std::string strMessage = message.toString();
	ProcessWork::instance().send(m_netServerManagerName,
		strMessage.c_str(),
		strMessage.length(),
		MessageType::CLIENT_INIT_RESPONSE);
}

void NetSender::initServer(const std::string& initInfo)
{
	ProcessWork::instance().send(m_netServerManagerName,
		initInfo.c_str(),
		initInfo.length(),
		MessageType::SERVER_INIT);
}