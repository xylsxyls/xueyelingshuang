#include "CookHttpServer.h"
#include "Config.h"
#include "CookHttpApiHandler.h"
#include "CookServerHelper.h"
#include "CStringManager/CStringManagerAPI.h"
#include "LogManager/LogManagerAPI.h"
#include <iostream>

CookHttpServer::CookHttpServer(CookApiService* apiService) :
m_apiService(apiService),
m_handler(new CookHttpApiHandler(apiService))
{

}

CookHttpServer::~CookHttpServer()
{
	stop();
}

bool CookHttpServer::start()
{
	if (m_apiService == nullptr)
	{
		LOGERROR("CookHttpServer start failed, api service is null.");
		std::cerr << "CookServer api service is null." << std::endl;
		return false;
	}
	if (m_handler.get() == nullptr)
	{
		LOGERROR("CookHttpServer start failed, api handler is null.");
		std::cerr << "CookServer api handler is null." << std::endl;
		return false;
	}
	if (m_httpService.isRunning())
	{
		LOGINFO("CookHttpServer start ignored, service is already running.");
		return true;
	}

	std::string maxRequestBytesText = CStringManager::toStringInt64(g_config.m_maxRequestBytes);
	LOGINFO("CookHttpServer start begin listenHost=%s port=%d workerThreads=%d maxRequestBytes=%s cors=%d",
	        g_config.m_httpListenHost.c_str(),
	        static_cast<int32_t>(g_config.m_httpPort),
	        g_config.m_httpWorkerThreads,
	        maxRequestBytesText.c_str(),
	        g_config.m_enableCors ? 1 : 0);
	m_httpService.clearHandlers();
	for (size_t i = 0; i < CookServerHelper::routeCount(); ++i)
	{
		const char* route = CookServerHelper::routeAt(i);
		LOGDEBUG("CookHttpServer add route begin route=%s", route);
		if (!m_httpService.addHandler(route, m_handler))
		{
			LOGERROR("CookHttpServer add route failed route=%s", route);
			std::cerr << "CookServer add route failed: " << route << std::endl;
			return false;
		}
		LOGDEBUG("CookHttpServer add route success route=%s", route);
	}

	HttpServiceOptions options;
	options.m_listenHost = g_config.m_httpListenHost;
	options.m_port = g_config.m_httpPort;
	options.m_workerThreads = g_config.m_httpWorkerThreads;
	options.m_maxRequestBodyBytes = g_config.m_maxRequestBytes;
	options.m_enableCors = g_config.m_enableCors;
	options.m_corsAllowOrigin = g_config.m_corsAllowOrigin;

	if (!m_httpService.start(options))
	{
		LOGERROR("CookHttpServer start failed listenHost=%s port=%d error=%s",
		         g_config.m_httpListenHost.c_str(),
		         static_cast<int32_t>(g_config.m_httpPort),
		         m_httpService.lastError().c_str());
		std::cerr << "CookServer start failed on port " << g_config.m_httpPort << ": " << m_httpService.lastError() << std::endl;
		return false;
	}

	LOGINFO("CookHttpServer start success listenHost=%s port=%d publicIp=%s routeCount=%d",
	        g_config.m_httpListenHost.c_str(),
	        static_cast<int32_t>(g_config.m_httpPort),
	        g_config.m_serverPublicIp.c_str(),
	        static_cast<int32_t>(CookServerHelper::routeCount()));
	std::cout << "CookServer listening on " << g_config.m_httpListenHost << ":" << g_config.m_httpPort << std::endl;
	std::cout << "Public API host: http://" << g_config.m_serverPublicIp << ":" << g_config.m_httpPort << std::endl;
	return true;
}

void CookHttpServer::stop()
{
	bool wasRunning = m_httpService.isRunning();
	if (wasRunning)
	{
		LOGINFO("CookHttpServer stop begin.");
	}
	m_httpService.stop();
	if (wasRunning)
	{
		LOGINFO("CookHttpServer stop complete.");
	}
}