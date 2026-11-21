#include "MysqlCpp.h"
#include "MysqlCppHelper.h"
#include "MysqlCppPrepareStatement.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstdlib>
#include <memory>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "mysql-connector-cpp/cppconn/connection.h"
#include "mysql-connector-cpp/cppconn/driver.h"
#include "mysql-connector-cpp/cppconn/exception.h"
#include "mysql-connector-cpp/cppconn/prepared_statement.h"
#include "mysql-connector-cpp/cppconn/resultset.h"
#include "mysql-connector-cpp/cppconn/statement.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

/** 在当前连接上执行固定的事务控制语句，服务器拒绝时由驱动的语句接口抛出 SQL 异常。
@param [in] connection 已连接的会话
@param [in] command 本文件选定的固定语句
*/
static void ExecuteSessionCommand(sql::Connection& connection, const char* command)
{
    // Connector/C++ 1.1.13 的 Connection::commit、rollback 和 setAutoCommit 会忽略 C API 的错误返回值。
    std::unique_ptr<sql::Statement> statement(connection.createStatement());
    statement->execute(command);
}

MysqlCpp::MysqlCpp() :
m_driver(nullptr),
m_con(nullptr),
m_isInit(false)
{

}

MysqlCpp::~MysqlCpp()
{
    uninit();
}

bool MysqlCpp::init()
{
    if (m_isInit)
    {
        return true;
    }

    try
    {
        m_driver = get_driver_instance();
        if (m_driver == nullptr)
        {
            setLastError("mysql driver is null");
            return false;
        }
        m_isInit = true;
        setLastError("");
        return true;
    }
    catch (...)
    {
        m_driver = nullptr;
        m_isInit = false;
        setLastError("get_driver_instance failed");
        return false;
    }
}

void MysqlCpp::uninit()
{
    close();
    m_driver = nullptr;
    m_config = MysqlCppConfig();
    m_lastError.clear();
    m_isInit = false;
}

bool MysqlCpp::isInit() const
{
    return m_isInit;
}

bool MysqlCpp::connect(const std::string& ip,
                       int32_t port,
                       const std::string& account,
                       const std::string& password)
{
    MysqlCppConfig config;
    config.m_ip = ip;
    config.m_port = port;
    config.m_account = account;
    config.m_password = password;
    return connect(config);
}

bool MysqlCpp::connect(const MysqlCppConfig& config)
{
    if (config.m_port < 1 || config.m_port > 65535)
    {
        setLastError("TCP port must be between 1 and 65535");
        return false;
    }
    if (config.m_ip.find('\0') != std::string::npos || config.m_account.find('\0') != std::string::npos ||
        config.m_password.find('\0') != std::string::npos || config.m_dbName.find('\0') != std::string::npos)
    {
        setLastError("mysql connection text cannot contain NUL");
        return false;
    }
    if (!m_isInit && !init())
    {
        return false;
    }
    if (m_driver == nullptr)
    {
        setLastError("mysql driver is null");
        return false;
    }

    close();
    m_config = config;
    try
    {
        MysqlCppConfig defaultConfig;
        std::string url = "tcp://" + config.m_ip + ":" + CStringManager::toStringInt32(config.m_port);
        sql::ConnectOptionsMap connectionProperties;
        connectionProperties["hostName"] = url;
        connectionProperties["userName"] = config.m_account;
        connectionProperties["password"] = config.m_password;
        connectionProperties["OPT_CONNECT_TIMEOUT"] = MysqlCppHelper::normalizeTimeoutSeconds(config.m_connectTimeoutSeconds, defaultConfig.m_connectTimeoutSeconds);
        connectionProperties["OPT_READ_TIMEOUT"] = MysqlCppHelper::normalizeTimeoutSeconds(config.m_readTimeoutSeconds, defaultConfig.m_readTimeoutSeconds);
        connectionProperties["OPT_WRITE_TIMEOUT"] = MysqlCppHelper::normalizeTimeoutSeconds(config.m_writeTimeoutSeconds, defaultConfig.m_writeTimeoutSeconds);
        connectionProperties["OPT_RECONNECT"] = config.m_autoReconnect;
        m_con = m_driver->connect(connectionProperties);
        if (m_con == nullptr)
        {
            setLastError("mysql connect returned null");
            return false;
        }
        if (!config.m_dbName.empty())
        {
            m_con->setSchema(config.m_dbName.c_str());
        }
        ExecuteSessionCommand(*m_con, config.m_autoCommit ? "SET SESSION autocommit=1" : "SET SESSION autocommit=0");
        if (!m_con->isValid())
        {
            close();
            setLastError("mysql connection is invalid");
            return false;
        }
        m_session = std::make_shared<unsigned char>(0);
        setLastError("");
        return true;
    }
    catch (const sql::SQLException& e)
    {
        close();
        setLastError(MysqlCppHelper::exceptionText(e));
        return false;
    }
    catch (...)
    {
        close();
        setLastError("mysql connect failed");
        return false;
    }
}

