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
	void initClientReceive(ClientReceiveCallback* callback);

	void initServerReceive(ServerReceiveCallback* callback);

	void initClient(int32_t serverId, const std::string& serverName, const std::string& initInfo);

	void clientInitResponse(int32_t connectId, int32_t clientPid, const std::string& responseInfo);

	void initServer(const std::string& initInfo);

	void sendServer(int32_t serverId, const char* buffer, int32_t length);

	void sendClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

	//��ʹ��post���ȳ�ʼ��ProcessWork�е�postThread
	void postServer(int32_t serverId, const char* buffer, int32_t length);

	void postClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

private:
	int32_t m_currentProcessPid;
};