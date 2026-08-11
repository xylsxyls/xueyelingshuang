#include "MysqlCppConnectionReleaser.h"
#include "MysqlCpp.h"
#include "MysqlCppConnectionPool.h"

MysqlCppConnectionReleaser::MysqlCppConnectionReleaser() :
m_pool(nullptr)
{

}

MysqlCppConnectionReleaser::MysqlCppConnectionReleaser(MysqlCppConnectionPool* pool, const std::shared_ptr<bool>& poolAlive) :
m_pool(pool),
m_poolAlive(poolAlive)
{

}

void MysqlCppConnectionReleaser::operator()(MysqlCpp* connection) const
{
    if (connection == nullptr)
    {
        return;
    }

    std::shared_ptr<bool> poolAlive = m_poolAlive.lock();
    if (m_pool != nullptr && poolAlive.get() != nullptr && *poolAlive)
    {
        m_pool->releaseConnectionFromReleaser(connection);
        return;
    }

    connection->uninit();
    delete connection;
}