void MysqlCpp::close()
{
    m_session.reset();
    if (m_con != nullptr)
    {
        delete m_con;
        m_con = nullptr;
    }
}

bool MysqlCpp::reconnect()
{
    m_session.reset();
    if (m_con != nullptr)
    {
        try
        {
            if (m_con->reconnect())
            {
                if (!m_config.m_dbName.empty())
                {
                    m_con->setSchema(m_config.m_dbName.c_str());
                }
                ExecuteSessionCommand(*m_con, m_config.m_autoCommit ? "SET SESSION autocommit=1" : "SET SESSION autocommit=0");
                m_session = std::make_shared<unsigned char>(0);
                setLastError("");
                return true;
            }
        }
        catch (...)
        {
        }
    }
    return connect(m_config);
}

bool MysqlCpp::ping() const
{
    if (m_con == nullptr)
    {
        return false;
    }
    try
    {
        return m_con->isValid();
    }
    catch (...)
    {
        return false;
    }
}

bool MysqlCpp::isConnected() const
{
    return ping();
}

std::string MysqlCpp::lastError() const
{
    return m_lastError;
}

bool MysqlCpp::selectDb(const std::string& dbName, bool autoCommit)
{
    if (dbName.find('\0') != std::string::npos)
    {
        setLastError("database name cannot contain NUL");
        return false;
    }
    if (!check())
    {
        setLastError("mysql connection is not available");
        return false;
    }
    try
    {
        m_con->setSchema(dbName.c_str());
        m_config.m_dbName = dbName;
        ExecuteSessionCommand(*m_con, autoCommit ? "SET SESSION autocommit=1" : "SET SESSION autocommit=0");
        m_config.m_autoCommit = autoCommit;
        setLastError("");
        return true;
    }
    catch (const sql::SQLException& e)
    {
        setLastError(MysqlCppHelper::exceptionText(e));
        return false;
    }
    catch (...)
    {
        setLastError("select database failed");
        return false;
    }
}

std::shared_ptr<MysqlCppPrepareStatement> MysqlCpp::preparedStatementCreator(const std::string& sqlString)
{
    return prepare(sqlString);
}

std::shared_ptr<MysqlCppPrepareStatement> MysqlCpp::prepare(const std::string& sqlString)
{
    if (!check())
    {
        setLastError("mysql connection is not available");
        return std::shared_ptr<MysqlCppPrepareStatement>(new MysqlCppPrepareStatement(nullptr, sqlString));
    }
    try
    {
        std::shared_ptr<MysqlCppPrepareStatement> statement(new MysqlCppPrepareStatement(nullptr, sqlString));
        statement->m_session = m_session;
        statement->m_preparedStatement = m_con->prepareStatement(sqlString);
        setLastError(statement->isValid() ? "" : "prepare statement returned null");
        return statement;
    }
    catch (const sql::SQLException& e)
    {
        setLastError(MysqlCppHelper::exceptionText(e));
        return std::shared_ptr<MysqlCppPrepareStatement>(new MysqlCppPrepareStatement(nullptr, sqlString));
    }
    catch (...)
    {
        setLastError("prepare statement failed");
        return std::shared_ptr<MysqlCppPrepareStatement>(new MysqlCppPrepareStatement(nullptr, sqlString));
    }
}

std::shared_ptr<MysqlCppResultSet> MysqlCpp::execute(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit)
{
    return query(statement, isCommit);
}

std::shared_ptr<MysqlCppResultSet> MysqlCpp::query(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit)
{
    if (!checkStatement(statement))
    {
        return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
    }
    try
    {
        std::shared_ptr<MysqlCppResultSet> rows(new MysqlCppResultSet(nullptr));
        sql::PreparedStatement* prepared = statement->preparedStatement();
        if (prepared->execute())
        {
            rows->m_statement = statement;
            rows->m_resultSet = prepared->getResultSet();
        }
        if (isCommit && !commit())
        {
            return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
        }
        setLastError("");
        return rows;
    }
    catch (const sql::SQLException& e)
    {
        if (isCommit)
        {
            rollback();
        }
        setLastError(MysqlCppHelper::exceptionText(e));
        return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
    }
    catch (...)
    {
        if (isCommit)
        {
            rollback();
        }
        setLastError("query failed");
        return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
    }
}

