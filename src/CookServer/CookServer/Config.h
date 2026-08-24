#pragma once
#include <stdint.h>
#include <string>

#define g_config Config::instance()

class Cini;

/** CookServer运行配置单例，集中管理服务启动、账号状态和资源目录相关配置
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

	/** 写入内置默认配置
	*/
	void setDefaults();

	/** 从exe同级CookServer.ini读取可变配置，文件不存在时保持默认值
	*/
	void loadIniOverrides();

	/** 获取ini配置文件完整路径
	@return 返回exe同级配置文件路径
	*/
	std::string iniPath() const;

	/** 从ini读取字符串配置，空值或不存在时返回默认值
	@param [in] ini ini读取对象
	@param [in] section 节点名
	@param [in] key 键名
	@param [in] defaultValue 默认值
	@return 返回配置值
	*/
	std::string readIniString(const Cini& ini, const std::string& section, const std::string& key, const std::string& defaultValue) const;

	/** 从ini读取32位整数配置，非法值返回默认值
	@param [in] ini ini读取对象
	@param [in] section 节点名
	@param [in] key 键名
	@param [in] defaultValue 默认值
	@return 返回配置值
	*/
	int32_t readIniInt32(const Cini& ini, const std::string& section, const std::string& key, int32_t defaultValue) const;

	/** 从ini读取64位整数配置，非法值返回默认值
	@param [in] ini ini读取对象
	@param [in] section 节点名
	@param [in] key 键名
	@param [in] defaultValue 默认值
	@return 返回配置值
	*/
	int64_t readIniInt64(const Cini& ini, const std::string& section, const std::string& key, int64_t defaultValue) const;

	/** 从ini读取布尔配置，支持true/false和1/0
	@param [in] ini ini读取对象
	@param [in] section 节点名
	@param [in] key 键名
	@param [in] defaultValue 默认值
	@return 返回配置值
	*/
	bool readIniBool(const Cini& ini, const std::string& section, const std::string& key, bool defaultValue) const;

	/** 规整ini文本值，去掉首尾空白
	@param [in] text 原始文本
	@return 返回规整后的文本
	*/
	std::string normalizeIniText(const std::string& text) const;

