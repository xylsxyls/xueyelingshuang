#pragma once
#include <string>

typedef struct uv_tcp_s uv_tcp_t;

class ClientPackage
{
public:
	std::string m_clientName;
	std::string m_serverName;
	int32_t m_serverPid;
	std::string m_loginName;
	uv_tcp_t* m_clientPtr;

	ClientPackage():
		m_clientPtr(nullptr),
		m_serverPid(0)
	{

	}

	ClientPackage(const std::string& clientName, const std::string& serverName, int32_t serverPid, const std::string& loginName, uv_tcp_t* clientPtr)
	{
		m_clientName = clientName;
		m_serverName = serverName;
		m_serverPid = serverPid;
		m_loginName = loginName;
		m_clientPtr = clientPtr;
	}

	ClientPackage(const ClientPackage& other)
	{
		m_clientName = other.m_clientName;
		m_serverName = other.m_serverName;
		m_serverPid = other.m_serverPid;
		m_loginName = other.m_loginName;
		m_clientPtr = other.m_clientPtr;
	}
};