MysqlCppExecuteResult MysqlCpp::executeUpdate(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit)
{
    MysqlCppExecuteResult result;
    if (!checkStatement(statement))
    {
        result.m_errorMessage = lastError();
        return result;
    }
    try
    {
        const int32_t affectedRows = statement->preparedStatement()->executeUpdate();
        const uint64_t lastInsertId = MysqlCppHelper::isLastInsertIdSql(statement->sql()) ? queryLastInsertId() : 0;
        if (isCommit && !commit())
        {
            result.m_errorMessage = lastError();
            return result;
        }
        result.m_affectedRows = affectedRows;
        result.m_lastInsertId = lastInsertId;
        result.m_success = true;
        setLastError("");
        return result;
    }
    catch (const sql::SQLException& e)
    {
        if (isCommit)
        {
            rollback();
        }
        result.m_errorMessage = MysqlCppHelper::exceptionText(e);
        setLastError(result.m_errorMessage);
        return result;
    }
    catch (...)
    {
        if (isCommit)
        {
            rollback();
        }
        result.m_errorMessage = "execute update failed";
        setLastError(result.m_errorMessage);
        return result;
    }
}

bool MysqlCpp::beginTransaction()
{
    if (!check())
    {
        setLastError("mysql connection is not available");
        return false;
    }
    try
    {
        ExecuteSessionCommand(*m_con, "SET SESSION autocommit=0");
        m_config.m_autoCommit = false;
        setLastError("");
        return true;
    }
    catch (const sql::SQLException& e)
    {
        setLastError(MysqlCppHelper::exceptionText(e));
        return false;
    }
    catch (...)
    {
        setLastError("begin transaction failed");
        return false;
    }
}

bool MysqlCpp::commit()
{
    if (!check())
    {
        setLastError("mysql connection is not available");
        return false;
    }
    try
    {
        ExecuteSessionCommand(*m_con, "COMMIT");
        setLastError("");
        return true;
    }
    catch (const sql::SQLException& e)
    {
        setLastError(MysqlCppHelper::exceptionText(e));
        return false;
    }
    catch (...)
    {
        setLastError("commit failed");
        return false;
    }
}

bool MysqlCpp::rollback()
{
    if (!check())
    {
        setLastError("mysql connection is not available");
        return false;
    }
    try
    {
        ExecuteSessionCommand(*m_con, "ROLLBACK");
        setLastError("");
        return true;
    }
    catch (const sql::SQLException& e)
    {
        setLastError(MysqlCppHelper::exceptionText(e));
        return false;
    }
    catch (...)
    {
        setLastError("rollback failed");
        return false;
    }
}

void MysqlCpp::importSql(const std::string& sqlPath,
                         const std::string& host,
                         const std::string& user,
                         const std::string& password,
                         const std::string& database)
{
#ifdef _WIN32
    const char* name = "mysql.exe";
#else
    const char* name = "mysql";
#endif
    const std::vector<std::string> arguments = {MysqlCppHelper::mysqlToolPath(name), "--protocol=TCP",
        "--port=" + CStringManager::toStringInt32(m_config.m_port), "--host=" + host, "--user=" + user,
        "--database=" + database};
    const int result = MysqlCppHelper::runTool(arguments, sqlPath, true, password);
    setLastError(result == 0 ? "" : "mysql import command failed, result=" + CStringManager::toStringInt32(result));
}

void MysqlCpp::exportSql(const std::string& sqlPath,
                         const std::string& host,
                         const std::string& user,
                         const std::string& password,
                         const std::string& exportData)
{
    exportSql(sqlPath, host, user, password, exportData, std::string());
}

void MysqlCpp::exportSql(const std::string& sqlPath, const std::string& host, const std::string& user,
    const std::string& password, const std::string& database, const std::string& table)
{
#ifdef _WIN32
    const char* name = "mysqldump.exe";
#else
    const char* name = "mysqldump";
#endif
    std::vector<std::string> arguments = {MysqlCppHelper::mysqlToolPath(name), "--protocol=TCP",
        "--port=" + CStringManager::toStringInt32(m_config.m_port), "--host=" + host, "--user=" + user,
        "--no-tablespaces", "--", database};
    if (!table.empty())
    {
        arguments.push_back(table);
    }
    const int result = MysqlCppHelper::runTool(arguments, sqlPath, false, password);
    setLastError(result == 0 ? "" : "mysql export command failed, result=" + CStringManager::toStringInt32(result));
}

void MysqlCpp::setAutoCommit(bool autoCommit)
{
    if (!check())
    {
        setLastError("mysql connection is not available");
        return;
    }
    try
    {
        ExecuteSessionCommand(*m_con, autoCommit ? "SET SESSION autocommit=1" : "SET SESSION autocommit=0");
        m_config.m_autoCommit = autoCommit;
        setLastError("");
    }
    catch (const sql::SQLException& e)
    {
        setLastError(MysqlCppHelper::exceptionText(e));
    }
    catch (...)
    {
        setLastError("set auto commit failed");
    }
}

