#pragma once
#include <atomic>
#include <memory>

class MysqlCpp;
class MysqlCppConnectionPool;

/** MySQL连接池shared_ptr删除器，负责在连接借用结束时归还连接或兜底释放连接
*/
class MysqlCppConnectionReleaser
{
public:
    /** 构造函数，创建一个不绑定连接池的删除器
    */
    MysqlCppConnectionReleaser();

    /** 构造函数，绑定连接池和连接池生命周期标记
    @param [in] pool 连接所属连接池，不负责释放
    @param [in] poolAlive 连接池生命周期标记，用于避免连接池析构后访问悬挂指针
    */
    MysqlCppConnectionReleaser(MysqlCppConnectionPool* pool, const std::shared_ptr<std::atomic<bool>>& poolAlive);

    /** 归还或释放连接
    @param [in] connection 需要归还或释放的连接
    */
    void operator()(MysqlCpp* connection) const;

private:
    // 连接所属连接池，不负责释放
    MysqlCppConnectionPool* m_pool;
    // 连接池生命周期标记，用于避免连接池析构后归还连接时访问悬挂指针
    std::weak_ptr<std::atomic<bool>> m_poolAlive;
};