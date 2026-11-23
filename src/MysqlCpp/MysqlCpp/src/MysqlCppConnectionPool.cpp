#include "MysqlCppConnectionPool.h"
#include "MysqlCpp.h"
#include "MysqlCppPoolLifetime.h"
#include <chrono>
#include <exception>

MysqlCppConnectionPool::MysqlCppConnectionPool() :
m_lifetime(std::make_shared<MysqlCppPoolLifetime>()),
m_minimum(m_config.m_poolMinConnections),
m_maximum(m_config.m_poolMaxConnections),
m_total(0),
m_pending(0),
m_generation(0),
m_isInit(false)
{
    m_lifetime->m_pool = this;
}

MysqlCppConnectionPool::~MysqlCppConnectionPool()
{
    std::lock_guard<std::mutex> guard(m_lifetime->m_mutex);
    uninit();
    m_lifetime->m_pool = nullptr;
}

bool MysqlCppConnectionPool::init(const MysqlCppConfig& config)
{
    return initRange(config, static_cast<size_t>(config.m_poolMinConnections), config.m_poolMaxConnections);
}

bool MysqlCppConnectionPool::init(const MysqlCppConfig& config, size_t connectionCount)
{
    return initRange(config, connectionCount, static_cast<int64_t>(connectionCount));
}

bool MysqlCppConnectionPool::initRange(const MysqlCppConfig& config, size_t minimum, int64_t maximum)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isInit)
    {
        return true;
    }
    if (!config.validConnection())
    {
        m_lastError = "invalid mysql connection parameters";
        return false;
    }
    if (minimum > m_availableConnections.max_size() || maximum < -1 || maximum == 0 ||
        (maximum > 0 && minimum > static_cast<uint64_t>(maximum)))
    {
        m_lastError = "mysql invalid pool capacity";
        return false;
    }
    if (m_total != 0 || m_pending != 0)
    {
        m_lastError = "mysql pool still has active connections";
        return false;
    }
    try
    {
        m_config = config;
        std::vector<std::unique_ptr<MysqlCpp>> created;
        created.reserve(minimum);
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
            m_availableConnections.push_back(created[i].release());
        }
    }
    catch (const std::exception& error)
    {
        m_lastError = error.what();
        return false;
    }
    m_total = minimum;
    m_minimum = minimum;
    m_maximum = maximum;
    m_lastError.clear();
    ++m_generation;
    m_isInit = true;
    return true;
}

std::unique_ptr<MysqlCpp> MysqlCppConnectionPool::createConnection(std::string& error)
{
    std::unique_ptr<MysqlCpp> connection(new MysqlCpp);
    if (!connection->connect(m_config))
    {
        error = connection->lastError();
        connection.reset();
    }
    return connection;
}

void MysqlCppConnectionPool::uninit()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    m_isInit = false;
    for (size_t i = 0; i < m_availableConnections.size(); ++i)
    {
        delete m_availableConnections[i];
    }
    m_total -= m_availableConnections.size();
    m_availableConnections.clear();
    m_lastError.clear();
    m_condition.notify_all();
}

void MysqlCppConnectionPool::close()
{
    uninit();
}

std::shared_ptr<MysqlCpp> MysqlCppConnectionPool::acquire()
{
    return acquireWithTimeout(nullptr);
}

std::shared_ptr<MysqlCpp> MysqlCppConnectionPool::acquire(int32_t timeoutMs)
{
    return acquireWithTimeout(&timeoutMs);
}

std::shared_ptr<MysqlCpp> MysqlCppConnectionPool::acquireWithTimeout(const int32_t* requestedTimeout)
{
    MysqlCpp* connection = nullptr;
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        // 同时统计已创建和正在创建的名额，锁外的建连操作也受上限约束。
        const int32_t timeoutMs = requestedTimeout ? *requestedTimeout : m_config.m_poolAcquireTimeoutMs;
        const uint64_t generation = m_generation;
        const auto ready = [this, generation]() -> bool
        {
            return !m_isInit || m_generation != generation || !m_availableConnections.empty() || m_maximum < 0 ||
                static_cast<uint64_t>(m_total) + m_pending < static_cast<uint64_t>(m_maximum);
        };
        if (timeoutMs < 0)
        {
            m_condition.wait(lock, ready);
        }
        else if (!m_condition.wait_for(lock, std::chrono::milliseconds(timeoutMs), ready))
        {
            m_lastError = "mysql pool acquire timeout";
            return nullptr;
        }
        if (!m_isInit || m_generation != generation)
        {
            m_lastError = "mysql pool is not initialized";
            return nullptr;
        }
        if (m_availableConnections.empty())
        {
            ++m_pending;
            lock.unlock();
            std::string error;
            std::unique_ptr<MysqlCpp> created;
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
                m_lastError = error.empty() ? "mysql pool closed during connection" : error;
                return nullptr;
            }
            ++m_total;
            connection = created.release();
        }
        else
        {
            connection = m_availableConnections.back();
            m_availableConnections.pop_back();
        }
        m_lastError.clear();
    }

    // 外借者可以修改乃至清空连接自身的配置。名额归还后，重建必须使用本代池的配置。
    if (!connection->ping() && !connection->connect(m_config))
    {
        const std::string error = connection->lastError();
        releaseConnection(connection, false);
        std::unique_lock<std::mutex> lock(m_mutex);
        m_lastError = error.empty() ? "mysql pool reconnect failed" : error;
        return nullptr;
    }

    const auto lifetime = m_lifetime;
    // 最后一次释放时接管 borrowed；持锁检查池的存活状态，析构完成后直接释放连接。
    return std::shared_ptr<MysqlCpp>(connection, [lifetime](MysqlCpp* borrowed)
    {
        std::lock_guard<std::mutex> guard(lifetime->m_mutex);
        if (lifetime->m_pool)
        {
            lifetime->m_pool->releaseConnection(borrowed);
        }
        else
        {
            delete borrowed;
        }
    });
}

size_t MysqlCppConnectionPool::availableCount()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_availableConnections.size();
}

size_t MysqlCppConnectionPool::totalCount()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_total;
}

std::string MysqlCppConnectionPool::lastError()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_lastError;
}

void MysqlCppConnectionPool::releaseConnection(MysqlCpp* connection, bool reusable)
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (reusable && m_isInit && m_availableConnections.size() < m_minimum)
    {
        // 初始化时已预留空闲目标容量，归还到该范围不会再分配。
        m_availableConnections.push_back(connection);
    }
    else
    {
        --m_total;
        delete connection;
    }
    m_condition.notify_one();
}