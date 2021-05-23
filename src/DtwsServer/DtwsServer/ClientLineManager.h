#pragma once
#include <map>
#include <mutex>

class ClientLineManager
{
protected:
	ClientLineManager();

public:
	static ClientLineManager& instance();

	void addClient(const std::string& loginName, int32_t connectId, int32_t clientPid);

private:
	std::mutex m_clientMutex;
	std::map<std::string, std::pair<int32_t, int32_t>> m_client;
};