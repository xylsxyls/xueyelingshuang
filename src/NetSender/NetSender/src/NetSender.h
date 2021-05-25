#pragma once
#include "NetSenderMacro.h"
#include <stdint.h>
#include "CorrespondParam/CorrespondParamAPI.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include "ProcessWork/ProcessWorkAPI.h"
#include "ClientReceiveCallback.h"
#include "ServerReceiveCallback.h"

/** 网络发送接收类
*/
class NetSenderAPI NetSender
{
protected:
	NetSender();

public:
	static NetSender& instance();

public:
	//反初始化需要调用ProcessWork中的反初始化函数
	//初始化需要调用ProcessWork中的initReceive函数
	void initClientReceive(ClientReceiveCallback* callback);

	void initServerReceive(ServerReceiveCallback* callback);

	//windows端不带后缀，linux端全名
	void initClient(int32_t serverId, const std::string& serverName, const std::string& initInfo);

	void sendClientInitResponse(int32_t connectId, int32_t clientPid, const std::string& responseInfo);

	void initServer(const std::string& initInfo);

	void sendServer(int32_t serverId, const std::string& message);

	void sendServer(int32_t serverId, const char* buffer, int32_t length);

	void sendClient(int32_t connectId, int32_t clientPid, const std::string& message);

	void sendClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

	//如使用post需先初始化ProcessWork中的postThread
	void postServer(int32_t serverId, const std::string& message);

	void postServer(int32_t serverId, const char* buffer, int32_t length);

	void postClient(int32_t connectId, int32_t clientPid, const std::string& message);

	void postClient(int32_t connectId, int32_t clientPid, const char* buffer, int32_t length);

	//服务端管理进程主动发送消息，如果clientPid为0表示发给客户端管理进程，该函数为消息组装
	std::string netServerMessage(int32_t clientPid, const std::string& message);

	std::string netServerMessage(int32_t clientPid, const char* buffer, int32_t length);

	//客户端管理进程主动发送消息，如果clientPid为0表示发给服务端管理进程，该函数为消息组装
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