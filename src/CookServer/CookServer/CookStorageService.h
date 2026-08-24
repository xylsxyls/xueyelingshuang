#pragma once

#include "HiRedis/HiRedisAPI.h"
#include "MysqlCpp/MysqlCppAPI.h"
#include <atomic>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <vector>

/** CookServer运行期存储服务
负责Redis热层、MySQL最终落库和后台同步任务调度。第一期先承接AccountStore状态快照，
后续可逐步拆成结构化Repository。
*/
class CookStorageService
{
public:
	/** 获取全局存储服务实例
	@return 返回存储服务单例
	*/
	static CookStorageService& instance();

	/** 初始化Redis、MySQL和后台同步线程
	@return 成功返回true，否则返回false
	*/
	bool init();

	/** 等待后台同步任务完成并释放连接池
	*/
	void shutdown();

	/** 当前是否启用Redis+MySQL存储
	@return 启用返回true，否则返回false
	*/
	bool isEnabled() const;

	/** 加载AccountStore状态快照
	@param [out] stateText 状态快照文本
	@return 成功加载返回true，无快照或失败返回false
	*/
	bool loadAccountState(std::string& stateText);

	/** 先写Redis，再投递MySQL后台同步任务
	@param [in] stateText 状态快照文本
	@return Redis写入或同步兜底成功返回true，否则返回false
	*/
	bool saveAccountStateAsync(const std::string& stateText);

	/** 同步写入MySQL，主要用于Redis失败兜底或关闭前补偿
	@param [in] stateText 状态快照文本
	@return 成功返回true，否则返回false
	*/
	bool saveAccountStateSync(const std::string& stateText);

	/** 后台任务入口：把指定版本状态同步到MySQL
	@param [in] stateText 状态快照文本
	@param [in] version 状态版本
	@return 成功返回true，否则返回false
	*/
	bool syncStateSnapshotToMysql(const std::string& stateText, int64_t version);

private:
	/** 构造函数
	*/
	CookStorageService();

	/** 析构函数
	*/
	~CookStorageService();

	/** 禁止拷贝构造
	@param [in] other 另一个存储服务
	*/
	CookStorageService(const CookStorageService& other);

	/** 禁止赋值
	@param [in] other 另一个存储服务
	@return 返回当前对象引用
	*/
	CookStorageService& operator=(const CookStorageService& other);

	/** 判断配置是否要求使用Redis+MySQL
	@return 是返回true，否则返回false
	*/
	bool shouldUseRedisMysqlBackend() const;

	/** 初始化MySQL数据库、连接池和表结构
	@return 成功返回true，否则返回false
	*/
	bool initMysql();

	/** 初始化Redis连接池
	@return 成功返回true，否则返回false
	*/
	bool initRedis();

	/** 启动MySQL后台同步线程
	@return 成功返回true，否则返回false
	*/
	bool startSyncThreads();

	/** 创建业务数据库
	@return 成功返回true，否则返回false
	*/
	bool createMysqlDatabase();

	/** 创建存储层需要的MySQL表
	@return 成功返回true，否则返回false
	*/
	bool createMysqlTables();

	/** 启动时根据Redis和MySQL现状初始化Redis快照
	@return 成功返回true，否则返回false
	*/
	bool initRedisSnapshotFromMysql();

	/** 读取MySQL状态快照
	@param [out] stateText 状态文本
	@param [out] version 状态版本
	@return 读取到快照返回true，否则返回false
	*/
	bool loadMysqlStateSnapshot(std::string& stateText, int64_t& version);

	/** 读取Redis状态快照
	@param [out] stateText 状态文本
	@param [out] version 状态版本
	@return 读取到快照返回true，否则返回false
	*/
	bool loadRedisStateSnapshot(std::string& stateText, int64_t& version);

	/** 写入Redis状态快照
	@param [in] stateText 状态文本
	@param [in] version 状态版本
	@return 成功返回true，否则返回false
	*/
	bool writeRedisStateSnapshot(const std::string& stateText, int64_t version);

	/** 写入MySQL状态快照，使用版本号防止旧快照覆盖新快照
	@param [in] stateText 状态文本
	@param [in] version 状态版本
	@return 成功返回true，否则返回false
	*/
	bool writeMysqlStateSnapshot(const std::string& stateText, int64_t version);

	/** 执行无参数MySQL更新语句
	@param [in] mysql MySQL连接
	@param [in] sql SQL文本
	@return 成功返回true，否则返回false
	*/
	bool executeMysqlUpdate(const std::shared_ptr<MysqlCpp>& mysql, const std::string& sql) const;

	/** 获取一个MySQL连接
	@return 返回MySQL连接，失败返回空指针
	*/
	std::shared_ptr<MysqlCpp> acquireMysql();

	/** 获取一个Redis连接
	@return 返回Redis连接，失败返回空指针
	*/
	std::shared_ptr<HiRedis> acquireRedis();

	/** 构建MySQL连接配置
	@param [in] withDatabase 是否自动选择业务库
	@return 返回连接配置
	*/
	MysqlCppConfig mysqlConfig(bool withDatabase) const;

	/** 构建Redis连接配置
	@return 返回Redis连接配置
	*/
	HiRedisConfig redisConfig() const;

	/** 检查MySQL标识符是否合法
	@param [in] identifier 标识符
	@return 合法返回true，否则返回false
	*/
	bool isMysqlIdentifierValid(const std::string& identifier) const;

	/** Redis快照key
	@return 返回Redis key
	*/
	std::string redisSnapshotKey() const;

	/** Redis快照版本key
	@return 返回Redis key
	*/
	std::string redisSnapshotVersionKey() const;

	/** Redis key前缀
	@return 返回规整后的key前缀
	*/
	std::string redisKeyPrefix() const;

	/** MySQL状态快照业务key
	@return 返回快照业务key
	*/
	std::string mysqlSnapshotKey() const;

private:
	std::atomic<bool> m_initialized;
	std::atomic<bool> m_enabled;
	std::atomic<bool> m_shuttingDown;
	std::atomic<int64_t> m_stateVersion;
	HiRedisConnectionPool m_redisPool;
	MysqlCppConnectionPool m_mysqlPool;
	std::vector<uint32_t> m_syncThreadIds;
	size_t m_nextSyncThreadIndex;
	std::mutex m_mutex;
};
