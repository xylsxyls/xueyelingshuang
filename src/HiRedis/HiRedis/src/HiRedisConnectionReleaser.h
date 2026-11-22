#pragma once
#include <atomic>
#include <memory>
#include <mutex>

class HiRedis;
class HiRedisConnectionPool;

/** Redis连接池shared_ptr删除器，负责在连接借用结束时归还连接或兜底释放连接
*/
class HiRedisConnectionReleaser
{
public:
    /** 归还或释放连接
    @param [in] connection 需要归还或释放的Redis连接
    */
    void operator()(HiRedis* connection) const;

private:
    friend class HiRedisConnectionPool;

    /** 只由池为本次借出的连接建立删除器，池和归还门共享同一生存期协议
    @param [in] pool 正在发放这份借用的池，其析构与归还串行
    */
    explicit HiRedisConnectionReleaser(HiRedisConnectionPool& pool);

private:
    // 连接所属连接池，不负责释放
    HiRedisConnectionPool* m_pool;
    // 连接池生命周期标记，用于避免连接池析构后归还连接时访问悬挂指针
    std::weak_ptr<std::atomic<bool>> m_poolAlive;
    // 与析构串行，避免在池的存活检查和实际归还之间失效
    std::shared_ptr<std::mutex> m_returnGate;
};