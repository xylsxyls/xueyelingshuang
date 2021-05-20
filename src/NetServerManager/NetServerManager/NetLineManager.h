#pragma once
#include <stdint.h>
#include <map>
#include <mutex>

typedef struct uv_tcp_s uv_tcp_t;

class NetLineManager
{
protected:
	NetLineManager();

public:
	static NetLineManager& instance();

public:
	//添加一个client，返回ConnectId
	int32_t addConnect(uv_tcp_t* client);

	uv_tcp_t* findClient(int32_t connectId);

	int32_t findConnectId(uv_tcp_t* client);

protected:
	int32_t m_connectId;
	std::mutex m_connectIdMutex;
	std::map<int32_t, uv_tcp_t*> m_connectIdMap;
};