public:
	// exe同级运行配置文件名
	std::string m_configFileName;
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
	// 业务状态存储后端，第一期正式支持redis_mysql，Redis热层加MySQL最终落库
	std::string m_storageBackend;
	// MySQL服务器地址
	std::string m_mysqlHost;
	// MySQL服务器端口
	int32_t m_mysqlPort;
	// MySQL登录账号
	std::string m_mysqlUser;
	// MySQL登录密码，禁止写入日志
	std::string m_mysqlPassword;
	// CookServer业务数据库名
	std::string m_mysqlDatabase;
	// MySQL连接池大小
	int32_t m_mysqlPoolSize;
	// 获取MySQL连接等待超时，单位毫秒
	int32_t m_mysqlAcquireTimeoutMilliseconds;
	// MySQL连接超时，单位秒
	int32_t m_mysqlConnectTimeoutSeconds;
	// MySQL读响应超时，单位秒
	int32_t m_mysqlReadTimeoutSeconds;
	// MySQL写请求超时，单位秒
	int32_t m_mysqlWriteTimeoutSeconds;
	// Redis服务器地址
	std::string m_redisHost;
	// Redis服务器端口
	int32_t m_redisPort;
	// Redis认证密码，空字符串表示无密码，禁止写入日志
	std::string m_redisPassword;
	// Redis DB下标
	int32_t m_redisDbIndex;
	// Redis连接池大小
	int32_t m_redisPoolSize;
	// Redis key前缀
	std::string m_redisKeyPrefix;
	// Redis连接超时，单位毫秒
	int32_t m_redisConnectTimeoutMilliseconds;
	// Redis命令超时，单位毫秒
	int32_t m_redisCommandTimeoutMilliseconds;
	// Redis是否启用TCP KeepAlive
	bool m_redisEnableKeepAlive;
	// 是否在CookServer启动时自动检查并拉起本机MySQL和Redis
	bool m_dependencyAutoStart;
	// 依赖服务启动后等待进程存在且端口可连接的最长时间，单位毫秒
	int32_t m_dependencyWaitTimeoutMilliseconds;
	// 等待依赖服务进程和端口时的轮询间隔，单位毫秒
	int32_t m_dependencyPollIntervalMilliseconds;
	// 依赖服务进程出现后继续等待其完成内部启动的时间，单位毫秒
	int32_t m_dependencyStartupSettleMilliseconds;
	// 是否允许自动启动MySQL
	bool m_mysqlAutoStart;
	// MySQL本地安装目录
	std::string m_mysqlHome;
	// MySQL本地进程名，用于避免重复启动
	std::string m_mysqlProcessName;
	// MySQL启动命令模板，支持{home}、{host}、{port}
	std::string m_mysqlStartCommand;
	// MySQL启动工作目录模板，支持{home}
	std::string m_mysqlWorkingDirectory;
	// 是否允许自动启动Redis
	bool m_redisAutoStart;
	// Redis本地安装目录
	std::string m_redisHome;
	// Redis本地进程名，用于避免重复启动
	std::string m_redisProcessName;
	// Redis配置文件路径模板，支持{home}
	std::string m_redisConfigFile;
	// Redis启动命令模板，支持{home}、{host}、{port}、{config}
	std::string m_redisStartCommand;
	// Redis启动工作目录模板，支持{home}
	std::string m_redisWorkingDirectory;
	// MySQL后台同步线程数量
	int32_t m_storageSyncThreadCount;
	// MySQL后台同步任务优先级
	int32_t m_storageSyncTaskPriority;
	// 菜谱资源目录名，实际读取路径固定为exe同级目录下的相对目录
	std::string m_recipeResourceDirectoryName;
	// 新账号默认赠送金币数
	int32_t m_accountDefaultCoins;
	// 新账号默认赠送元宝数
	int32_t m_accountDefaultYuanbao;
	// 单次充值允许的最大金币数
	int32_t m_accountMaxRechargeCoins;
	// 登录token有效期，单位秒
	int32_t m_authTokenExpireSeconds;
	// 每日签到奖励金币
	int32_t m_checkinRewardCoins;
	// 看完广告奖励金币
	int32_t m_adRewardCoins;
	// 广告奖励要求的最短观看时间，单位秒
	int32_t m_adRequiredSeconds;
	// 单次上传视频最大字节数
	int64_t m_maxVideoBytes;
	// 视频文件保存目录名，实际路径固定为exe同级目录下的相对目录
	std::string m_videoStorageDirectoryName;
	// 首页视频资源目录名，实际路径固定为exe同级目录下的相对目录
	std::string m_videoResourceDirectoryName;
	// 首页视频资源目录扫描间隔，单位秒，0表示每次请求都重新扫描
	int32_t m_videoResourceScanIntervalSeconds;
	// 首页默认一次推荐的视频数量
	int32_t m_videoFeedDefaultCount;
	// 用户看过视频记录的过期天数，过期后可再次推荐
	int32_t m_videoSeenExpireDays;
	// 视频Range响应的最大分段字节数，0表示按客户端Range原样返回
	int64_t m_videoStreamChunkBytes;
	// 视频第一帧图片下载接口路径
	std::string m_videoPosterApiPath;
	// 是否在扫描视频资源时生成第一帧图片
	bool m_videoPosterGenerateEnabled;
	// 第一帧图片生成命令模板，支持{videoFile}，命令需要把图片二进制写到stdout
	std::string m_videoPosterCommandTemplate;
	// 第一帧图片生成命令是否通过系统shell运行
	bool m_videoPosterCommandUseShell;
	// 首页向前预取的视频第一帧数量
	int32_t m_videoPosterPrefetchCount;
	// 视频第一帧内存缓存最大条数，0表示不缓存
	int32_t m_videoPosterMemoryCacheMaxItems;
	// 服务端语音合成提供方，windows_sapi表示使用Windows本机SAPI生成缓存音频
	std::string m_voiceTtsProvider;
	// 服务端语音缓存目录名，实际路径固定为exe同级目录下的相对目录
	std::string m_voiceCacheDirectoryName;
	// 语音音频下载接口路径
	std::string m_voiceAudioApiPath;
	// 单次语音合成允许的最大UTF-8字节数
	int32_t m_voiceMaxTextBytes;
	// 允许随/api/voice/tts JSON内联返回的最大音频字节数，0表示关闭内联音频
	int32_t m_voiceInlineAudioMaxBytes;
	// 服务端语音生成工作线程数
	int32_t m_voiceWorkerThreadCount;
	// 外部语音合成命令模板，用于command_wav或command_mp3提供方
	std::string m_voiceCommandTemplate;
	// 外部命令通用提供方使用的音频格式，支持wav和mp3
	std::string m_voiceCommandAudioFormat;
	// 外部语音命令是否通过系统shell运行
	bool m_voiceCommandUseShell;
	// 客户端开始做菜前必须先准备好的时间点组数量
	int32_t m_voiceInitialRequiredCount;
	// 客户端开始做菜前必须先准备好的时间点组数量，语义更明确的新字段
	int32_t m_voiceInitialPromptGroupCount;
	// 客户端语音并行请求数量
	int32_t m_voiceClientConcurrency;
	// 单次准备语音拉取等待时间，单位毫秒
	int32_t m_voiceFetchWaitMilliseconds;
	// 单条语音最大拉取尝试次数
	int32_t m_voiceFetchMaxAttempts;
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
