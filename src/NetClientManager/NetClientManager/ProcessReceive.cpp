#include "ProcessReceive.h"
#include "Client.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "Compress/CompressAPI.h"

ProcessReceive::ProcessReceive():
m_client(nullptr)
{
	m_computerName = CSystem::getComputerName();
}

//从本地进程接收
void ProcessReceive::receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type)
{
	std::string strBuffer(buffer, length);
	switch (type)
	{
	case MessageType::CLIENT_INIT:
	{
		printf("CLIENT_INIT process, length = %d\n", length);
	}
	break;
	default:
		break;
	}
	
	//压缩
	std::string compressMessage;
	Compress::zlibCompress(compressMessage, strBuffer, 9);
	//发送
	m_client->send(compressMessage.c_str(), compressMessage.length(), type);
}

void ProcessReceive::setClient(Client* client)
{
	m_client = client;
}