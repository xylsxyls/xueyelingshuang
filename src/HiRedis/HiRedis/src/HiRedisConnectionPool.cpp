#include "HiRedisConnectionPool.h"
#include "HiRedis.h"
#include "HiRedisHelper.h"
#include "HiRedisConnectionReleaser.h"
#include <chrono>
#include <utility>
#include <stdexcept>
#include <new>

HiRedisConnectionPool::HiRedisConnectionPool() :
m_borrowed(0),
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
    if (!m_aliveFlag || minimum > m_availableConnections.max_size() || maximum < -1 || maximum == 0 ||
        (maximum > 0 && minimum > static_cast<uint64_t>(maximum)))
    {
        m_lastError = "redis invalid pool capacity";
        return false;
    }
    const char* invalid = HiRedisHelper::connectionConfigError(config);
    if (invalid)
    {
        m_lastError = invalid;
        return false;
    }
    if (m_borrowed != 0 || m_pending != 0)
    {
        m_lastError = "redis pool still has active connections";
        return false;
    }
    try
    {
        m_config = config;
        std::vector<std::unique_ptr<HiRedis>> created;
        created.reserve(minimum);
        for (size_t i = 0; i < minimum; ++i)
        {
            auto connection = createConnection(m_lastError);
            if (!connection)
            {
                return false;
            }
            created.push_back(std::move(connection));
        }
        // 全部预建成功才发布；中途失败由局部容器关闭已建立的连接。
        m_availableConnections.swap(created);
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
    return acquireImpl(false, timeoutMs);
}

std::shared_ptr<HiRedis> HiRedisConnectionPool::acquire()
{
    return acquireImpl(true, 0);
}

std::shared_ptr<HiRedis> HiRedisConnectionPool::acquireImpl(bool useDefault, int32_t timeoutMs)
{
    HiRedis* connection = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (useDefault)
        {
            timeoutMs = m_config.m_poolAcquireTimeoutMs;
        }
        // 同时统计已创建和正在创建的名额，锁外的建连操作也受上限约束。
        const uint64_t generation = m_generation;
        const auto ready = [this, generation]() -> bool
        {
            return !m_isInit || m_generation != generation || !m_availableConnections.empty() || m_maximum < 0 ||
                static_cast<uint64_t>(m_borrowed) + m_pending < static_cast<uint64_t>(m_maximum);
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
            std::string error;
            std::unique_ptr<HiRedis> created;
            ++m_pending;
            lock.unlock();
            try
            {
                created = createConnection(error);
            }
            catch (const std::exception& failure)
            {
                lock.lock();
                --m_pending;
                m_condition.notify_all();
                m_lastError = failure.what();
                return nullptr;
            }
            lock.lock();
            --m_pending;
            m_condition.notify_all();
            if (!m_isInit || !created)
            {
                m_lastError = error.empty() ? "redis pool closed during connection" : error;
                return nullptr;
            }
            connection = created.release();
        }
        else
        {
            connection = m_availableConnections.back().release();
            m_availableConnections.pop_back();
        }
        ++m_borrowed;
        m_lastError.clear();
    }

    HiRedisConnectionReleaser releaser(*this);
    std::shared_ptr<HiRedis> lease(connection, releaser);
    // 名额在归还前一直占用，期间池不能重新初始化，m_config 快照保持有效。
    if (!connection->isConnected() && !connection->connect(m_config))
    {
        std::string error = connection->lastError();
        lease.reset();
        std::unique_lock<std::mutex> lock(m_mutex);
        m_lastError = error.empty() ? "redis pool reconnect failed" : error;
        return nullptr;
    }

    return lease;
}

size_t HiRedisConnectionPool::availableCount()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_availableConnections.size();
}

size_t HiRedisConnectionPool::totalCount()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_borrowed + m_availableConnections.size();
}

std::string HiRedisConnectionPool::lastError()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_lastError;
}

void HiRedisConnectionPool::releaseConnection(HiRedis* connection)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    // 未保留的连接在解锁前关闭，下一次建连不会与其争用同一名额。
    std::unique_ptr<HiRedis> returned(connection);
    --m_borrowed;
    if (m_isInit && m_availableConnections.size() < m_minimum)
    {
        m_availableConnections.push_back(std::move(returned));
    }
    m_condition.notify_one();
}