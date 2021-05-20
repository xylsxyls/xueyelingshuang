#pragma once
#include <stdint.h>
#include <map>
#include <mutex>

class ClientLineManager
{
protected:
	ClientLineManager();

public:
	static ClientLineManager& instance();

public:
	void addClient(int32_t connectId, int32_t clientPid, const std::string& clientName);

private:
	std::mutex m_clientMutex;
	std::map<int32_t, std::map<int32_t, std::string>> m_clientMap;
};