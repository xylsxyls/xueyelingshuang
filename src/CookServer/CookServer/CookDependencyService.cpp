#include "CookDependencyService.h"
#include "Config.h"
#include "CookHelper.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"

#include <vector>

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <Windows.h>
#endif

bool CookDependencyService::ensureStartupDependencies()
{
	if (!shouldCheckStorageDependencies())
	{
		LOGINFO("CookDependencyService skip dependency startup backend=%s", g_config.m_storageBackend.c_str());
		return true;
	}
	if (!g_config.m_dependencyAutoStart)
	{
		LOGINFO("CookDependencyService dependency auto start disabled.");
		return true;
	}

	ServiceConfig mysqlConfig;
	mysqlConfig.m_name = "MySQL";
	mysqlConfig.m_host = g_config.m_mysqlHost;
	mysqlConfig.m_port = g_config.m_mysqlPort;
	mysqlConfig.m_autoStart = g_config.m_mysqlAutoStart;
	mysqlConfig.m_home = g_config.m_mysqlHome;
	mysqlConfig.m_processName = g_config.m_mysqlProcessName;
	mysqlConfig.m_startCommand = g_config.m_mysqlStartCommand;
	mysqlConfig.m_workingDirectory = g_config.m_mysqlWorkingDirectory;

	ServiceConfig redisConfig;
	redisConfig.m_name = "Redis";
	redisConfig.m_host = g_config.m_redisHost;
	redisConfig.m_port = g_config.m_redisPort;
	redisConfig.m_autoStart = g_config.m_redisAutoStart;
	redisConfig.m_home = g_config.m_redisHome;
	redisConfig.m_processName = g_config.m_redisProcessName;
	redisConfig.m_configFile = g_config.m_redisConfigFile;
	redisConfig.m_startCommand = g_config.m_redisStartCommand;
	redisConfig.m_workingDirectory = g_config.m_redisWorkingDirectory;

	return ensureServiceAvailable(mysqlConfig) && ensureServiceAvailable(redisConfig);
}

bool CookDependencyService::shouldCheckStorageDependencies()
{
	std::string backend = CookHelper::lowerAscii(g_config.m_storageBackend);
	return backend == "redis_mysql" || backend == "mysql_redis" || backend == "redis+mysql";
}

bool CookDependencyService::ensureServiceAvailable(const ServiceConfig& config)
{
	if (isProcessRunning(config.m_processName))
	{
		LOGINFO("CookDependencyService service process already running, wait connectable service=%s process=%s host=%s port=%d",
		        config.m_name.c_str(),
		        config.m_processName.c_str(),
		        config.m_host.c_str(),
		        config.m_port);
		return waitServiceReady(config, false);
	}

	if (!config.m_autoStart)
	{
		LOGWARNING("CookDependencyService auto start disabled for service=%s host=%s port=%d",
		           config.m_name.c_str(),
		           config.m_host.c_str(),
		           config.m_port);
		return true;
	}

	if (!startProcess(config))
	{
		return false;
	}
	return waitServiceReady(config, true);
}

bool CookDependencyService::waitServiceReady(const ServiceConfig& config, bool shouldSettleAfterConnect)
{
	int32_t elapsedMilliseconds = 0;
	bool sawProcess = false;
	while (elapsedMilliseconds <= g_config.m_dependencyWaitTimeoutMilliseconds)
	{
		if (isProcessRunning(config.m_processName))
		{
			sawProcess = true;
			if (isTcpConnectable(config.m_host, config.m_port, g_config.m_dependencyPollIntervalMilliseconds))
			{
				if (shouldSettleAfterConnect && g_config.m_dependencyStartupSettleMilliseconds > 0)
				{
					CSystem::Sleep(g_config.m_dependencyStartupSettleMilliseconds);
				}
				if (!isProcessRunning(config.m_processName))
				{
					LOGERROR("CookDependencyService service exited after port became connectable service=%s process=%s elapsedMs=%d",
					         config.m_name.c_str(),
					         config.m_processName.c_str(),
					         elapsedMilliseconds);
					return false;
				}
				LOGINFO("CookDependencyService service ready service=%s process=%s host=%s port=%d elapsedMs=%d settleMs=%d",
				        config.m_name.c_str(),
				        config.m_processName.c_str(),
				        config.m_host.c_str(),
				        config.m_port,
				        elapsedMilliseconds,
				        shouldSettleAfterConnect ? g_config.m_dependencyStartupSettleMilliseconds : 0);
				return true;
			}
		}
		else if (sawProcess)
		{
			LOGERROR("CookDependencyService service process exited before ready service=%s process=%s host=%s port=%d elapsedMs=%d",
			         config.m_name.c_str(),
			         config.m_processName.c_str(),
			         config.m_host.c_str(),
			         config.m_port,
			         elapsedMilliseconds);
			return false;
		}
		CSystem::Sleep(g_config.m_dependencyPollIntervalMilliseconds);
		elapsedMilliseconds += g_config.m_dependencyPollIntervalMilliseconds;
	}
	LOGERROR("CookDependencyService wait service ready timeout service=%s process=%s host=%s port=%d timeoutMs=%d",
	         config.m_name.c_str(),
	         config.m_processName.c_str(),
	         config.m_host.c_str(),
	         config.m_port,
	         g_config.m_dependencyWaitTimeoutMilliseconds);
	return false;
}

