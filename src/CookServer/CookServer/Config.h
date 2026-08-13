#pragma once
#include <stdint.h>
#include <string>

#define g_config Config::instance()

/** CookServer运行配置单例，集中管理服务启动、账号状态和本地演示数据相关配置
*/
class Config
{
public:
	/** 获取全局配置单例
	@return 返回Config单例引用
	*/
	static Config& instance();

	/** 初始化配置
	@return 返回true表示配置初始化成功
	*/
	bool init();

	/** 反初始化配置，清空当前配置值，使实例可以再次init
	*/
	void uninit();

private:
	/** 构造函数，只做基础类型成员的安全初始化
	*/
	Config();

	/** 析构函数，单例生命周期结束时不做业务清理
	*/
	~Config();

	/** 禁止拷贝构造
	@param [in] other 另一个配置对象
	*/
	Config(const Config& other);

	/** 禁止赋值
	@param [in] other 另一个配置对象
	@return 返回当前对象引用
	*/
	Config& operator=(const Config& other);

public:
	// 对外展示的服务器IP，客户端默认请求地址应由前端配置层读取该值对应的部署环境
	std::string m_serverPublicIp;
	// HTTP服务监听地址，0.0.0.0表示监听本机所有网卡
	std::string m_httpListenHost;
	// HTTP服务端口，微信小程序、Android和iOS客户端共用同一套HTTP API
	uint16_t m_httpPort;
	// 当前API版本，用于健康检查和客户端兼容性判断
	std::string m_apiVersion;
	// TCP监听队列长度，保留给后续HttpService参数扩展使用
	int32_t m_httpBacklog;
	// HTTP工作线程数量，影响同时处理请求的能力
	int32_t m_httpWorkerThreads;
	// 单个HTTP请求体最大字节数，防止异常请求占用过多内存
	int64_t m_maxRequestBytes;
	// 是否自动补充跨域响应头
	bool m_enableCors;
	// 跨域允许的来源，m_enableCors为true时生效
	std::string m_corsAllowOrigin;
	// 简易账号状态文件名，实际读写路径固定为exe同级目录
	std::string m_accountStateFileName;
	// 菜谱资源目录名，实际读取路径固定为exe同级目录下的相对目录
	std::string m_recipeResourceDirectoryName;
	// 新账号默认赠送金币数
	int32_t m_accountDefaultCoins;
	// 单次充值允许的最大金币数
	int32_t m_accountMaxRechargeCoins;
	// 主线程保活循环休眠时间，单位毫秒
	int32_t m_mainLoopSleepMilliseconds;
	// 可被展示为自由时间的最短连续时长，单位秒
	int32_t m_freeBlockMinimumSeconds;
	// 日志目录，空字符串表示使用exe所在目录
	std::string m_logPath;
	// 单个日志实体文件最大字节数
	int64_t m_logMaxFileBytes;
	// 单个进程最多保留的日志实体文件数量，.0日志文件不会被滚删
	int32_t m_logMaxFileCount;
	// 每写入多少条日志检查一次文件大小
	int32_t m_logCheckFileSizeInterval;
	// 是否同步输出日志到控制台
	bool m_logOutputConsole;
	// 启动时是否归档上一次进程留下的旧日志
	bool m_logArchiveOldLog;
	// 当前配置是否已经完成init
	bool m_isInit;
};