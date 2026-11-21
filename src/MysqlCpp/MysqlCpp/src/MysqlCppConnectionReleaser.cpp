#include "MysqlCppConnectionReleaser.h"
#include "MysqlCpp.h"
#include "MysqlCppConnectionPool.h"

MysqlCppConnectionReleaser::MysqlCppConnectionReleaser() :
m_pool(nullptr)
{

}

MysqlCppConnectionReleaser::MysqlCppConnectionReleaser(MysqlCppConnectionPool* pool, const std::shared_ptr<std::atomic<bool>>& poolAlive) :
m_pool(pool),
m_poolAlive(poolAlive),
m_returnGate(pool == nullptr ? nullptr : pool->m_returnGate)
{

}

void MysqlCppConnectionReleaser::operator()(MysqlCpp* connection) const
{
    if (connection == nullptr)
    {
        return;
    }

    std::shared_ptr<std::atomic<bool>> poolAlive = m_poolAlive.lock();
    if (m_returnGate && poolAlive)
    {
        std::lock_guard<std::mutex> guard(*m_returnGate);
        if (poolAlive->load())
        {
            m_pool->releaseConnectionFromReleaser(connection);
            return;
        }
    }

    connection->uninit();
    delete connection;
}