bool CookDependencyService::startProcess(const ServiceConfig& config)
{
#ifdef _WIN32
	std::string command = applyPlaceholders(config.m_startCommand, config);
	std::string workingDirectory = applyPlaceholders(config.m_workingDirectory, config);
	if (command.empty())
	{
		LOGERROR("CookDependencyService empty start command service=%s", config.m_name.c_str());
		return false;
	}
	if (!workingDirectory.empty() && !CSystem::DirOrFileExist(workingDirectory))
	{
		LOGERROR("CookDependencyService working directory missing service=%s directory=%s",
		         config.m_name.c_str(),
		         workingDirectory.c_str());
		return false;
	}

	std::vector<char> commandBuffer(command.begin(), command.end());
	commandBuffer.push_back('\0');
	STARTUPINFOA startupInfo;
	PROCESS_INFORMATION processInfo;
	ZeroMemory(&startupInfo, sizeof(startupInfo));
	ZeroMemory(&processInfo, sizeof(processInfo));
	startupInfo.cb = sizeof(startupInfo);
	const char* currentDirectory = workingDirectory.empty() ? nullptr : workingDirectory.c_str();
	BOOL created = ::CreateProcessA(nullptr,
	                                &commandBuffer[0],
	                                nullptr,
	                                nullptr,
	                                FALSE,
	                                CREATE_NEW_CONSOLE,
	                                nullptr,
	                                currentDirectory,
	                                &startupInfo,
	                                &processInfo);
	if (!created)
	{
		LOGERROR("CookDependencyService start process failed service=%s command=%s error=%lu",
		         config.m_name.c_str(),
		         command.c_str(),
		         static_cast<unsigned long>(::GetLastError()));
		return false;
	}
	LOGINFO("CookDependencyService start process success service=%s pid=%lu command=%s",
	        config.m_name.c_str(),
	        static_cast<unsigned long>(processInfo.dwProcessId),
	        command.c_str());
	::CloseHandle(processInfo.hThread);
	::CloseHandle(processInfo.hProcess);
	return true;
#else
	LOGWARNING("CookDependencyService start process unsupported on this platform service=%s", config.m_name.c_str());
	(void)config;
	return false;
#endif
}

std::string CookDependencyService::applyPlaceholders(const std::string& text, const ServiceConfig& config)
{
	std::string result = text;
	result = replaceAll(result, "{home}", config.m_home);
	result = replaceAll(result, "{host}", config.m_host);
	result = replaceAll(result, "{port}", CStringManager::toStringInt32(config.m_port));
	result = replaceAll(result, "{config}", replaceAll(config.m_configFile, "{home}", config.m_home));
	return result;
}

std::string CookDependencyService::replaceAll(const std::string& text, const std::string& from, const std::string& to)
{
	if (from.empty())
	{
		return text;
	}
	std::string result = text;
	size_t pos = 0;
	while ((pos = result.find(from, pos)) != std::string::npos)
	{
		result.replace(pos, from.size(), to);
		pos += to.size();
	}
	return result;
}

bool CookDependencyService::isProcessRunning(const std::string& processName)
{
	return !processName.empty() && CSystem::processFirstPid(processName) != 0;
}

bool CookDependencyService::isTcpConnectable(const std::string& host, int32_t port, int32_t timeoutMilliseconds)
{
#ifdef _WIN32
	if (host.empty() || port <= 0 || port > 65535)
	{
		return false;
	}
	WSADATA wsaData;
	if (::WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		return false;
	}

	addrinfo hints;
	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;

	std::string portText = CStringManager::toStringInt32(port);
	addrinfo* addressInfo = nullptr;
	if (::getaddrinfo(host.c_str(), portText.c_str(), &hints, &addressInfo) != 0)
	{
		::WSACleanup();
		return false;
	}

	bool connectable = false;
	for (addrinfo* item = addressInfo; item != nullptr && !connectable; item = item->ai_next)
	{
		SOCKET socketHandle = ::socket(item->ai_family, item->ai_socktype, item->ai_protocol);
		if (socketHandle == INVALID_SOCKET)
		{
			continue;
		}
		u_long nonBlocking = 1;
		if (::ioctlsocket(socketHandle, FIONBIO, &nonBlocking) != 0)
		{
			::closesocket(socketHandle);
			continue;
		}
		int32_t ret = ::connect(socketHandle, item->ai_addr, static_cast<int32_t>(item->ai_addrlen));
		if (ret == 0)
		{
			connectable = true;
			::closesocket(socketHandle);
			break;
		}
		int32_t errorCode = ::WSAGetLastError();
		if (errorCode == WSAEWOULDBLOCK || errorCode == WSAEINPROGRESS || errorCode == WSAEINVAL)
		{
			fd_set writeSet;
			FD_ZERO(&writeSet);
			FD_SET(socketHandle, &writeSet);
			timeval timeout;
			timeout.tv_sec = timeoutMilliseconds / 1000;
			timeout.tv_usec = (timeoutMilliseconds % 1000) * 1000;
			ret = ::select(0, nullptr, &writeSet, nullptr, &timeout);
			if (ret > 0 && FD_ISSET(socketHandle, &writeSet))
			{
				int32_t socketError = 0;
				int32_t socketErrorSize = sizeof(socketError);
				if (::getsockopt(socketHandle, SOL_SOCKET, SO_ERROR, reinterpret_cast<char*>(&socketError), &socketErrorSize) == 0 && socketError == 0)
				{
					connectable = true;
				}
			}
		}
		::closesocket(socketHandle);
	}
	::freeaddrinfo(addressInfo);
	::WSACleanup();
	return connectable;
#else
	(void)host;
	(void)port;
	(void)timeoutMilliseconds;
	return true;
#endif
}
