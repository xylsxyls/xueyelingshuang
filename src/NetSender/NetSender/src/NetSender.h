#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ClientReceiveCallback.h"
#include "ServerReceiveCallback.h"

/** ���緢�ͽ�����
*/
class NetSenderAPI NetSender
{
protected:
	NetSender();

public:
	static NetSender& instance();

public:
	//����ʼ����Ҫ����ProcessWork�еķ���ʼ������
	//��ʼ����Ҫ����ProcessWork�е�initReceive����
	void initClientReceive(ClientReceiveCallback* callback);

	void initServerReceive(ServerReceiveCallback* callback);

	//windows�˲�����׺��linux��ȫ��
	void initClient(int32_t serverId, const std::string& serverName, const std::string& initInfo);

	void sendClientInitResponse(int32_t connectId, int32_t clientPid, const std::string& responseInfo);

	void initServer(const std::string& initInfo);

	void sendServer(int32_t serverId, const std::string& message);

	void sendServer(int32_t serverId, const char* buffer, int32_t length);

	void sendClient(int32_t connectId, int32_t clientPid, const std::string& message);

	void sendClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

	//��ʹ��post���ȳ�ʼ��ProcessWork�е�postThread
	void postServer(int32_t serverId, const std::string& message);

	void postServer(int32_t serverId, const char* buffer, int32_t length);

	void postClient(int32_t connectId, int32_t clientPid, const std::string& message);

	void postClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

	//����˹����������������Ϣ�����clientPidΪ0��ʾ�����ͻ��˹�����̣��ú���Ϊ��Ϣ��װ
	std::string netServerMessage(int32_t clientPid, const std::string& message);

	std::string netServerMessage(int32_t clientPid, const char* buffer, int32_t length);

	//�ͻ��˹����������������Ϣ�����clientPidΪ0��ʾ��������˹�����̣��ú���Ϊ��Ϣ��װ
	std::string netClientMessage(int32_t serverId, const std::string& message);

	std::string netClientMessage(int32_t serverId, const char* buffer, int32_t length);

private:
	int32_t m_currentProcessPid;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
	std::string m_currentExeFullName;
	std::string m_netClientManagerName;
	std::string m_netServerManagerName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};