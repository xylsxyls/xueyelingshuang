#include "MysqlCpp.h"
#include "MysqlCppExecution.h"
#include "MysqlCppHelper.h"
#include "MysqlCppPrepareStatement.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstdlib>
#include <memory>
#include <mutex>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "mysql-connector-cpp/cppconn/connection.h"
#include "mysql-connector-cpp/cppconn/driver.h"
#include "mysql-connector-cpp/cppconn/exception.h"
#include "mysql-connector-cpp/cppconn/prepared_statement.h"
#include "mysql-connector-cpp/cppconn/resultset.h"
#include "mysql-connector-cpp/cppconn/resultset_metadata.h"
#include "mysql-connector-cpp/cppconn/statement.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

// Connector/C++ 的驱动工厂共享进程级实例表，首次取驱动也需要串行。
static std::mutex g_driverMutex;

/** 执行当前会话的控制语句，服务端拒绝时由驱动抛出 SQL 异常
@param [in] connection 已连接的会话
@param [in] command 本文件构造的完整控制语句，同步执行
*/
static void ExecuteSessionCommand(sql::Connection& connection, const char* command)
{
    // Connector/C++ 1.1.13 的 Connection::commit、rollback 和 setAutoCommit 会忽略 C API 的错误返回值。
    std::unique_ptr<sql::Statement> statement(connection.createStatement());
    statement->execute(command);
}

/** 切换到指定库，将库名作为一个 MySQL 标识符发送
@param [in] connection 已建立的服务器会话
@param [in] database 原始库名，调用方已检查不含 NUL
*/
static void SelectSessionSchema(sql::Connection& connection, const std::string& database)
{
    std::string command("USE `");
    for (size_t i = 0; i < database.size(); ++i)
    {
        if (database[i] == '`')
        {
            command.push_back('`');
        }
        command.push_back(database[i]);
    }
    command.push_back('`');
    ExecuteSessionCommand(connection, command.c_str());
}

/** 关闭将被同一原生语句的下一次执行覆盖的游标
@param [in,out] cursor 本次已校验语句的活动结果弱引用
*/
static void ClosePreviousCursor(std::weak_ptr<MysqlCppResultSet>& cursor)
{
    auto previous = cursor.lock();
    cursor.reset();
    if (previous && !previous->close())
    {
        throw sql::SQLException("failed to consume the previous procedure result");
    }
}

MysqlCpp::MysqlCpp() :
m_driver(nullptr),
m_con(nullptr),
m_isInit(false)
{

}

MysqlCpp::~MysqlCpp()
{
    close();
}

bool MysqlCpp::configure(const MysqlCppConfig& config)
{
    if (m_isInit || !config.validConnection())
    {
        setLastError("configure requires an uninitialized object and valid connection parameters");
        return false;
    }
    m_config = config;
    m_lastError.clear();
    return true;
}

bool MysqlCpp::connect()
{
    return connect(m_config);
}

