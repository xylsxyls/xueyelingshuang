#include "HiRedisConnectionPool.h"
#include "HiRedis.h"
#include "HiRedisConnectionReleaser.h"
#include <chrono>

HiRedisConnectionPool::HiRedisConnectionPool() :
m_aliveFlag(new std::atomic<bool>(true)),
m_isInit(false)
{

}

HiRedisConnectionPool::~HiRedisConnectionPool()
{
    if (m_aliveFlag.get() != nullptr)
    {
        m_aliveFlag->store(false);
    }
}

bool HiRedisConnectionPool::init(const HiRedisConfig& config, size_t connectionCount)
{
    uninit();
    if (connectionCount == 0)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_lastError = "redis pool size is zero";
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_allConnections.empty())
        {
            m_lastError = "redis pool still has borrowed connections";
            return false;
        }
    }

    std::vector<HiRedis*> allConnections;
    std::vector<HiRedis*> availableConnections;
    for (size_t i = 0; i < connectionCount; ++i)
    {
        HiRedis* connection = new HiRedis;
        if (!connection->init(config))
        {
            std::string error = connection->lastError();
            connection->uninit();
            delete connection;
            for (size_t j = 0; j < allConnections.size(); ++j)
            {
                allConnections[j]->uninit();
                delete allConnections[j];
            }
            std::unique_lock<std::mutex> lock(m_mutex);
            m_lastError = error;
            return false;
        }
        allConnections.push_back(connection);
        availableConnections.push_back(connection);
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_allConnections.empty())
    {
        for (size_t i = 0; i < allConnections.size(); ++i)
        {
            allConnections[i]->uninit();
            delete allConnections[i];
        }
        m_lastError = "redis pool still has borrowed connections";
        return false;
    }
    m_allConnections = allConnections;
    m_availableConnections = availableConnections;
    m_lastError.clear();
    m_isInit = true;
    return true;
}

void HiRedisConnectionPool::uninit()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    std::vector<HiRedis*> borrowedConnections;
    for (size_t i = 0; i < m_allConnections.size(); ++i)
    {
        bool isAvailable = false;
        for (size_t j = 0; j < m_availableConnections.size(); ++j)
        {
            if (m_allConnections[i] == m_availableConnections[j])
            {
                isAvailable = true;
                break;
            }
        }

        if (isAvailable)
        {
            m_allConnections[i]->uninit();
            delete m_allConnections[i];
        }
        else
        {
            borrowedConnections.push_back(m_allConnections[i]);
        }
    }
    m_allConnections = borrowedConnections;
    m_availableConnections.clear();
    m_lastError.clear();
    m_isInit = false;
    m_condition.notify_all();
}

void HiRedisConnectionPool::close()
{
    uninit();
}

std::shared_ptr<HiRedis> HiRedisConnectionPool::acquire(int32_t timeoutMs)
{
    HiRedis* connection = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (timeoutMs < 0)
        {
            while (m_isInit && m_availableConnections.empty() && !m_allConnections.empty())
            {
                m_condition.wait(lock);
            }
        }
        else if (m_isInit && m_availableConnections.empty() && !m_allConnections.empty())
        {
            m_condition.wait_for(lock, std::chrono::milliseconds(timeoutMs), [this]() {
                return !m_isInit || !m_availableConnections.empty() || m_allConnections.empty();
            });
        }

        if (!m_isInit)
        {
            m_lastError = "redis pool is not initialized";
            return std::shared_ptr<HiRedis>();
        }
        if (m_availableConnections.empty())
        {
            m_lastError = "redis pool acquire timeout";
            return std::shared_ptr<HiRedis>();
        }

        connection = m_availableConnections.back();
        m_availableConnections.pop_back();
        m_lastError.clear();
    }

    if (!connection->isConnected())
    {
        if (!connection->reconnect())
        {
            std::string error = connection->lastError();
            releaseConnection(connection);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_lastError = error.empty() ? "redis pool reconnect failed" : error;
            return std::shared_ptr<HiRedis>();
        }
    }

    HiRedisConnectionReleaser releaser(this, m_aliveFlag);
    return std::shared_ptr<HiRedis>(connection, releaser);
}

size_t HiRedisConnectionPool::availableCount()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_availableConnections.size();
}

size_t HiRedisConnectionPool::totalCount()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_allConnections.size();
}

std::string HiRedisConnectionPool::lastError()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_lastError;
}

void HiRedisConnectionPool::releaseConnectionFromReleaser(HiRedis* connection)
{
    releaseConnection(connection);
}

void HiRedisConnectionPool::releaseConnection(HiRedis* connection)
{
    if (connection == nullptr)
    {
        return;
    }
    std::unique_lock<std::mutex> lock(m_mutex);
    for (size_t i = 0; i < m_availableConnections.size(); ++i)
    {
        if (m_availableConnections[i] == connection)
        {
            m_condition.notify_one();
            return;
        }
    }

    for (size_t i = 0; i < m_allConnections.size(); ++i)
    {
        if (m_allConnections[i] != connection)
        {
            continue;
        }

        if (m_isInit)
        {
            m_availableConnections.push_back(connection);
        }
        else
        {
            m_allConnections.erase(m_allConnections.begin() + i);
            connection->uninit();
            delete connection;
        }
        m_condition.notify_one();
        return;
    }
    m_condition.notify_one();
}