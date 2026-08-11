#include "HiRedisConnectionReleaser.h"
#include "HiRedis.h"
#include "HiRedisConnectionPool.h"

HiRedisConnectionReleaser::HiRedisConnectionReleaser() :
m_pool(nullptr)
{

}

HiRedisConnectionReleaser::HiRedisConnectionReleaser(HiRedisConnectionPool* pool, const std::shared_ptr<std::atomic<bool>>& poolAlive) :
m_pool(pool),
m_poolAlive(poolAlive)
{

}

void HiRedisConnectionReleaser::operator()(HiRedis* connection) const
{
    if (connection == nullptr)
    {
        return;
    }

    std::shared_ptr<std::atomic<bool>> poolAlive = m_poolAlive.lock();
    if (m_pool != nullptr && poolAlive.get() != nullptr && poolAlive->load())
    {
        m_pool->releaseConnectionFromReleaser(connection);
        return;
    }

    connection->uninit();
    delete connection;
}