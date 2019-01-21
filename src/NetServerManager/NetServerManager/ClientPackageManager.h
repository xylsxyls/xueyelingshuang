#pragma once
#include <map>
#include <stdint.h>
#include "ClientPackage.h"
#include "ProtoMessage/ProtoMessageAPI.h"
#include <mutex>

class ClientPackageManager
{
protected:
	ClientPackageManager();

public:
	static ClientPackageManager& instance();

public:
	void addClientPackage(ProtoMessage& message, uv_tcp_t* sender);

	uv_tcp_t* getClientPtr(int32_t clientId);

	int32_t getClientId(uv_tcp_t* clientPtr);

	int32_t getServerPid(uv_tcp_t* clientPtr);

private:
	std::map<int32_t, ClientPackage> m_serverPackageMap;
	std::map<uv_tcp_t*, int32_t> m_clientPtrMap;
	static int32_t s_clientId;
	std::mutex m_mutex;
};