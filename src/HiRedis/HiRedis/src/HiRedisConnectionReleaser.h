#pragma once
#include <atomic>
#include <memory>

class HiRedis;
class HiRedisConnectionPool;

/** Redis连接池shared_ptr删除器，负责在连接借用结束时归还连接或兜底释放连接
*/
class HiRedisConnectionReleaser
{
public:
    /** 构造函数，创建空释放器
    */
    HiRedisConnectionReleaser();

    /** 构造函数，绑定连接池和连接池生命周期标记
    @param [in] pool 连接所属连接池，不负责释放
    @param [in] poolAlive 连接池生命周期标记，用于避免连接池析构后访问悬挂指针
    */
    HiRedisConnectionReleaser(HiRedisConnectionPool* pool, const std::shared_ptr<std::atomic<bool>>& poolAlive);

    /** 归还或释放连接
    @param [in] connection 需要归还或释放的Redis连接
    */
    void operator()(HiRedis* connection) const;

private:
    // 连接所属连接池，不负责释放
    HiRedisConnectionPool* m_pool;
    // 连接池生命周期标记，用于避免连接池析构后归还连接时访问悬挂指针
    std::weak_ptr<std::atomic<bool>> m_poolAlive;
};