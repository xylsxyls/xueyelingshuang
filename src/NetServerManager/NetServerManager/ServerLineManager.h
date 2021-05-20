#pragma once
#include <stdint.h>
#include <map>
#include <string>
#include <mutex>

class ServerLineManager
{
protected:
	ServerLineManager();

public:
	static ServerLineManager& instance();

public:
	void addServerId(int32_t serverId, const std::string& serverName);

	void addServerPid(int32_t serverPid, const std::string& serverName);

	//’“≤ªµΩ∑µªÿ-1
	int32_t findServerId(const std::string& serverName);

	int32_t findServerId(int32_t serverPid);

	std::string findServerNameFromPid(int32_t serverPid);

	std::string findServerNameFromId(int32_t serverId);

	int32_t findServerPid(const std::string& serverName);

	int32_t findServerPid(int32_t serverId);

private:
	std::mutex m_serverIdMutex;
	std::map<int32_t, std::string> m_serverIdMap;
	std::mutex m_serverPidMutex;
	std::map<std::string, int32_t> m_serverPidMap;
};