bool MysqlCpp::init()
{
    if (m_isInit)
    {
        return true;
    }

    try
    {
        std::lock_guard<std::mutex> guard(g_driverMutex);
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
    if (!config.validConnection())
    {
        setLastError("invalid TCP endpoint or embedded NUL in connection parameters");
        return false;
    }
    if (!m_isInit && !init())
    {
        return false;
    }
    close();
    m_config = config;
    try
    {
        sql::ConnectOptionsMap connectionProperties;
        MysqlCppHelper::connectionOptions(config, connectionProperties);
        m_con = m_driver->connect(connectionProperties);
        if (m_con == nullptr)
        {
            setLastError("mysql connect returned null");
            return false;
        }
        if (!config.m_dbName.empty())
        {
            SelectSessionSchema(*m_con, config.m_dbName);
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
    // 驱动的 reconnect 在 ping 成功时保留原事务；显式重连需要真正结束旧会话。
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
        SelectSessionSchema(*m_con, dbName);
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
    auto statement = std::make_shared<MysqlCppPrepareStatement>(currentExecution(), sqlString);
    setLastError(statement->lastError());
    return statement;
}

MysqlCppExecution MysqlCpp::currentExecution() const
{
    MysqlCppExecution execution = {m_con, m_session, nullptr, nullptr};
    return execution;
}

std::shared_ptr<MysqlCppResultSet> MysqlCpp::execute(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit)
{
    return query(statement, isCommit);
}

std::shared_ptr<MysqlCppResultSet> MysqlCpp::query(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit)
{
    MysqlCppExecution execution = currentExecution();
    if (!selectStatement(statement, execution))
    {
        return std::make_shared<MysqlCppResultSet>(execution, nullptr);
    }
    try
    {
        ClosePreviousCursor(*execution.m_cursor);
        auto rows = std::make_shared<MysqlCppResultSet>(execution, statement);
        if (isCommit && !commit())
        {
            return std::make_shared<MysqlCppResultSet>(execution, nullptr);
        }
        setLastError("");
        if (rows->isValid())
        {
            *execution.m_cursor = rows;
        }
        return rows;
    }
    catch (const sql::SQLException& e)
    {
        if (isCommit)
        {
            rollback();
        }
        setLastError(MysqlCppHelper::exceptionText(e));
        return std::make_shared<MysqlCppResultSet>(execution, nullptr);
    }
    catch (...)
    {
        if (isCommit)
        {
            rollback();
        }
        setLastError("query failed");
        return std::make_shared<MysqlCppResultSet>(execution, nullptr);
    }
}

MysqlCppExecuteResult MysqlCpp::executeUpdate(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit)
{
    MysqlCppExecuteResult result;
    MysqlCppExecution execution = currentExecution();
    if (!selectStatement(statement, execution))
    {
        result.m_errorMessage = lastError();
        return result;
    }
    try
    {
        // 驱动的 executeUpdate 不会消费返回行。静态已知有列时，在发送 SQL 前拒绝。
        sql::ResultSetMetaData* metadata = execution.m_statement->getMetaData();
        if (metadata && metadata->getColumnCount() != 0)
        {
            result.m_errorMessage = "executeUpdate requires a statement without result columns";
            setLastError(result.m_errorMessage);
            return result;
        }
        ClosePreviousCursor(*execution.m_cursor);
        const int32_t affectedRows = execution.m_statement->executeUpdate();
        if (affectedRows < 0)
        {
            // 动态返回行的过程或超出接口范围的行数：关闭原生语句以释放未读响应。
            statement->close();
            if (isCommit)
            {
                rollback();
            }
            result.m_errorMessage = "executeUpdate did not return a representable affected row count";
            setLastError(result.m_errorMessage);
            return result;
        }
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

bool MysqlCpp::beginScopedTransaction(MysqlCppExecution& execution)
{
    const bool started = beginTransaction();
    if (started)
    {
        execution.m_session = m_session;
    }
    else
    {
        execution.m_session.reset();
    }
    return started;
}

bool MysqlCpp::beginTransaction()
{
    setAutoCommit(false);
    return m_lastError.empty();
}

bool MysqlCpp::commit()
{
    return executeControl("COMMIT", "commit failed");
}

bool MysqlCpp::rollback()
{
    return executeControl("ROLLBACK", "rollback failed");
}

bool MysqlCpp::executeControl(const char* command, const char* failure)
{
    if (!check())
    {
        setLastError("mysql connection is not available");
        return false;
    }
    try
    {
        ExecuteSessionCommand(*m_con, command);
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
        setLastError(failure);
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
    const std::string address = MysqlCppHelper::toolHost(host);
    if (address.empty())
    {
        setLastError("invalid MySQL tool host");
        return;
    }
    const std::vector<std::string> arguments = {MysqlCppHelper::mysqlToolPath(name), "--protocol=TCP",
        "--port=" + CStringManager::toStringInt32(m_config.m_port), "--host=" + address, "--user=" + user,
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
    const std::string address = MysqlCppHelper::toolHost(host);
    if (address.empty())
    {
        setLastError("invalid MySQL tool host");
        return;
    }
    std::vector<std::string> arguments = {MysqlCppHelper::mysqlToolPath(name), "--protocol=TCP",
        "--port=" + CStringManager::toStringInt32(m_config.m_port), "--host=" + address, "--user=" + user,
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
    if (executeControl(autoCommit ? "SET SESSION autocommit=1" : "SET SESSION autocommit=0", "set auto commit failed"))
    {
        m_config.m_autoCommit = autoCommit;
    }
}

bool MysqlCpp::check() const
{
    return m_isInit && m_driver != nullptr && m_con != nullptr;
}

bool MysqlCpp::checkStatement(const std::shared_ptr<MysqlCppPrepareStatement>& statement)
{
    MysqlCppExecution execution = currentExecution();
    return selectStatement(statement, execution);
}

bool MysqlCpp::selectStatement(const std::shared_ptr<MysqlCppPrepareStatement>& statement, MysqlCppExecution& execution)
{
    if (!check())
    {
        setLastError("mysql connection is not available");
    }
    else if (statement && statement->selectForExecution(execution))
    {
        return true;
    }
    else if (statement && statement->isValid())
    {
        setLastError("statement belongs to a different mysql connection");
    }
    else
    {
        const std::string reason = statement ? statement->lastError() : std::string();
        setLastError(reason.empty() ? "invalid mysql statement" : reason);
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
//#include "MysqlCppConnectionPool.h"
//#include "MysqlCppTransaction.h"
//#include <cerrno>
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
//    char* end = nullptr;
//    errno = 0;
//    const long number = port ? std::strtol(port, &end, 10) : config.m_port;
//    if ((port && (end == port || *end != 0 || errno == ERANGE)) || number < 1 || number > 65535 ||
//        !config.setEndpoint(host ? host : config.m_ip, static_cast<int32_t>(number)) ||
//        !config.setCredentials(user, password) || !config.setDatabase(database))
//    {
//        return 1;
//    }
//    config.setTimeouts(3, 5, 7);
//    config.setAutoCommit(false);
//    config.setAutoReconnect(false);
//    MysqlCpp mysql;
//    if (!config.validConnection() || !mysql.configure(config) || !mysql.init() || !mysql.isInit() ||
//        !mysql.connect() || !mysql.isConnected() || !mysql.ping() ||
//        !mysql.selectDb(config.m_dbName))
//    {
//        std::cerr << mysql.lastError() << std::endl;
//        return 1;
//    }
//    mysql.setAutoCommit(false);
//    if (!mysql.lastError().empty())
//    {
//        std::cerr << mysql.lastError() << std::endl;
//        return 1;
//    }
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
//    if (!insert->isValid())
//    {
//        std::cerr << insert->lastError() << std::endl;
//        return 1;
//    }
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
//    if (!row || row->rowsCount() != 1 || row->columnCount() != 2 || !row->next())
//    {
//        return 1;
//    }
//    std::cout << "Order 101, customer " << row->getString("customer")
//        << ", quantity " << row->getInt("quantity") << '\n';
//    if (!row->close())
//    {
//        mysql.rollback();
//        return 1;
//    }
//    byId->close();
//    const auto totals = mysql.execute(mysql.prepare("SELECT SUM(quantity) FROM demo_orders"));
//    if (!totals || !totals->next())
//    {
//        return 1;
//    }
//    std::cout << "Total units " << totals->getInt(0) << '\n';
//    const auto dropped = mysql.executeUpdate(mysql.prepare("DROP TEMPORARY TABLE demo_orders"));
//    if (!dropped.m_success || !mysql.reconnect() || insert->isValid() || row->isValid())
//    {
//        return 1;
//    }
//    mysql.close();
//    mysql.uninit();
//    MysqlCppConnectionPool pool;
//    if (!config.setPoolLimits(0, 2))
//    {
//        return 1;
//    }
//    config.setPoolAcquireTimeout(1000);
//    if (!pool.init(config))
//    {
//        std::cerr << pool.lastError() << std::endl;
//        return 1;
//    }
//    {
//        const auto session = pool.acquire();
//        if (!session || pool.totalCount() != 1 || pool.availableCount() != 0)
//        {
//            return 1;
//        }
//        MysqlCppTransaction transaction(*session);
//        const auto values = session->query(session->prepare("SELECT 3 AS units"), false);
//        const auto records = values->toMapVector();
//        if (!transaction.isActive() || !values->isValid() || values->columnCount() != 1 ||
//            records.size() != 1 || records[0].count("units") != 1 || records[0].at("units") != "3" || !transaction.commit())
//        {
//            return 1;
//        }
//    }
//    pool.close();
//    return 0;
//}
// 使用示例结束。