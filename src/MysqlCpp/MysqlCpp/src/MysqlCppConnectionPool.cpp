#include "MysqlCppConnectionPool.h"
#include "MysqlCpp.h"
#include "MysqlCppConnectionReleaser.h"
#include <chrono>
#include <new>

MysqlCppConnectionPool::MysqlCppConnectionPool() :
m_aliveFlag(new (std::nothrow) std::atomic<bool>(true)),
m_isInit(false)
{

}

MysqlCppConnectionPool::~MysqlCppConnectionPool()
{
    uninit();
    if (m_aliveFlag.get() != nullptr)
    {
        m_aliveFlag->store(false);
    }
}

bool MysqlCppConnectionPool::init(const MysqlCppConfig& config, size_t connectionCount)
{
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (m_isInit)
        {
            return true;
        }
    }
    if (m_aliveFlag.get() == nullptr)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_lastError = "mysql pool alive flag alloc failed";
        return false;
    }
    if (connectionCount == 0)
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_lastError = "mysql pool size is zero";
        return false;
    }
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        if (!m_allConnections.empty())
        {
            m_lastError = "mysql pool still has borrowed connections";
            return false;
        }
    }

    auto deleteConnection = [](MysqlCpp* connection)
    {
        if (connection == nullptr)
        {
            return;
        }
        connection->uninit();
        delete connection;
    };
    auto deleteConnections = [&deleteConnection](std::vector<MysqlCpp*>& connections)
    {
        for (size_t i = 0; i < connections.size(); ++i)
        {
            deleteConnection(connections[i]);
        }
        connections.clear();
    };

    std::vector<MysqlCpp*> allConnections;
    std::vector<MysqlCpp*> availableConnections;
    for (size_t i = 0; i < connectionCount; ++i)
    {
        MysqlCpp* connection = new (std::nothrow) MysqlCpp;
        if (connection == nullptr)
        {
            deleteConnections(allConnections);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_lastError = "mysql connection alloc failed";
            return false;
        }
        if (!connection->init())
        {
            std::string error = connection->lastError();
            deleteConnection(connection);
            deleteConnections(allConnections);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_lastError = error;
            return false;
        }
        if (!connection->connect(config))
        {
            std::string error = connection->lastError();
            deleteConnection(connection);
            deleteConnections(allConnections);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_lastError = error;
            return false;
        }
        allConnections.push_back(connection);
        availableConnections.push_back(connection);
    }

    std::unique_lock<std::mutex> lock(m_mutex);
    if (m_isInit)
    {
        lock.unlock();
        deleteConnections(allConnections);
        return true;
    }
    if (!m_allConnections.empty())
    {
        m_lastError = "mysql pool still has borrowed connections";
        lock.unlock();
        deleteConnections(allConnections);
        return false;
    }
    m_allConnections = allConnections;
    m_availableConnections = availableConnections;
    m_lastError.clear();
    m_isInit = true;
    return true;
}

void MysqlCppConnectionPool::uninit()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    if (!m_isInit)
    {
        return;
    }
    std::vector<MysqlCpp*> borrowedConnections;
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

void MysqlCppConnectionPool::close()
{
    uninit();
}

std::shared_ptr<MysqlCpp> MysqlCppConnectionPool::acquire(int32_t timeoutMs)
{
    MysqlCpp* connection = nullptr;
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
            m_lastError = "mysql pool is not initialized";
            return std::shared_ptr<MysqlCpp>();
        }
        if (m_availableConnections.empty())
        {
            m_lastError = "mysql pool acquire timeout";
            return std::shared_ptr<MysqlCpp>();
        }

        connection = m_availableConnections.back();
        m_availableConnections.pop_back();
        m_lastError.clear();
    }

    if (!connection->ping())
    {
        if (!connection->reconnect())
        {
            std::string error = connection->lastError();
            releaseConnection(connection);
            std::unique_lock<std::mutex> lock(m_mutex);
            m_lastError = error.empty() ? "mysql pool reconnect failed" : error;
            return std::shared_ptr<MysqlCpp>();
        }
    }

    MysqlCppConnectionReleaser releaser(this, m_aliveFlag);
    return std::shared_ptr<MysqlCpp>(connection, releaser);
}

size_t MysqlCppConnectionPool::availableCount()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_availableConnections.size();
}

size_t MysqlCppConnectionPool::totalCount()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_allConnections.size();
}

std::string MysqlCppConnectionPool::lastError()
{
    std::unique_lock<std::mutex> lock(m_mutex);
    return m_lastError;
}

void MysqlCppConnectionPool::releaseConnectionFromReleaser(MysqlCpp* connection)
{
    releaseConnection(connection);
}

void MysqlCppConnectionPool::releaseConnection(MysqlCpp* connection)
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