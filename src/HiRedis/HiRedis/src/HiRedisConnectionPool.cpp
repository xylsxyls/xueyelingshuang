#include "HiRedisConnectionPool.h"
#include "HiRedis.h"
#include "HiRedisConnectionReleaser.h"
#include <chrono>
#include <algorithm>
#include <stdexcept>
#include <new>

HiRedisConnectionPool::HiRedisConnectionPool() :
m_returnGate(std::make_shared<std::mutex>()),
m_minimum(1),
m_maximum(1),
m_pending(0),
m_generation(0),
m_aliveFlag(new (std::nothrow) std::atomic<bool>(true)),
m_isInit(false)
{

}

HiRedisConnectionPool::~HiRedisConnectionPool()
{
    std::lock_guard<std::mutex> guard(*m_returnGate);
    uninit();
    if (m_aliveFlag.get() != nullptr)
    {
        m_aliveFlag->store(false);
    }
}

bool HiRedisConnectionPool::init(const HiRedisConfig& config)
{
    return initRange(config, static_cast<size_t>(config.m_poolMinConnections), config.m_poolMaxConnections);
}

bool HiRedisConnectionPool::init(const HiRedisConfig& config, size_t connectionCount)
{
    return initRange(config, connectionCount, static_cast<int64_t>(connectionCount));
}

bool HiRedisConnectionPool::initRange(const HiRedisConfig& config, size_t minimum, int64_t maximum)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isInit)
    {
        return true;
    }
    if (!m_aliveFlag || minimum > m_allConnections.max_size() || maximum < -1 || maximum == 0 ||
        (maximum > 0 && minimum > static_cast<uint64_t>(maximum)))
    {
        m_lastError = "redis invalid pool capacity";
        return false;
    }
    if (!m_allConnections.empty() || m_pending != 0)
    {
        m_lastError = "redis pool still has active connections";
        return false;
    }
    try
    {
        m_config = config;
        std::vector<std::unique_ptr<HiRedis>> created;
        created.reserve(minimum);
        m_allConnections.reserve(minimum);
        m_availableConnections.reserve(minimum);
        for (size_t i = 0; i < minimum; ++i)
        {
            auto connection = createConnection(m_lastError);
            if (!connection)
            {
                return false;
            }
            created.push_back(std::move(connection));
        }
        for (size_t i = 0; i < minimum; ++i)
        {
            m_availableConnections.push_back(created[i].get());
            m_allConnections.push_back(created[i].release());
        }
    }
    catch (const std::exception& error)
    {
        m_lastError = error.what();
        return false;
    }
    m_minimum = minimum;
    m_maximum = maximum;
    m_lastError.clear();
    ++m_generation;
    m_isInit = true;
    return true;
}

std::unique_ptr<HiRedis> HiRedisConnectionPool::createConnection(std::string& error)
{
    std::unique_ptr<HiRedis> connection(new HiRedis);
    if (!connection->connect(m_config))
    {
        error = connection->lastError();
        connection.reset();
    }
    return connection;
}

void HiRedisConnectionPool::uninit()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isInit = false;
    for (size_t i = 0; i < m_availableConnections.size(); ++i)
    {
        HiRedis* connection = m_availableConnections[i];
        m_allConnections.erase(std::find(m_allConnections.begin(), m_allConnections.end(), connection));
        delete connection;
    }
    m_availableConnections.clear();
    m_lastError.clear();
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
        // 同时统计已创建和正在创建的名额，锁外的建连操作也受上限约束。
        const uint64_t generation = m_generation;
        const auto ready = [this, generation]() -> bool
        {
            return !m_isInit || m_generation != generation || !m_availableConnections.empty() || m_maximum < 0 ||
                static_cast<uint64_t>(m_allConnections.size()) + m_pending < static_cast<uint64_t>(m_maximum);
        };
        if (timeoutMs < 0)
        {
            m_condition.wait(lock, ready);
        }
        else if (!m_condition.wait_for(lock, std::chrono::milliseconds(timeoutMs), ready))
        {
            m_lastError = "redis pool acquire timeout";
            return nullptr;
        }
        if (!m_isInit || m_generation != generation)
        {
            m_lastError = "redis pool is not initialized";
            return nullptr;
        }
        if (m_availableConnections.empty())
        {
            ++m_pending;
            lock.unlock();
            std::string error;
            std::unique_ptr<HiRedis> created;
            try
            {
                created = createConnection(error);
            }
            catch (const std::exception& failure)
            {
                error = failure.what();
            }
            lock.lock();
            --m_pending;
            m_condition.notify_all();
            if (!m_isInit || !created)
            {
                m_lastError = error.empty() ? "redis pool closed during connection" : error;
                return nullptr;
            }
            try
            {
                m_availableConnections.reserve(m_allConnections.size() + 1);
                m_allConnections.push_back(created.get());
            }
            catch (const std::exception& failure)
            {
                m_lastError = failure.what();
                return nullptr;
            }
            connection = created.release();
        }
        else
        {
            connection = m_availableConnections.back();
            m_availableConnections.pop_back();
        }
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

        if (m_isInit && m_availableConnections.size() < m_minimum)
        {
            m_availableConnections.push_back(connection);
        }
        else
        {
            m_allConnections.erase(m_allConnections.begin() + i);
            delete connection;
        }
        m_condition.notify_one();
        return;
    }
    m_condition.notify_one();
}