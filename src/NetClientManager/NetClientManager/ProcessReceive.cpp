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
void ProcessReceive::receive(int32_t sendPid, char* buffer, int32_t length, CorrespondParam::ProtocolId protocalId)
{
	std::string strBuffer(buffer, length);
	switch (protocalId)
	{
	case CorrespondParam::CLIENT_INIT:
	{
		printf("CLIENT_INIT process, length = %d\n", length);
		ProtoMessage message;
		message.from(strBuffer);
		//客户端进程本身的初始化信息存入
		message[CLIENT_NAME] = CSystem::GetName(CSystem::processName(sendPid), 3);
		message[LOGIN_NAME] = m_computerName;
		//打包
		message.toString(strBuffer);
	}
	break;
	default:
		break;
	}
	
	//压缩
	std::string compressMessage;
	Compress::zlibCompress(compressMessage, strBuffer, 9);
	//发送
	m_client->send(compressMessage.c_str(), compressMessage.length(), protocalId);
}

void ProcessReceive::setClient(Client* client)
{
	m_client = client;
}