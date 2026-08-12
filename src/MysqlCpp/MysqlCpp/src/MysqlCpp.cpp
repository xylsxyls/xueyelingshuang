#include "MysqlCpp.h"
#include "MysqlCppHelper.h"
#include "MysqlCppPrepareStatement.h"
#include "MysqlSqlString.h"
#include "CStringManager/CStringManagerAPI.h"
#include <cstdlib>
#include <memory>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif

#include "cppconn/connection.h"
#include "cppconn/driver.h"
#include "cppconn/exception.h"
#include "cppconn/prepared_statement.h"
#include "cppconn/resultset.h"
#include "cppconn/statement.h"

#ifdef _MSC_VER
#pragma warning(pop)
#endif

MysqlCpp::MysqlCpp() :
m_driver(nullptr),
m_con(nullptr),
m_isInit(false)
{

}

MysqlCpp::~MysqlCpp()
{

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
        m_con->setAutoCommit(config.m_autoCommit);
        if (!m_con->isValid())
        {
            close();
            setLastError("mysql connection is invalid");
            return false;
        }
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
    if (m_con != nullptr)
    {
        delete m_con;
        m_con = nullptr;
    }
}

bool MysqlCpp::reconnect()
{
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
                m_con->setAutoCommit(m_config.m_autoCommit);
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
    if (!check())
    {
        setLastError("mysql connection is not available");
        return false;
    }
    try
    {
        m_con->setSchema(dbName.c_str());
        m_con->setAutoCommit(autoCommit);
        m_config.m_dbName = dbName;
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
        return std::shared_ptr<MysqlCppPrepareStatement>(new MysqlCppPrepareStatement(m_con->prepareStatement(sqlString), sqlString));
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
    if (statement.get() != nullptr && MysqlCppHelper::isQuerySql(statement->sql()))
    {
        return query(statement, isCommit);
    }
    executeUpdate(statement, isCommit);
    return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
}

std::shared_ptr<MysqlCppResultSet> MysqlCpp::query(const std::shared_ptr<MysqlCppPrepareStatement>& statement, bool isCommit)
{
    if (!check() || statement.get() == nullptr || !statement->isValid())
    {
        if (!check())
        {
            setLastError("mysql connection is not available");
        }
        return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(nullptr));
    }
    try
    {
        sql::ResultSet* result = statement->preparedStatement()->executeQuery();
        if (isCommit)
        {
            commit();
        }
        setLastError("");
        return std::shared_ptr<MysqlCppResultSet>(new MysqlCppResultSet(result));
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
    if (!check() || statement.get() == nullptr || !statement->isValid())
    {
        if (!check())
        {
            setLastError("mysql connection is not available");
        }
        result.m_errorMessage = lastError().empty() ? "invalid mysql statement" : lastError();
        return result;
    }
    try
    {
        result.m_affectedRows = statement->preparedStatement()->executeUpdate();
        if (MysqlCppHelper::isLastInsertIdSql(statement->sql()))
        {
            result.m_lastInsertId = queryLastInsertId();
        }
        if (isCommit && !commit())
        {
            result.m_errorMessage = lastError();
            return result;
        }
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
        m_con->setAutoCommit(false);
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
        m_con->commit();
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
        m_con->rollback();
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
    int32_t result = std::system(MysqlSqlString::importString(sqlPath, host, user, password, database).c_str());
    setLastError(result == 0 ? "" : "mysql import command failed, result=" + CStringManager::toStringInt32(result));
}

void MysqlCpp::exportSql(const std::string& sqlPath,
                         const std::string& host,
                         const std::string& user,
                         const std::string& password,
                         const std::string& exportData)
{
    int32_t result = std::system(MysqlSqlString::exportString(sqlPath, host, user, password, exportData).c_str());
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
        m_con->setAutoCommit(autoCommit);
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

//#include "MysqlCppConnectionPool.h"
//#include "MysqlCppTransaction.h"
//#include <atomic>
//#include <cstdlib>
//#include <ctime>
//#include <functional>
//#include <iostream>
//#include <sstream>
//#include <thread>
//
//int main()
//{
//    int32_t totalCount = 0;
//    int32_t failCount = 0;
//    int32_t skipCount = 0;
//
//    auto check = [&totalCount, &failCount](bool condition, const std::string& name) -> bool
//    {
//        ++totalCount;
//        if (condition)
//        {
//            std::cout << "[PASS] " << name << std::endl;
//            return true;
//        }
//        ++failCount;
//        std::cout << "[FAIL] " << name << std::endl;
//        return false;
//    };
//
//    auto skip = [&skipCount](const std::string& name, const std::string& reason) -> void
//    {
//        ++skipCount;
//        std::cout << "[SKIP] " << name << " : " << reason << std::endl;
//    };
//
//    auto envString = [](const char* name, const std::string& defaultValue) -> std::string
//    {
//        const char* value = std::getenv(name);
//        if (value == nullptr || value[0] == '\0')
//        {
//            return defaultValue;
//        }
//        return value;
//    };
//
//    auto envInt32 = [&envString](const char* name, int32_t defaultValue) -> int32_t
//    {
//        std::string value = envString(name, "");
//        if (value.empty())
//        {
//            return defaultValue;
//        }
//        return static_cast<int32_t>(std::atoi(value.c_str()));
//    };
//
//    auto toStringInt32 = [](int32_t value) -> std::string
//    {
//        std::ostringstream oss;
//        oss << value;
//        return oss.str();
//    };
//
//    check(MysqlCppHelper::firstSqlWord("  select * from t") == "SELECT", "MysqlCppHelper first word whitespace");
//    check(MysqlCppHelper::firstSqlWord("-- comment\r\nshow tables") == "SHOW", "MysqlCppHelper first word line comment");
//    check(MysqlCppHelper::firstSqlWord("# comment\nexplain select 1") == "EXPLAIN", "MysqlCppHelper first word sharp comment");
//    check(MysqlCppHelper::firstSqlWord("/* block comment */ with c as (select 1) select * from c") == "WITH", "MysqlCppHelper first word block comment");
//    check(MysqlCppHelper::firstSqlWord("/* unfinished") == "", "MysqlCppHelper unfinished comment");
//    check(MysqlCppHelper::isQuerySql("select 1"), "MysqlCppHelper query select");
//    check(MysqlCppHelper::isQuerySql("/*x*/ show tables"), "MysqlCppHelper query show");
//    check(!MysqlCppHelper::isQuerySql("insert into t values(1)"), "MysqlCppHelper non query insert");
//    check(MysqlCppHelper::isLastInsertIdSql("insert into t values(1)"), "MysqlCppHelper last insert id insert");
//    check(MysqlCppHelper::isLastInsertIdSql(" replace into t values(1)"), "MysqlCppHelper last insert id replace");
//    check(!MysqlCppHelper::isLastInsertIdSql("update t set a=1"), "MysqlCppHelper no last insert id update");
//    check(MysqlCppHelper::normalizeTimeoutSeconds(3, 5) == 3, "MysqlCppHelper valid timeout");
//    check(MysqlCppHelper::normalizeTimeoutSeconds(0, 5) == 5, "MysqlCppHelper default timeout");
//    check(MysqlCppHelper::normalizeTimeoutSeconds(-1, 0) == 1, "MysqlCppHelper fallback timeout");
//    check(MysqlCppHelper::toDriverIndex(0) == 1, "MysqlCppHelper driver index first");
//    check(MysqlCppHelper::toDriverIndex(UINT32_MAX) == 0, "MysqlCppHelper driver index overflow");
//
//    MysqlCppConfig defaultConfig;
//    check(defaultConfig.m_ip == "127.0.0.1", "MysqlCppConfig default ip");
//    check(defaultConfig.m_port == 3306, "MysqlCppConfig default port");
//    check(!defaultConfig.m_autoCommit, "MysqlCppConfig default auto commit");
//    check(defaultConfig.m_autoReconnect, "MysqlCppConfig default auto reconnect");
//    check(defaultConfig.m_connectTimeoutSeconds > 0, "MysqlCppConfig default connect timeout");
//    check(defaultConfig.m_readTimeoutSeconds > 0, "MysqlCppConfig default read timeout");
//    check(defaultConfig.m_writeTimeoutSeconds > 0, "MysqlCppConfig default write timeout");
//
//    MysqlCppExecuteResult defaultExecuteResult;
//    check(!defaultExecuteResult.m_success, "MysqlCppExecuteResult default success");
//    check(defaultExecuteResult.m_affectedRows == 0, "MysqlCppExecuteResult default affected rows");
//    check(defaultExecuteResult.m_lastInsertId == 0, "MysqlCppExecuteResult default insert id");
//
//    std::string importCommand = MysqlSqlString::importString("D:\\test data\\a.sql", "127.0.0.1", "root", "123456", "cook");
//    std::string exportCommand = MysqlSqlString::exportString("D:\\test data\\b.sql", "127.0.0.1", "root", "123456", "cook recipe");
//    check(importCommand.find("mysql") != std::string::npos && importCommand.find("cook") != std::string::npos, "MysqlSqlString import string");
//    check(exportCommand.find("mysqldump") != std::string::npos && exportCommand.find("cook recipe") != std::string::npos, "MysqlSqlString export string");
//    std::string quotedImportCommand = MysqlSqlString::importString("D:\\test data\\a.sql", "127.0.0.1", "root user", "pass word", "cook db");
//    check(quotedImportCommand.find("root user") != std::string::npos &&
//        quotedImportCommand.find("pass word") != std::string::npos &&
//        quotedImportCommand.find("cook db") != std::string::npos, "MysqlSqlString quote command arguments");
//
//    MysqlCpp localMysql;
//    check(!localMysql.isInit(), "MysqlCpp default not init");
//    check(localMysql.init(), "MysqlCpp init");
//    check(localMysql.isInit(), "MysqlCpp is init");
//    check(!localMysql.ping(), "MysqlCpp no connection ping false");
//    check(!localMysql.isConnected(), "MysqlCpp no connection isConnected false");
//    check(!localMysql.beginTransaction(), "MysqlCpp begin transaction without connection");
//    check(!localMysql.commit(), "MysqlCpp commit without connection");
//    check(!localMysql.rollback(), "MysqlCpp rollback without connection");
//    check(!localMysql.lastError().empty(), "MysqlCpp no connection error text");
//
//    std::shared_ptr<MysqlCppPrepareStatement> invalidStatement = localMysql.prepare("SELECT 1");
//    check(invalidStatement.get() != nullptr, "MysqlCpp invalid prepare returns object");
//    check(invalidStatement.get() != nullptr && !invalidStatement->isValid(), "MysqlCpp invalid prepare not valid");
//    check(invalidStatement.get() != nullptr && invalidStatement->sql() == "SELECT 1", "MysqlCpp invalid prepare keeps sql");
//    if (invalidStatement.get() != nullptr)
//    {
//        invalidStatement->setBigInt(0, "1");
//        invalidStatement->setBoolean(0, true);
//        invalidStatement->setDateTime(0, "2026-01-01 00:00:00");
//        invalidStatement->setDouble(0, 1.25);
//        invalidStatement->setInt(0, 1);
//        invalidStatement->setUInt(0, 2);
//        invalidStatement->setInt64(0, 3);
//        invalidStatement->setUInt64(0, 4);
//        invalidStatement->setNull(0, 0);
//        invalidStatement->setString(0, "abc");
//        invalidStatement->setInt(UINT32_MAX, 1);
//        invalidStatement->setString(UINT32_MAX, "overflow");
//        check(true, "MysqlCpp invalid statement setters are safe");
//    }
//
//    std::shared_ptr<MysqlCppResultSet> invalidResult = localMysql.query(invalidStatement);
//    check(invalidResult.get() != nullptr && !invalidResult->isValid(), "MysqlCpp invalid query result");
//    check(invalidResult.get() != nullptr && !invalidResult->next(), "MysqlCpp invalid result next");
//    check(invalidResult.get() != nullptr && !invalidResult->previous(), "MysqlCpp invalid result previous");
//    check(invalidResult.get() != nullptr && invalidResult->columnCount() == 0, "MysqlCpp invalid result column count");
//    check(invalidResult.get() != nullptr && invalidResult->rowsCount() == 0, "MysqlCpp invalid result rows count");
//    check(invalidResult.get() != nullptr && invalidResult->columnNames().empty(), "MysqlCpp invalid result column names");
//    check(invalidResult.get() != nullptr && invalidResult->toVector().empty(), "MysqlCpp invalid result to vector");
//    check(invalidResult.get() != nullptr && invalidResult->toMapVector().empty(), "MysqlCpp invalid result to map vector");
//    check(invalidResult.get() != nullptr && invalidResult->getString(0).empty(), "MysqlCpp invalid result string by index");
//    check(invalidResult.get() != nullptr && invalidResult->getString("name").empty(), "MysqlCpp invalid result string by name");
//    check(invalidResult.get() != nullptr && invalidResult->getInt(0) == 0, "MysqlCpp invalid result int by index");
//    check(invalidResult.get() != nullptr && invalidResult->getInt("id") == 0, "MysqlCpp invalid result int by name");
//    check(invalidResult.get() != nullptr && invalidResult->getUInt(0) == 0, "MysqlCpp invalid result uint by index");
//    check(invalidResult.get() != nullptr && invalidResult->getUInt("id") == 0, "MysqlCpp invalid result uint by name");
//    check(invalidResult.get() != nullptr && invalidResult->getInt64(0) == 0, "MysqlCpp invalid result int64 by index");
//    check(invalidResult.get() != nullptr && invalidResult->getInt64("id") == 0, "MysqlCpp invalid result int64 by name");
//    check(invalidResult.get() != nullptr && invalidResult->getUInt64(0) == 0, "MysqlCpp invalid result uint64 by index");
//    check(invalidResult.get() != nullptr && invalidResult->getUInt64("id") == 0, "MysqlCpp invalid result uint64 by name");
//    check(invalidResult.get() != nullptr && invalidResult->getBoolean(0) == false, "MysqlCpp invalid result bool by index");
//    check(invalidResult.get() != nullptr && invalidResult->getBoolean("flag") == false, "MysqlCpp invalid result bool by name");
//    check(invalidResult.get() != nullptr && invalidResult->getDouble(0) == 0, "MysqlCpp invalid result double by index");
//    check(invalidResult.get() != nullptr && invalidResult->getDouble("score") == 0, "MysqlCpp invalid result double by name");
//    check(invalidResult.get() != nullptr && invalidResult->getBlob(0) == nullptr, "MysqlCpp invalid result blob by index");
//    check(invalidResult.get() != nullptr && invalidResult->getBlob("data") == nullptr, "MysqlCpp invalid result blob by name");
//    check(invalidResult.get() != nullptr && invalidResult->getString(UINT32_MAX).empty(), "MysqlCpp invalid result string overflow index");
//    check(invalidResult.get() != nullptr && invalidResult->getInt(UINT32_MAX) == 0, "MysqlCpp invalid result int overflow index");
//    check(invalidResult.get() != nullptr && invalidResult->getBlob(UINT32_MAX) == nullptr, "MysqlCpp invalid result blob overflow index");
//
//    std::shared_ptr<MysqlCppPrepareStatement> nullStatement;
//    MysqlCppExecuteResult nullUpdate = localMysql.executeUpdate(nullStatement);
//    check(!nullUpdate.m_success && !nullUpdate.m_errorMessage.empty(), "MysqlCpp null statement execute update");
//    std::shared_ptr<MysqlCppResultSet> nullExecute = localMysql.execute(nullStatement);
//    check(nullExecute.get() != nullptr && !nullExecute->isValid(), "MysqlCpp null statement execute");
//    MysqlCppExecuteResult invalidUpdate = localMysql.executeUpdate(invalidStatement);
//    check(!invalidUpdate.m_success, "MysqlCpp invalid execute update");
//    std::shared_ptr<MysqlCppResultSet> invalidExecute = localMysql.execute(invalidStatement);
//    check(invalidExecute.get() != nullptr && !invalidExecute->isValid(), "MysqlCpp invalid execute");
//    localMysql.setAutoCommit(true);
//    check(!localMysql.selectDb("mysqlcpp_none", true), "MysqlCpp select db without connection");
//    localMysql.close();
//    localMysql.uninit();
//    check(!localMysql.isInit(), "MysqlCpp uninit");
//
//    MysqlCppConnectionPool emptyPool;
//    check(!emptyPool.init(defaultConfig, 0), "MysqlCppConnectionPool reject zero size");
//    check(emptyPool.acquire(1).get() == nullptr, "MysqlCppConnectionPool acquire without init");
//    check(emptyPool.availableCount() == 0, "MysqlCppConnectionPool available without init");
//    check(emptyPool.totalCount() == 0, "MysqlCppConnectionPool total without init");
//    emptyPool.close();
//    emptyPool.uninit();
//    emptyPool.uninit();
//    check(true, "MysqlCppConnectionPool duplicate uninit without init safe");
//
//    MysqlCppConfig mysqlConfig;
//    mysqlConfig.m_ip = envString("MYSQLCPP_TEST_IP", "127.0.0.1");
//    mysqlConfig.m_port = envInt32("MYSQLCPP_TEST_PORT", 3306);
//    mysqlConfig.m_account = envString("MYSQLCPP_TEST_ACCOUNT", "root");
//    mysqlConfig.m_password = envString("MYSQLCPP_TEST_PASSWORD", "");
//    mysqlConfig.m_dbName = envString("MYSQLCPP_TEST_DB", "test");
//    mysqlConfig.m_autoCommit = false;
//    mysqlConfig.m_autoReconnect = true;
//    mysqlConfig.m_connectTimeoutSeconds = envInt32("MYSQLCPP_TEST_CONNECT_TIMEOUT_SECONDS", 3);
//    mysqlConfig.m_readTimeoutSeconds = envInt32("MYSQLCPP_TEST_READ_TIMEOUT_SECONDS", 5);
//    mysqlConfig.m_writeTimeoutSeconds = envInt32("MYSQLCPP_TEST_WRITE_TIMEOUT_SECONDS", 5);
//
//    MysqlCpp mysql;
//    if (!mysql.connect(mysqlConfig))
//    {
//        skip("MysqlCpp real mysql tests", "set MYSQLCPP_TEST_IP/MYSQLCPP_TEST_ACCOUNT/MYSQLCPP_TEST_PASSWORD/MYSQLCPP_TEST_DB to enable, last error: " + mysql.lastError());
//        std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << " skipped=" << skipCount << std::endl;
//        return failCount == 0 ? 0 : 1;
//    }
//
//    check(mysql.isInit(), "MysqlCpp real connection is init");
//    check(mysql.isConnected(), "MysqlCpp real connection is connected");
//    check(mysql.ping(), "MysqlCpp real ping");
//    check(mysql.selectDb(mysqlConfig.m_dbName, false), "MysqlCpp real select db");
//    mysql.setAutoCommit(false);
//
//    std::string tableName = "mysqlcpp_test_" + toStringInt32(static_cast<int32_t>(std::time(nullptr) % 1000000));
//
//    auto executeSql = [&mysql](const std::string& sqlText) -> MysqlCppExecuteResult
//    {
//        return mysql.executeUpdate(mysql.prepare(sqlText));
//    };
//
//    auto executeSqlNoCommit = [&mysql](const std::string& sqlText) -> MysqlCppExecuteResult
//    {
//        return mysql.executeUpdate(mysql.prepare(sqlText), false);
//    };
//
//    check(executeSql("DROP TABLE IF EXISTS " + tableName).m_success, "MysqlCpp drop old table");
//    check(executeSql("CREATE TABLE " + tableName + " (id INT PRIMARY KEY, name VARCHAR(64), score INT, flag TINYINT, amount DOUBLE)").m_success, "MysqlCpp create table");
//
//    std::shared_ptr<MysqlCppPrepareStatement> insertStatement = mysql.prepare("INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (?, ?, ?, ?, ?)");
//    check(insertStatement.get() != nullptr && insertStatement->isValid(), "MysqlCpp prepare insert");
//    if (insertStatement.get() != nullptr)
//    {
//        insertStatement->setInt(0, 1);
//        insertStatement->setString(1, "alpha");
//        insertStatement->setInt(2, 95);
//        insertStatement->setBoolean(3, true);
//        insertStatement->setDouble(4, 12.5);
//    }
//    MysqlCppExecuteResult insertResult = mysql.executeUpdate(insertStatement);
//    check(insertResult.m_success && insertResult.m_affectedRows == 1, "MysqlCpp execute insert");
//
//    std::shared_ptr<MysqlCppPrepareStatement> selectStatement = mysql.prepare("SELECT id, name, score, flag, amount FROM " + tableName + " WHERE id=?");
//    check(selectStatement.get() != nullptr && selectStatement->isValid(), "MysqlCpp prepare select");
//    if (selectStatement.get() != nullptr)
//    {
//        selectStatement->setInt(0, 1);
//    }
//    std::shared_ptr<MysqlCppResultSet> selectResult = mysql.query(selectStatement);
//    bool rowOk = false;
//    if (selectResult.get() != nullptr && selectResult->next())
//    {
//        rowOk = selectResult->getInt(0) == 1 &&
//            selectResult->getString("name") == "alpha" &&
//            selectResult->getInt("score") == 95 &&
//            selectResult->getBoolean("flag") &&
//            selectResult->getDouble("amount") > 12.4 &&
//            selectResult->getDouble("amount") < 12.6;
//    }
//    check(rowOk, "MysqlCpp query row getters");
//    check(selectResult.get() != nullptr && selectResult->columnCount() == 5, "MysqlCpp result column count");
//    check(selectResult.get() != nullptr && selectResult->columnNames().size() == 5, "MysqlCpp result column names");
//
//    std::shared_ptr<MysqlCppPrepareStatement> vectorStatement = mysql.prepare("SELECT id, name FROM " + tableName + " ORDER BY id");
//    std::shared_ptr<MysqlCppResultSet> vectorResult = mysql.query(vectorStatement);
//    std::vector<std::vector<std::string>> vectorRows = vectorResult.get() == nullptr ? std::vector<std::vector<std::string>>() : vectorResult->toVector();
//    check(vectorRows.size() == 1 && vectorRows[0].size() == 2, "MysqlCpp result to vector");
//
//    std::shared_ptr<MysqlCppResultSet> mapResult = mysql.query(mysql.prepare("SELECT id, name FROM " + tableName + " ORDER BY id"));
//    std::vector<std::map<std::string, std::string>> mapRows = mapResult.get() == nullptr ? std::vector<std::map<std::string, std::string>>() : mapResult->toMapVector();
//    check(mapRows.size() == 1 && mapRows[0]["name"] == "alpha", "MysqlCpp result to map vector");
//
//    std::shared_ptr<MysqlCppPrepareStatement> updateStatement = mysql.prepare("UPDATE " + tableName + " SET score=? WHERE id=?");
//    if (updateStatement.get() != nullptr)
//    {
//        updateStatement->setInt(0, 96);
//        updateStatement->setInt(1, 1);
//    }
//    MysqlCppExecuteResult updateResult = mysql.executeUpdate(updateStatement);
//    check(updateResult.m_success && updateResult.m_affectedRows == 1, "MysqlCpp execute update");
//    check(updateResult.m_lastInsertId == 0, "MysqlCpp update does not reuse last insert id");
//
//    std::shared_ptr<MysqlCppResultSet> executeSelectResult = mysql.execute(mysql.prepare("SELECT id FROM " + tableName + " WHERE id=1"));
//    check(executeSelectResult.get() != nullptr && executeSelectResult->isValid() && executeSelectResult->next(), "MysqlCpp execute select dispatcher");
//    std::shared_ptr<MysqlCppResultSet> executeUpdateResult = mysql.execute(mysql.prepare("UPDATE " + tableName + " SET score=97 WHERE id=1"));
//    check(executeUpdateResult.get() != nullptr && !executeUpdateResult->isValid(), "MysqlCpp execute update dispatcher");
//
//    MysqlCppTransaction transaction(mysql);
//    check(transaction.isActive(), "MysqlCppTransaction begin");
//    check(executeSqlNoCommit("INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (2, 'beta', 88, 0, 2.5)").m_success, "MysqlCppTransaction insert before commit");
//    check(transaction.commit(), "MysqlCppTransaction commit");
//    std::shared_ptr<MysqlCppResultSet> commitResult = mysql.query(mysql.prepare("SELECT id FROM " + tableName + " WHERE id=2"));
//    check(commitResult.get() != nullptr && commitResult->next(), "MysqlCppTransaction commit visible");
//
//    MysqlCppTransaction rollbackTransaction(mysql);
//    check(rollbackTransaction.isActive(), "MysqlCppTransaction rollback begin");
//    check(executeSqlNoCommit("INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (3, 'gamma', 77, 1, 3.5)").m_success, "MysqlCppTransaction insert before rollback");
//    rollbackTransaction.rollback();
//    std::shared_ptr<MysqlCppResultSet> rollbackResult = mysql.query(mysql.prepare("SELECT id FROM " + tableName + " WHERE id=3"));
//    check(rollbackResult.get() != nullptr && !rollbackResult->next(), "MysqlCppTransaction rollback invisible");
//
//    MysqlCppConnectionPool pool;
//    bool poolInit = pool.init(mysqlConfig, 4);
//    check(poolInit, "MysqlCppConnectionPool real init");
//    if (poolInit)
//    {
//        check(pool.totalCount() == 4, "MysqlCppConnectionPool total count");
//        check(pool.availableCount() == 4, "MysqlCppConnectionPool available count");
//        check(pool.init(mysqlConfig, 4), "MysqlCppConnectionPool double init no-op");
//        check(pool.totalCount() == 4 && pool.availableCount() == 4, "MysqlCppConnectionPool double init keep old pool");
//        {
//            std::shared_ptr<MysqlCpp> pooledConnection = pool.acquire(1000);
//            check(pooledConnection.get() != nullptr && pooledConnection->ping(), "MysqlCppConnectionPool acquire");
//            check(pool.availableCount() == 3, "MysqlCppConnectionPool borrowed count");
//            check(pool.init(mysqlConfig, 4), "MysqlCppConnectionPool double init with borrowed no-op");
//            check(pool.totalCount() == 4 && pool.availableCount() == 3, "MysqlCppConnectionPool double init keep borrowed state");
//        }
//        check(pool.availableCount() == 4, "MysqlCppConnectionPool release by shared ptr");
//
//        const int32_t threadCount = 6;
//        const int32_t loopCount = 20;
//        std::atomic<int32_t> threadFailures(0);
//        std::vector<std::thread> threads;
//        for (int32_t i = 0; i < threadCount; ++i)
//        {
//            threads.push_back(std::thread([&pool, &threadFailures, &tableName, i, loopCount]() -> void
//            {
//                for (int32_t j = 0; j < loopCount; ++j)
//                {
//                    std::shared_ptr<MysqlCpp> connection = pool.acquire(3000);
//                    if (connection.get() == nullptr)
//                    {
//                        ++threadFailures;
//                        continue;
//                    }
//                    int32_t id = 10000 + i * 100 + j;
//                    std::ostringstream oss;
//                    oss << "INSERT INTO " << tableName << " (id, name, score, flag, amount) VALUES (?, ?, ?, ?, ?)";
//                    std::shared_ptr<MysqlCppPrepareStatement> threadInsert = connection->prepare(oss.str());
//                    if (threadInsert.get() == nullptr || !threadInsert->isValid())
//                    {
//                        ++threadFailures;
//                        continue;
//                    }
//                    threadInsert->setInt(0, id);
//                    threadInsert->setString(1, "thread");
//                    threadInsert->setInt(2, j);
//                    threadInsert->setBoolean(3, true);
//                    threadInsert->setDouble(4, 1.0);
//                    if (!connection->executeUpdate(threadInsert).m_success)
//                    {
//                        ++threadFailures;
//                        continue;
//                    }
//
//                    std::ostringstream selectOss;
//                    selectOss << "SELECT id FROM " << tableName << " WHERE id=?";
//                    std::shared_ptr<MysqlCppPrepareStatement> threadSelect = connection->prepare(selectOss.str());
//                    if (threadSelect.get() == nullptr || !threadSelect->isValid())
//                    {
//                        ++threadFailures;
//                        continue;
//                    }
//                    threadSelect->setInt(0, id);
//                    std::shared_ptr<MysqlCppResultSet> threadResult = connection->query(threadSelect);
//                    if (threadResult.get() == nullptr || !threadResult->next() || threadResult->getInt(0) != id)
//                    {
//                        ++threadFailures;
//                    }
//                }
//            }));
//        }
//
//        for (size_t i = 0; i < threads.size(); ++i)
//        {
//            threads[i].join();
//        }
//        check(threadFailures.load() == 0, "MysqlCppConnectionPool multi thread pressure");
//        pool.uninit();
//        pool.uninit();
//        check(pool.totalCount() == 0 && pool.availableCount() == 0, "MysqlCppConnectionPool duplicate uninit safe");
//    }
//
//    std::shared_ptr<MysqlCpp> borrowedAfterPoolDestroy;
//    {
//        MysqlCppConnectionPool scopedPool;
//        bool scopedPoolInit = scopedPool.init(mysqlConfig, 1);
//        check(scopedPoolInit, "MysqlCppConnectionPool scoped init");
//        if (scopedPoolInit)
//        {
//            borrowedAfterPoolDestroy = scopedPool.acquire(1000);
//            check(borrowedAfterPoolDestroy.get() != nullptr, "MysqlCppConnectionPool scoped acquire");
//        }
//    }
//    if (borrowedAfterPoolDestroy.get() != nullptr)
//    {
//        check(borrowedAfterPoolDestroy->ping(), "MysqlCppConnectionPool borrowed connection after pool destroy");
//        borrowedAfterPoolDestroy.reset();
//        check(true, "MysqlCppConnectionPool borrowed connection reset after pool destroy");
//    }
//
//    check(executeSql("DROP TABLE IF EXISTS " + tableName).m_success, "MysqlCpp cleanup test table");
//    mysql.close();
//    mysql.uninit();
//    check(!mysql.isInit(), "MysqlCpp real uninit");
//
//    std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << " skipped=" << skipCount << std::endl;
//    return failCount == 0 ? 0 : 1;
//}
//