#pragma once
#include <map>
#include <mutex>
#include <vector>

class ClientLineManager
{
protected:
	ClientLineManager();

public:
	static ClientLineManager& instance();

	void addClient(const std::string& loginName, int32_t connectId, int32_t clientPid);

	std::vector<std::pair<int32_t, int32_t>> allClient();

private:
	std::mutex m_clientMutex;
	std::map<std::string, std::pair<int32_t, int32_t>> m_client;
};