bool MysqlCpp::check() const
{
    return m_isInit && m_driver != nullptr && m_con != nullptr;
}

bool MysqlCpp::checkStatement(const std::shared_ptr<MysqlCppPrepareStatement>& statement)
{
    if (!check())
    {
        setLastError("mysql connection is not available");
    }
    else if (!statement || !statement->isValid())
    {
        if (!statement || m_lastError.empty())
        {
            setLastError("invalid mysql statement");
        }
    }
    else if (statement->m_session.lock() != m_session)
    {
        setLastError("statement belongs to a different mysql connection");
    }
    else
    {
        return true;
    }
    return false;
}

void MysqlCpp::setLastError(const std::string& error)
{
    m_lastError = error;
}

uint64_t MysqlCpp::queryLastInsertId() const
{
    if (!check())
    {
        return 0;
    }
    try
    {
        std::unique_ptr<sql::Statement> statement(m_con->createStatement());
        if (statement.get() == nullptr)
        {
            return 0;
        }
        std::unique_ptr<sql::ResultSet> result(statement->executeQuery("SELECT LAST_INSERT_ID()"));
        if (result.get() != nullptr && result->next())
        {
            return result->getUInt64(1);
        }
    }
    catch (...)
    {
    }
    return 0;
}

// 使用示例开始。单独编译本文件时可启用下方的 main。
//#include <iostream>
//
//int main()
//{
//    // 使用专用账号连接到其演示数据库，只创建当前连接可见的临时订单表。
//    const char* host = std::getenv("MYSQLCPP_TEST_IP");
//    const char* port = std::getenv("MYSQLCPP_TEST_PORT");
//    const char* user = std::getenv("MYSQLCPP_TEST_ACCOUNT");
//    const char* password = std::getenv("MYSQLCPP_TEST_PASSWORD");
//    const char* database = std::getenv("MYSQLCPP_TEST_DB");
//    if (!user || !password || !database)
//    {
//        std::cerr << "Set MYSQLCPP_TEST_ACCOUNT, MYSQLCPP_TEST_PASSWORD and MYSQLCPP_TEST_DB.\n";
//        return 1;
//    }
//    MysqlCppConfig config;
//    config.m_ip = host ? host : "127.0.0.1";
//    config.m_port = port ? std::atoi(port) : 33841;
//    config.m_account = user;
//    config.m_password = password;
//    config.m_dbName = database;
//    MysqlCpp mysql;
//    if (!mysql.init() || !mysql.connect(config) || !mysql.isConnected() || !mysql.ping() ||
//        !mysql.selectDb(config.m_dbName))
//    {
//        std::cerr << mysql.lastError() << std::endl;
//        return 1;
//    }
//    mysql.setAutoCommit(false);
//    const auto created = mysql.executeUpdate(mysql.prepare(
//        "CREATE TEMPORARY TABLE demo_orders (id INT PRIMARY KEY, customer VARCHAR(64), quantity INT)"));
//    if (!created.m_success || !mysql.beginTransaction())
//    {
//        return 1;
//    }
//
//    const auto insert = mysql.preparedStatementCreator("INSERT INTO demo_orders VALUES (?, ?, ?)");
//    if (!insert || !insert->isValid())
//    {
//        return 1;
//    }
//    insert->setInt(0, 101);
//    insert->setString(1, "Ada");
//    insert->setInt(2, 3);
//    const auto written = mysql.executeUpdate(insert, false);
//    if (!written.m_success || written.m_affectedRows != 1 || !mysql.commit())
//    {
//        std::cerr << mysql.lastError() << std::endl;
//        mysql.rollback();
//        return 1;
//    }
//
//    const auto byId = mysql.prepare("SELECT customer, quantity FROM demo_orders WHERE id=?");
//    if (!byId || !byId->isValid())
//    {
//        return 1;
//    }
//    byId->setInt(0, 101);
//    const auto row = mysql.query(byId, false);
//    if (!row || !row->next())
//    {
//        return 1;
//    }
//    std::cout << "Order 101, customer " << row->getString("customer")
//        << ", quantity " << row->getInt("quantity") << '\n';
//    const auto totals = mysql.execute(mysql.prepare("SELECT SUM(quantity) FROM demo_orders"));
//    if (!totals || !totals->next())
//    {
//        return 1;
//    }
//    std::cout << "Total units " << totals->getInt(0) << '\n';
//    const auto dropped = mysql.executeUpdate(mysql.prepare("DROP TEMPORARY TABLE demo_orders"));
//    // 若中途返回，服务端也会在连接析构时自动释放该临时表。
//    return dropped.m_success ? 0 : 1;
//}
// 使用示例结束。
