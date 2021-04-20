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

//�ӱ��ؽ��̽���
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
		//�ͻ��˽��̱���ĳ�ʼ����Ϣ����
		message[CLIENT_NAME] = CSystem::GetName(CSystem::processName(sendPid), 3);
		message[LOGIN_NAME] = m_computerName;
		//���
		message.toString(strBuffer);
	}
	break;
	default:
		break;
	}
	
	//ѹ��
	std::string compressMessage;
	Compress::zlibCompress(compressMessage, strBuffer, 9);
	//����
	m_client->send(compressMessage.c_str(), compressMessage.length(), protocalId);
}

void ProcessReceive::setClient(Client* client)
{
	m_client = client;
}