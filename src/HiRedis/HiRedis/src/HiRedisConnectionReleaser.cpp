#include "HiRedisConnectionReleaser.h"
#include "HiRedis.h"
#include "HiRedisConnectionPool.h"

HiRedisConnectionReleaser::HiRedisConnectionReleaser(HiRedisConnectionPool& pool) :
m_pool(&pool),
m_poolAlive(pool.m_aliveFlag),
m_returnGate(pool.m_returnGate)
{

}

void HiRedisConnectionReleaser::operator()(HiRedis* connection) const
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
            m_pool->releaseConnection(connection);
            return;
        }
    }

    delete connection;
}