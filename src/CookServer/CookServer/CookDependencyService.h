#pragma once
#include <stdint.h>
#include <string>

/** CookServer启动依赖检查器，负责在服务初始化前拉起本机MySQL和Redis
*/
class CookDependencyService
{
public:
	/** 确保当前存储后端需要的外部依赖已经可连接
	@return 返回true表示依赖已可用或无需处理
	*/
	static bool ensureStartupDependencies();

private:
	struct ServiceConfig
	{
		std::string m_name;
		std::string m_host;
		int32_t m_port;
		bool m_autoStart;
		std::string m_home;
		std::string m_processName;
		std::string m_configFile;
		std::string m_startCommand;
		std::string m_workingDirectory;
	};

	static bool shouldCheckStorageDependencies();

	static bool ensureServiceAvailable(const ServiceConfig& config);

	static bool waitServiceReady(const ServiceConfig& config, bool shouldSettleAfterConnect);

	static bool startProcess(const ServiceConfig& config);

	static std::string applyPlaceholders(const std::string& text, const ServiceConfig& config);

	static std::string replaceAll(const std::string& text, const std::string& from, const std::string& to);

	static bool isProcessRunning(const std::string& processName);

	static bool isTcpConnectable(const std::string& host, int32_t port, int32_t timeoutMilliseconds);
};
