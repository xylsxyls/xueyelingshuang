#include "CookServerTestConfig.h"
#include "CSystem/CSystemAPI.h"
#include "CStringManager/CStringManagerAPI.h"
#include <ctime>

CookServerTestConfig& CookServerTestConfig::instance()
{
	static CookServerTestConfig s_config;
	return s_config;
}

CookServerTestConfig::CookServerTestConfig() :
m_port(0),
m_timeoutMs(0),
m_maxResponseBytes(0),
m_stressThreadCount(0),
m_stressLoopCount(0),
m_enableStress(false),
m_isInit(false)
{

}

CookServerTestConfig::~CookServerTestConfig()
{

}

bool CookServerTestConfig::init(int32_t argc, char* argv[])
{
	m_host = "127.0.0.1";
	m_port = 5207;
	m_timeoutMs = 5000;
	m_maxResponseBytes = 5 * 1024 * 1024;
	m_stressThreadCount = 16;
	m_stressLoopCount = 50;
	m_enableStress = true;
	m_reportFileName = "CookServerTestReport.txt";
	m_featureDocumentFileName = "CookServer功能说明文档.txt";
	m_deployDocumentFileName = "CookServer部署文档.txt";
	m_userIdPrefix = "cook_test_" +
		CStringManager::toStringInt32(static_cast<int32_t>(CSystem::currentProcessPid())) +
		"_" +
		CStringManager::toStringInt64(static_cast<int64_t>(std::time(nullptr)));

	for (int32_t i = 1; i < argc; ++i)
	{
		if (argv[i] != nullptr)
		{
			parseArgument(argv[i]);
		}
	}

	m_isInit = true;
	return true;
}

void CookServerTestConfig::uninit()
{
	m_host.clear();
	m_userIdPrefix.clear();
	m_reportFileName.clear();
	m_featureDocumentFileName.clear();
	m_deployDocumentFileName.clear();
	m_port = 0;
	m_timeoutMs = 0;
	m_maxResponseBytes = 0;
	m_stressThreadCount = 0;
	m_stressLoopCount = 0;
	m_enableStress = false;
	m_isInit = false;
}

std::string CookServerTestConfig::reportPath() const
{
	return CSystem::GetCurrentExePath() + m_reportFileName;
}

std::string CookServerTestConfig::cookServerFeatureDocumentPath() const
{
	return CSystem::GetCurrentExePath() + m_featureDocumentFileName;
}

std::string CookServerTestConfig::cookServerDeployDocumentPath() const
{
	return CSystem::GetCurrentExePath() + m_deployDocumentFileName;
}

bool CookServerTestConfig::parseArgument(const std::string& argument)
{
	if (argument.find("--host=") == 0)
	{
		m_host = argument.substr(7);
		return true;
	}
	if (argument.find("--port=") == 0)
	{
		m_port = parseInt32(argument.substr(7), m_port, 1, 65535);
		return true;
	}
	if (argument.find("--timeoutMs=") == 0)
	{
		m_timeoutMs = parseInt32(argument.substr(12), m_timeoutMs, 100, 60000);
		return true;
	}
	if (argument.find("--maxResponseBytes=") == 0)
	{
		m_maxResponseBytes = parseInt32(argument.substr(19), m_maxResponseBytes, 1024, 50 * 1024 * 1024);
		return true;
	}
	if (argument.find("--stressThreads=") == 0)
	{
		m_stressThreadCount = parseInt32(argument.substr(16), m_stressThreadCount, 1, 256);
		return true;
	}
	if (argument.find("--stressLoops=") == 0)
	{
		m_stressLoopCount = parseInt32(argument.substr(14), m_stressLoopCount, 1, 100000);
		return true;
	}
	if (argument.find("--userPrefix=") == 0)
	{
		m_userIdPrefix = argument.substr(13);
		return true;
	}
	if (argument == "--noStress")
	{
		m_enableStress = false;
		return true;
	}
	return false;
}

int32_t CookServerTestConfig::parseInt32(const std::string& text, int32_t defaultValue, int32_t minValue, int32_t maxValue) const
{
	int64_t value = CStringManager::atoi64(text.c_str());
	if (value < minValue || value > maxValue)
	{
		return defaultValue;
	}
	return static_cast<int32_t>(value);
}