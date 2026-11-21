#include "PlatformTest/PlatformTestAPI.h"
#include "MysqlCppTest.h"
#include <cstring>
#include <cstdio>
#include <fstream>
#include "MysqlCpp/MysqlCppAPI.h"
#include "MysqlCpp/MysqlCppHelper.h"
#include <chrono>

#include "mysql-connector-cpp/cppconn/exception.h"
#include <atomic>
#include <cstdlib>
#include <ctime>
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <thread>

/** 对本库的一项固定规模操作连续采样，结果用于同平台的改动前后比较。 */
static void RunBenchmark()
{
    const std::string sql(" /* caller */ -- comment\n select value FROM user_info");
    LibraryBenchmark::run("MysqlCpp", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t index = 0; index < count; ++index)
        {
            checksum += MysqlCppHelper::isQuerySql(sql) ? 1 : 0;
        }
        return checksum;
    });
}

/** 复用预编译语句，测量绑定、只读查询和结果读取
@param [in] mysql 已连接并核实版本的 MySQL 客户端，计时期间由本测试独占
*/
static void RunDatabaseBenchmark(MysqlCpp& mysql)
{
    const std::shared_ptr<MysqlCppPrepareStatement> statement = mysql.prepare("SELECT CAST(? AS SIGNED)");
    if (!statement || !statement->isValid())
    {
        throw std::runtime_error("Cannot prepare MySQL benchmark query");
    }
    LibraryBenchmark::run("MysqlCpp.PreparedSelect", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const int32_t expected = static_cast<int32_t>(i) + 1;
            statement->setInt(0, expected);
            const std::shared_ptr<MysqlCppResultSet> result = mysql.query(statement, false);
            if (!result || !result->next() || result->getInt(0) != expected || result->next())
            {
                throw std::runtime_error("Unexpected MySQL benchmark result");
            }
            checksum += expected;
        }
        return checksum;
    });
}

/** 在独立进程中核对离线接口或连接到真实的隔离服务。
@param [in] offline 执行离线断言
@param [in] online 执行数据库场景
@param [in] benchmark 连接成功后执行数据库计时
@return 成功返回 0，断言失败返回 1
*/
static int RunChecks(bool offline, bool online, bool benchmark)
{
    int32_t totalCount = 0;
    int32_t failCount = 0;
    int32_t skipCount = 0;

    auto check = [&totalCount, &failCount](bool condition, const std::string& name) -> bool
    {
        ++totalCount;
        if (condition)
        {
            std::cout << "[PASS] " << name << std::endl;
            return true;
        }
        ++failCount;
        std::cout << "[FAIL] " << name << std::endl;
        return false;
    };

    auto envString = [](const char* name, const std::string& defaultValue) -> std::string
    {
        const char* value = std::getenv(name);
        if (value == nullptr || value[0] == '\0')
        {
            return defaultValue;
        }
        return value;
    };

    auto envInt32 = [&envString](const char* name, int32_t defaultValue) -> int32_t
    {
        std::string value = envString(name, "");
        if (value.empty())
        {
            return defaultValue;
        }
        return static_cast<int32_t>(std::atoi(value.c_str()));
    };

    auto toStringInt32 = [](int32_t value) -> std::string
    {
        std::ostringstream oss;
        oss << value;
        return oss.str();
    };

    if (offline)
    {
        check(MysqlCppHelper::firstSqlWord("  select * from t") == "SELECT", "MysqlCppHelper first word whitespace");
        check(MysqlCppHelper::firstSqlWord("-- comment\r\nshow tables") == "SHOW", "MysqlCppHelper first word line comment");
        check(MysqlCppHelper::firstSqlWord("# comment\nexplain select 1") == "EXPLAIN", "MysqlCppHelper first word sharp comment");
        check(MysqlCppHelper::firstSqlWord("/* block comment */ with c as (select 1) select * from c") == "WITH", "MysqlCppHelper first word block comment");
        check(MysqlCppHelper::firstSqlWord("/* unfinished") == "", "MysqlCppHelper unfinished comment");
        check(MysqlCppHelper::isQuerySql("select 1"), "MysqlCppHelper query select");
        check(MysqlCppHelper::isQuerySql("/*x*/ show tables"), "MysqlCppHelper query show");
        check(!MysqlCppHelper::isQuerySql("SeLeCtEd field") && MysqlCppHelper::isQuerySql("dEsCrIbE t"),
            "MysqlCppHelper complete ASCII keyword match");
        check(!MysqlCppHelper::isQuerySql("insert into t values(1)"), "MysqlCppHelper non query insert");
        check(MysqlCppHelper::isLastInsertIdSql("insert into t values(1)"), "MysqlCppHelper last insert id insert");
        check(MysqlCppHelper::isLastInsertIdSql(" replace into t values(1)"), "MysqlCppHelper last insert id replace");
        check(!MysqlCppHelper::isLastInsertIdSql("update t set a=1"), "MysqlCppHelper no last insert id update");
        check(MysqlCppHelper::normalizeTimeoutSeconds(3, 5) == 3, "MysqlCppHelper valid timeout");
        check(MysqlCppHelper::normalizeTimeoutSeconds(0, 5) == 5, "MysqlCppHelper default timeout");
        check(MysqlCppHelper::normalizeTimeoutSeconds(-1, 0) == 1, "MysqlCppHelper fallback timeout");
        check(MysqlCppHelper::toDriverIndex(0) == 1, "MysqlCppHelper driver index first");
        check(MysqlCppHelper::toDriverIndex(UINT32_MAX) == 0, "MysqlCppHelper driver index overflow");

        const sql::SQLException duplicate("duplicate row", "23000", 1062);
        check(MysqlCppHelper::exceptionText(duplicate) == "duplicate row code=1062", "MysqlCppHelper driver error text");
#ifdef _WIN32
        check(MysqlCppHelper::quoteCommandText("one \"two\"") == "\"one \\\"two\\\"\"", "MysqlCppHelper quote Windows argument");
        const char separator = '\\';
#else
        check(MysqlCppHelper::quoteCommandText("one 'two'") == "'one '\\''two'\\'''", "MysqlCppHelper quote POSIX argument");
        const char separator = '/';
#endif
        std::string expectedToolPath = envString("MYSQL_ROOT", "");
        if (!expectedToolPath.empty())
        {
            const char last = expectedToolPath[expectedToolPath.size() - 1];
            if (last != '/' && last != '\\')
            {
                expectedToolPath += separator;
            }
            expectedToolPath += "bin";
            expectedToolPath += separator;
        }
        expectedToolPath += "mysqldump";
        check(MysqlCppHelper::mysqlToolPath("mysqldump") == expectedToolPath, "MysqlCppHelper client tool path");

        MysqlCppConfig defaultConfig;
        check(defaultConfig.m_ip == "127.0.0.1", "MysqlCppConfig default ip");
        check(defaultConfig.m_port == 3306, "MysqlCppConfig default port");
        check(!defaultConfig.m_autoCommit, "MysqlCppConfig default auto commit");
        check(defaultConfig.m_autoReconnect, "MysqlCppConfig default auto reconnect");
        check(defaultConfig.m_connectTimeoutSeconds == 5, "MysqlCppConfig default connect timeout");
        check(defaultConfig.m_readTimeoutSeconds == 10, "MysqlCppConfig default read timeout");
        check(defaultConfig.m_writeTimeoutSeconds == 10, "MysqlCppConfig default write timeout");

        MysqlCppExecuteResult defaultExecuteResult;
        check(!defaultExecuteResult.m_success, "MysqlCppExecuteResult default success");
        check(defaultExecuteResult.m_affectedRows == 0, "MysqlCppExecuteResult default affected rows");
        check(defaultExecuteResult.m_lastInsertId == 0, "MysqlCppExecuteResult default insert id");

        std::string importCommand = MysqlSqlString::importString("D:\\test data\\a.sql", "127.0.0.1", "root", "123456", "cook");
        std::string exportCommand = MysqlSqlString::exportString("D:\\test data\\b.sql", "127.0.0.1", "root", "123456", "cook recipe");
        check(importCommand.find("mysql") != std::string::npos && importCommand.find("cook") != std::string::npos, "MysqlSqlString import string");
        check(exportCommand.find("mysqldump") != std::string::npos && exportCommand.find("cook recipe") != std::string::npos, "MysqlSqlString export string");
        std::string quotedImportCommand = MysqlSqlString::importString("D:\\test data\\a.sql", "127.0.0.1", "root user", "pass word", "cook db");
        check(MysqlSqlString::exportString("out.sql", "127.0.0.1", "db", "pass", std::string("d\0b", 3)).empty(),
            "command preview rejects embedded NUL");
#ifdef _WIN32
        check(MysqlSqlString::importString("in.sql", "127.0.0.1", "db", "100% real", "db").empty(),
            "command preview refuses ambiguous Windows substitution");
#endif
        check(quotedImportCommand.find("root user") != std::string::npos &&
            quotedImportCommand.find("pass word") != std::string::npos &&
            quotedImportCommand.find("cook db") != std::string::npos, "MysqlSqlString quote command arguments");

        MysqlCpp localMysql;
        check(!localMysql.isInit(), "MysqlCpp default not init");
        check(localMysql.init(), "MysqlCpp init");
        check(localMysql.isInit(), "MysqlCpp is init");
        {
            MysqlCpp scopedMysql;
            check(scopedMysql.init(), "MysqlCpp destructor scoped init");
        }
        check(localMysql.isInit() && !localMysql.ping(), "MysqlCpp destructor uninit safe");
        check(!localMysql.ping(), "MysqlCpp no connection ping false");
        check(!localMysql.isConnected(), "MysqlCpp no connection isConnected false");
        check(!localMysql.beginTransaction(), "MysqlCpp begin transaction without connection");
        check(!localMysql.commit(), "MysqlCpp commit without connection");
        check(!localMysql.rollback(), "MysqlCpp rollback without connection");
        check(!localMysql.lastError().empty(), "MysqlCpp no connection error text");

        std::shared_ptr<MysqlCppPrepareStatement> invalidStatement = localMysql.prepare("SELECT 1");
        check(invalidStatement.get() != nullptr, "MysqlCpp invalid prepare returns object");
        check(invalidStatement.get() != nullptr && !invalidStatement->isValid(), "MysqlCpp invalid prepare not valid");
        check(invalidStatement.get() != nullptr && invalidStatement->sql() == "SELECT 1", "MysqlCpp invalid prepare keeps sql");
        if (invalidStatement.get() != nullptr)
        {
            invalidStatement->setBigInt(0, "1");
            invalidStatement->setBoolean(0, true);
            invalidStatement->setDateTime(0, "2026-01-01 00:00:00");
            invalidStatement->setDouble(0, 1.25);
            invalidStatement->setInt(0, 1);
            invalidStatement->setUInt(0, 2);
            invalidStatement->setInt64(0, 3);
            invalidStatement->setUInt64(0, 4);
            invalidStatement->setNull(0, 0);
            invalidStatement->setString(0, "abc");
            invalidStatement->setInt(UINT32_MAX, 1);
            invalidStatement->setString(UINT32_MAX, "overflow");
            check(!invalidStatement->isValid(), "MysqlCpp invalid statement setters are safe");
        }

        std::shared_ptr<MysqlCppResultSet> invalidResult = localMysql.query(invalidStatement);
        check(invalidResult.get() != nullptr && !invalidResult->isValid(), "MysqlCpp invalid query result");
        check(invalidResult.get() != nullptr && !invalidResult->next(), "MysqlCpp invalid result next");
        check(invalidResult.get() != nullptr && !invalidResult->previous(), "MysqlCpp invalid result previous");
        check(invalidResult.get() != nullptr && invalidResult->columnCount() == 0, "MysqlCpp invalid result column count");
        check(invalidResult.get() != nullptr && invalidResult->rowsCount() == 0, "MysqlCpp invalid result rows count");
        check(invalidResult.get() != nullptr && invalidResult->columnNames().empty(), "MysqlCpp invalid result column names");
        check(invalidResult.get() != nullptr && invalidResult->toVector().empty(), "MysqlCpp invalid result to vector");
        check(invalidResult.get() != nullptr && invalidResult->toMapVector().empty(), "MysqlCpp invalid result to map vector");
        check(invalidResult.get() != nullptr && invalidResult->getString(0).empty(), "MysqlCpp invalid result string by index");
        check(invalidResult.get() != nullptr && invalidResult->getString("name").empty(), "MysqlCpp invalid result string by name");
        check(invalidResult.get() != nullptr && invalidResult->getInt(0) == 0, "MysqlCpp invalid result int by index");
        check(invalidResult.get() != nullptr && invalidResult->getInt("id") == 0, "MysqlCpp invalid result int by name");
        check(invalidResult.get() != nullptr && invalidResult->getUInt(0) == 0, "MysqlCpp invalid result uint by index");
        check(invalidResult.get() != nullptr && invalidResult->getUInt("id") == 0, "MysqlCpp invalid result uint by name");
        check(invalidResult.get() != nullptr && invalidResult->getInt64(0) == 0, "MysqlCpp invalid result int64 by index");
        check(invalidResult.get() != nullptr && invalidResult->getInt64("id") == 0, "MysqlCpp invalid result int64 by name");
        check(invalidResult.get() != nullptr && invalidResult->getUInt64(0) == 0, "MysqlCpp invalid result uint64 by index");
        check(invalidResult.get() != nullptr && invalidResult->getUInt64("id") == 0, "MysqlCpp invalid result uint64 by name");
        check(invalidResult.get() != nullptr && invalidResult->getBoolean(0) == false, "MysqlCpp invalid result bool by index");
        check(invalidResult.get() != nullptr && invalidResult->getBoolean("flag") == false, "MysqlCpp invalid result bool by name");
        check(invalidResult.get() != nullptr && invalidResult->getDouble(0) == 0, "MysqlCpp invalid result double by index");
        check(invalidResult.get() != nullptr && invalidResult->getDouble("score") == 0, "MysqlCpp invalid result double by name");
        check(invalidResult.get() != nullptr && invalidResult->getBlob(0) == nullptr, "MysqlCpp invalid result blob by index");
        check(invalidResult.get() != nullptr && invalidResult->getBlob("data") == nullptr, "MysqlCpp invalid result blob by name");
        check(invalidResult.get() != nullptr && invalidResult->getString(UINT32_MAX).empty(), "MysqlCpp invalid result string overflow index");
        check(invalidResult.get() != nullptr && invalidResult->getInt(UINT32_MAX) == 0, "MysqlCpp invalid result int overflow index");
        check(invalidResult.get() != nullptr && invalidResult->getBlob(UINT32_MAX) == nullptr, "MysqlCpp invalid result blob overflow index");

        std::shared_ptr<MysqlCppPrepareStatement> nullStatement;
        MysqlCppExecuteResult nullUpdate = localMysql.executeUpdate(nullStatement);
        check(!nullUpdate.m_success && !nullUpdate.m_errorMessage.empty(), "MysqlCpp null statement execute update");
        std::shared_ptr<MysqlCppResultSet> nullExecute = localMysql.execute(nullStatement);
        check(nullExecute.get() != nullptr && !nullExecute->isValid(), "MysqlCpp null statement execute");
        MysqlCppExecuteResult invalidUpdate = localMysql.executeUpdate(invalidStatement);
        check(!invalidUpdate.m_success, "MysqlCpp invalid execute update");
        std::shared_ptr<MysqlCppResultSet> invalidExecute = localMysql.execute(invalidStatement);
        check(invalidExecute.get() != nullptr && !invalidExecute->isValid(), "MysqlCpp invalid execute");
        localMysql.setAutoCommit(true);
        check(!localMysql.selectDb("mysqlcpp_none", true), "MysqlCpp select db without connection");
        localMysql.close();
        localMysql.uninit();
        check(!localMysql.isInit(), "MysqlCpp uninit");

        MysqlCppConnectionPool emptyPool;
        check(!emptyPool.init(defaultConfig, 0), "MysqlCppConnectionPool reject zero size");
        check(emptyPool.acquire(1).get() == nullptr, "MysqlCppConnectionPool acquire without init");
        check(emptyPool.availableCount() == 0, "MysqlCppConnectionPool available without init");
        check(emptyPool.totalCount() == 0, "MysqlCppConnectionPool total without init");
        emptyPool.close();
        emptyPool.uninit();
        emptyPool.uninit();
        check(emptyPool.totalCount() == 0 && emptyPool.availableCount() == 0, "MysqlCppConnectionPool duplicate uninit without init safe");
    }
    if (!online) { return failCount == 0 ? 0 : 1; }

    MysqlCppConfig mysqlConfig;
    mysqlConfig.m_ip = envString("MYSQLCPP_TEST_IP", "127.0.0.1");
    mysqlConfig.m_port = envInt32("MYSQLCPP_TEST_PORT", 33841);
    mysqlConfig.m_account = envString("MYSQLCPP_TEST_ACCOUNT", "root");
    mysqlConfig.m_password = envString("MYSQLCPP_TEST_PASSWORD", "");
    mysqlConfig.m_dbName = envString("MYSQLCPP_TEST_DB", "test");
    mysqlConfig.m_autoCommit = false;
    mysqlConfig.m_autoReconnect = true;
    mysqlConfig.m_connectTimeoutSeconds = envInt32("MYSQLCPP_TEST_CONNECT_TIMEOUT_SECONDS", 3);
    mysqlConfig.m_readTimeoutSeconds = envInt32("MYSQLCPP_TEST_READ_TIMEOUT_SECONDS", 5);
    mysqlConfig.m_writeTimeoutSeconds = envInt32("MYSQLCPP_TEST_WRITE_TIMEOUT_SECONDS", 5);

    MysqlCpp mysql;
    if (!mysql.connect(mysqlConfig))
    {
        check(false, "MysqlCpp test instance connection failed: " + mysql.lastError());
        std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << " skipped=" << skipCount << std::endl;
        return 1;
    }

    const std::shared_ptr<MysqlCppResultSet> identity = mysql.query(mysql.prepare("SELECT VERSION()"));
    const std::string serverVersion = identity && identity->next() ? identity->getString(0) : std::string();
    if (!check(serverVersion == "8.4.13" || serverVersion.compare(0, 7, "8.4.13-") == 0,
        "MysqlCpp requires MySQL server 8.4.13, observed " + serverVersion))
    {
        return 1;
    }

    if (benchmark)
    {
        try
        {
            RunDatabaseBenchmark(mysql);
        }
        catch (const std::exception& error)
        {
            std::cerr << error.what() << std::endl;
            return 1;
        }
        return failCount == 0 ? 0 : 1;
    }
    check(mysql.isInit(), "MysqlCpp real connection is init");
    check(mysql.isConnected(), "MysqlCpp real connection is connected");
    {
        const std::string suffix("\0suffix", 7);
        const auto prepared = mysql.prepare("SELECT 17");
        const bool refused = !mysql.selectDb(std::string("information_schema") + suffix, true) && !mysql.lastError().empty();
        auto row = mysql.query(mysql.prepare("SELECT DATABASE(), @@session.autocommit"), false);
        check(refused && prepared->isValid() && row->next() && row->getString(0) == mysqlConfig.m_dbName &&
            row->getInt(1) == (mysqlConfig.m_autoCommit ? 1 : 0), "embedded NUL schema selection preserves schema and autocommit");
        if (!refused) { mysql.selectDb(mysqlConfig.m_dbName, mysqlConfig.m_autoCommit); }
        std::string MysqlCppConfig::*fields[] = {&MysqlCppConfig::m_ip, &MysqlCppConfig::m_account,
            &MysqlCppConfig::m_password, &MysqlCppConfig::m_dbName};
        const char* labels[] = {"host", "account", "password", "database"};
        for (size_t i = 0; i < sizeof(fields) / sizeof(fields[0]); ++i)
        {
            MysqlCpp session;
            const bool connected = session.connect(mysqlConfig);
            const auto original = session.prepare("SELECT 17");
            MysqlCppConfig invalid = mysqlConfig;
            (invalid.*fields[i]) += suffix;
            const bool rejected = !session.connect(invalid) && !session.lastError().empty();
            const auto stillConnected = session.query(original, false);
            check(connected && rejected && original->isValid() && stillConnected->next() && stillConnected->getInt(0) == 17,
                std::string("reject embedded NUL in ") + labels[i] + " before replacing active session");
        }
    }
    check(mysql.ping(), "MysqlCpp real ping");
    {
        MysqlCpp second;
        check(second.connect(mysqlConfig), "second SQL session opens");
        auto query = mysql.prepare("SELECT CONNECTION_ID()");
        auto write = mysql.prepare("SET @mysqlcpp_cross_session=7");
        check(!second.query(query)->isValid() && !second.lastError().empty(), "foreign query rejected by receiving session");
        check(!second.execute(query)->isValid() && !second.lastError().empty(), "foreign generic statement rejected");
        check(!second.executeUpdate(write).m_success && !second.lastError().empty(), "foreign update rejected");
        auto mark = mysql.query(mysql.prepare("SELECT @mysqlcpp_cross_session IS NULL"));
        check(mark->next() && mark->getInt(0) == 1, "foreign write never reached owning session");
        check(!second.query(std::shared_ptr<MysqlCppPrepareStatement>())->isValid() && !second.lastError().empty(),
            "null query records an error");
        auto ok = second.prepare("SELECT 1");
        check(ok->isValid() && second.lastError().empty(), "successful prepare clears preceding error");
        const std::string bytes("a\0b\xff", 4);
        std::istringstream stream(bytes);
        auto createBlob = mysql.prepare("CREATE TEMPORARY TABLE mysqlcpp_blob_roundtrip (payload BLOB)");
        check(mysql.executeUpdate(createBlob).m_success, "create session-local binary column");
        auto blob = mysql.prepare("INSERT INTO mysqlcpp_blob_roundtrip (payload) VALUES (?)");
        blob->setBlob(0, &stream);
        check(mysql.executeUpdate(blob).m_affectedRows == 1, "stream parameter inserts one BLOB");
        auto row = mysql.query(mysql.prepare("SELECT OCTET_LENGTH(payload), HEX(payload), payload AS raw_value FROM mysqlcpp_blob_roundtrip"));
        check(row->next() && row->getUInt(0) == bytes.size() && row->getString(1) == "610062FF" &&
            row->getString(2) == bytes, "prepared blob keeps exact length embedded zero and high byte");
        std::unique_ptr<std::istream> byIndex(row->getBlob(2));
        std::unique_ptr<std::istream> byName(row->getBlob("raw_value"));
        std::ostringstream indexValue;
        std::ostringstream namedValue;
        if (byIndex && byName)
        {
            indexValue << byIndex->rdbuf();
            namedValue << byName->rdbuf();
        }
        check(indexValue.str() == bytes && namedValue.str() == bytes && !row->next(),
            "BLOB stream getters by index and name preserve every byte");
        check(mysql.executeUpdate(mysql.prepare("DROP TEMPORARY TABLE mysqlcpp_blob_roundtrip")).m_success,
            "release session-local binary column");
        auto previous = second.prepare("SELECT 1");
        auto previousRows = second.query(previous);
        {
            MysqlCppTransaction old(second);
            check(old.isActive(), "transaction initially tied to live session");
            second.close();
            check(!old.isActive() && !previous->isValid() && !previousRows->isValid() && !previousRows->next(),
                "closing session invalidates transaction statement and result");
            check(second.connect(mysqlConfig) && !old.commit() && !second.query(previous)->isValid(),
                "old objects never execute on next session");
            previous->setInt(0, 2);
        }
        auto replaced = second.prepare("SELECT 1");
        check(second.reconnect() && !replaced->isValid(), "explicit reconnect invalidates prepared statements");
        auto values = mysql.execute(mysql.prepare("TABLE information_schema.engines LIMIT 1"));
        check(values->isValid() && values->next(), "generic execute discovers TABLE result from server");
    }
    check(mysql.selectDb(mysqlConfig.m_dbName, false), "MysqlCpp real select db");
    mysql.setAutoCommit(false);

    std::ostringstream uniqueName;
    uniqueName << "mysqlcpp_test_" << std::chrono::high_resolution_clock::now().time_since_epoch().count();
    const std::string tableName = uniqueName.str();

    auto executeSql = [&mysql](const std::string& sqlText) -> MysqlCppExecuteResult
    {
        return mysql.executeUpdate(mysql.prepare(sqlText));
    };

    auto executeSqlNoCommit = [&mysql](const std::string& sqlText) -> MysqlCppExecuteResult
    {
        return mysql.executeUpdate(mysql.prepare(sqlText), false);
    };

    if (!check(executeSql("CREATE TABLE " + tableName + " (id INT PRIMARY KEY, name VARCHAR(64), score INT, flag TINYINT, amount DOUBLE)").m_success, "MysqlCpp create table")) { return 1; }

    // 8.4 的预处理协议允许 CALL，但不支持直接准备 XA。经独立 CLI 建立并清理三个短过程。
    const std::string xaPath = tableName + " xa.sql";
    const auto installXa = [&](bool create) -> bool
    {
        std::ofstream sqlFile(xaPath.c_str(), std::ios::binary | std::ios::trunc);
        const char* actions[] = {"START", "END", "ROLLBACK"};
        for (size_t i = 0; i < 3; ++i)
        {
            const std::string procedure = tableName + "_" + actions[i];
            if (create) { sqlFile << "CREATE PROCEDURE " << procedure << "() XA " << actions[i] << " '" << tableName << "';\n"; }
            else { sqlFile << "DROP PROCEDURE IF EXISTS " << procedure << ";\n"; }
        }
        sqlFile.close();
        if (!sqlFile) { return false; }
        mysql.importSql(xaPath, mysqlConfig.m_ip, mysqlConfig.m_account, mysqlConfig.m_password, mysqlConfig.m_dbName);
        return mysql.lastError().empty();
    };
    const bool xaInstalled = installXa(true);
    const bool xaStarted = xaInstalled && executeSqlNoCommit("CALL " + tableName + "_START()").m_success;
    check(xaStarted, "start isolated XA branch through prepared CALL");
    bool pendingInsert = false;
    if (xaStarted)
    {
        MysqlCppTransaction scope(mysql);
        const bool scopeStarted = scope.isActive();
        scope.rollback();
        check(scopeStarted && scope.isActive() && mysql.lastError().find("1399") != std::string::npos,
            "failed scoped rollback keeps transaction active");
        const bool commitRejected = !mysql.commit() && mysql.lastError().find("1399") != std::string::npos;
        const bool rollbackRejected = !mysql.rollback() && mysql.lastError().find("1399") != std::string::npos;
        check(commitRejected && rollbackRejected, "transaction control propagates XA commit and rollback errors");
        mysql.setAutoCommit(true);
        const bool enableRejected = !mysql.lastError().empty();
        const auto session = mysql.query(mysql.prepare("SELECT @@session.autocommit"), false);
        check(enableRejected && session && session->next() && session->getInt(0) == 0,
            "failed autocommit update preserves actual session mode");
        const bool selectRejected = !mysql.selectDb("information_schema", true) && !mysql.lastError().empty();
        const auto schema = mysql.query(mysql.prepare("SELECT DATABASE()"), false);
        check(selectRejected && schema && schema->next() && schema->getString(0) == "information_schema" &&
            mysql.selectDb(mysqlConfig.m_dbName, false), "partial schema update retains server-selected database");
        const auto rows = mysql.query(mysql.prepare("SELECT 1"), true);
        check(rows && !rows->isValid() && mysql.lastError().find("1399") != std::string::npos,
            "query propagates server rejection of commit");
        const auto update = mysql.executeUpdate(mysql.prepare("INSERT INTO " + tableName +
            " (id, name) VALUES (LAST_INSERT_ID(778), 'rejected')"), true);
        const auto serverId = mysql.query(mysql.prepare("SELECT LAST_INSERT_ID()"), false);
        check(serverId && serverId->next() && serverId->getInt(0) == 778 &&
            !update.m_success && update.m_errorMessage.find("1399") != std::string::npos &&
            update.m_affectedRows == 0 && update.m_lastInsertId == 0,
            "failed update commit returns empty row count and insert ID");
        const bool ended = executeSqlNoCommit("CALL " + tableName + "_END()").m_success;
        const bool rolledBack = executeSqlNoCommit("CALL " + tableName + "_ROLLBACK()").m_success;
        check(ended && rolledBack, "end and roll back isolated XA branch");
        pendingInsert = ended && rolledBack &&
            executeSqlNoCommit("INSERT INTO " + tableName + " (id, name) VALUES (-777, 'scope')").m_success;
    }
    const auto scoped = mysql.query(mysql.prepare("SELECT COUNT(*) FROM " + tableName + " WHERE id=-777"), false);
    check(pendingInsert && scoped && scoped->next() && scoped->getInt(0) == 0,
        "scoped destructor retries rollback after a server rejection");
    const bool removedXa = installXa(false);
    check(std::remove(xaPath.c_str()) == 0 && removedXa, "remove XA test procedures and script");

    std::shared_ptr<MysqlCppPrepareStatement> insertStatement = mysql.prepare("INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (?, ?, ?, ?, ?)");
    check(insertStatement.get() != nullptr && insertStatement->isValid(), "MysqlCpp prepare insert");
    if (insertStatement.get() != nullptr)
    {
        insertStatement->setInt(0, 1);
        insertStatement->setString(1, "alpha");
        insertStatement->setInt(2, 95);
        insertStatement->setBoolean(3, true);
        insertStatement->setDouble(4, 12.5);
    }
    MysqlCppExecuteResult insertResult = mysql.executeUpdate(insertStatement);
    check(insertResult.m_success && insertResult.m_affectedRows == 1, "MysqlCpp execute insert");

    std::shared_ptr<MysqlCppPrepareStatement> selectStatement = mysql.prepare("SELECT id, name, score, flag, amount FROM " + tableName + " WHERE id=?");
    check(selectStatement.get() != nullptr && selectStatement->isValid(), "MysqlCpp prepare select");
    if (selectStatement.get() != nullptr)
    {
        selectStatement->setInt(0, 1);
    }
    std::shared_ptr<MysqlCppResultSet> selectResult = mysql.query(selectStatement);
    bool rowOk = false;
    if (selectResult.get() != nullptr && selectResult->next())
    {
        rowOk = selectResult->getInt(0) == 1 &&
            selectResult->getString("name") == "alpha" &&
            selectResult->getInt("score") == 95 &&
            selectResult->getBoolean("flag") &&
            selectResult->getDouble("amount") > 12.4 &&
            selectResult->getDouble("amount") < 12.6;
    }
    check(rowOk, "MysqlCpp query row getters");
    check(selectResult.get() != nullptr && selectResult->columnCount() == 5, "MysqlCpp result column count");
    check(selectResult.get() != nullptr && selectResult->columnNames().size() == 5, "MysqlCpp result column names");

    std::shared_ptr<MysqlCppPrepareStatement> vectorStatement = mysql.prepare("SELECT id, name FROM " + tableName + " ORDER BY id");
    std::shared_ptr<MysqlCppResultSet> vectorResult = mysql.query(vectorStatement);
    std::vector<std::vector<std::string>> vectorRows = vectorResult.get() == nullptr ? std::vector<std::vector<std::string>>() : vectorResult->toVector();
    check(vectorRows.size() == 1 && vectorRows[0].size() == 2, "MysqlCpp result to vector");

    std::shared_ptr<MysqlCppResultSet> mapResult = mysql.query(mysql.prepare("SELECT id, name FROM " + tableName + " ORDER BY id"));
    std::vector<std::map<std::string, std::string>> mapRows = mapResult.get() == nullptr ? std::vector<std::map<std::string, std::string>>() : mapResult->toMapVector();
    check(mapRows.size() == 1 && mapRows[0]["name"] == "alpha", "MysqlCpp result to map vector");

    std::shared_ptr<MysqlCppPrepareStatement> updateStatement = mysql.prepare("UPDATE " + tableName + " SET score=? WHERE id=?");
    if (updateStatement.get() != nullptr)
    {
        updateStatement->setInt(0, 96);
        updateStatement->setInt(1, 1);
    }
    MysqlCppExecuteResult updateResult = mysql.executeUpdate(updateStatement);
    check(updateResult.m_success && updateResult.m_affectedRows == 1, "MysqlCpp execute update");
    check(updateResult.m_lastInsertId == 0, "MysqlCpp update does not reuse last insert id");

    std::shared_ptr<MysqlCppResultSet> executeSelectResult = mysql.execute(mysql.prepare("SELECT id FROM " + tableName + " WHERE id=1"));
    check(executeSelectResult.get() != nullptr && executeSelectResult->isValid() && executeSelectResult->next(), "MysqlCpp execute select dispatcher");
    std::shared_ptr<MysqlCppResultSet> executeUpdateResult = mysql.execute(mysql.prepare("UPDATE " + tableName + " SET score=97 WHERE id=1"));
    check(executeUpdateResult.get() != nullptr && !executeUpdateResult->isValid(), "MysqlCpp execute update dispatcher");

    MysqlCppTransaction transaction(mysql);
    check(transaction.isActive(), "MysqlCppTransaction begin");
    check(executeSqlNoCommit("INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (2, 'beta', 88, 0, 2.5)").m_success, "MysqlCppTransaction insert before commit");
    check(transaction.commit(), "MysqlCppTransaction commit");
    std::shared_ptr<MysqlCppResultSet> commitResult = mysql.query(mysql.prepare("SELECT id FROM " + tableName + " WHERE id=2"));
    check(commitResult.get() != nullptr && commitResult->next(), "MysqlCppTransaction commit visible");

    MysqlCppTransaction rollbackTransaction(mysql);
    check(rollbackTransaction.isActive(), "MysqlCppTransaction rollback begin");
    check(executeSqlNoCommit("INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (3, 'gamma', 77, 1, 3.5)").m_success, "MysqlCppTransaction insert before rollback");
    rollbackTransaction.rollback();
    std::shared_ptr<MysqlCppResultSet> rollbackResult = mysql.query(mysql.prepare("SELECT id FROM " + tableName + " WHERE id=3"));
    check(rollbackResult.get() != nullptr && !rollbackResult->next(), "MysqlCppTransaction rollback invisible");

    {
        MysqlCppConfig limits = mysqlConfig;
        MysqlCppConnectionPool elastic;
        check(limits.m_poolMinConnections == 1 && limits.m_poolMaxConnections == 1, "pool default bounds");
        limits.m_poolMinConnections = -1;
        check(!elastic.init(limits), "pool negative minimum");
        limits.m_poolMinConnections = 0;
        limits.m_poolMaxConnections = -2;
        check(!elastic.init(limits), "pool invalid negative maximum");
        limits.m_poolMaxConnections = 0;
        check(!elastic.init(limits), "pool zero maximum");
        limits.m_poolMinConnections = 3;
        limits.m_poolMaxConnections = 2;
        check(!elastic.init(limits), "pool maximum below minimum");
        limits.m_poolMinConnections = 0;
        check(elastic.init(limits) && elastic.totalCount() == 0, "pool lazy initialization");
        auto first = elastic.acquire(0);
        auto second = elastic.acquire(0);
        check(first && second && first != second && elastic.totalCount() == 2, "pool growth to limit");
        check(!elastic.acquire(0), "pool limit immediate timeout");
        first.reset();
        second.reset();
        check(elastic.totalCount() == 0 && elastic.availableCount() == 0, "pool zero idle retention");
        elastic.close();
        limits.m_poolMinConnections = 1;
        limits.m_poolMaxConnections = -1;
        check(elastic.init(limits) && elastic.availableCount() == 1, "pool unbounded warm start");
        first = elastic.acquire();
        second = elastic.acquire();
        auto third = elastic.acquire();
        check(first && second && third && elastic.totalCount() == 3, "pool unbounded growth");
        first.reset();
        second.reset();
        third.reset();
        check(elastic.totalCount() == 1 && elastic.availableCount() == 1, "pool retains configured minimum");
        limits.m_poolMaxConnections = 1;
        for (int i = 0; i < 8; ++i)
        {
            std::unique_ptr<MysqlCppConnectionPool> owner(new MysqlCppConnectionPool);
            auto opened = owner->init(limits);
            auto borrowed = owner->acquire(0);
            check(opened && borrowed && borrowed->ping(), "pool borrowed before racing destruction");
            std::atomic<bool> start(false);
            std::thread returning([&]() {
                while (!start.load())
                {
                    std::this_thread::yield();
                }
                borrowed.reset();
            });
            start.store(true);
            owner.reset();
            returning.join();
            check(!borrowed && !owner, "pool concurrent return and destruction");
        }
        for (int i = 0; i < 2; ++i)
        {
            limits.m_port = i == 0 ? 0 : 65536;
            MysqlCpp invalid;
            check(!invalid.connect(limits) && !invalid.lastError().empty(), "reject port outside TCP range");
        }
    }

    {
        MysqlCppConnectionPool closing;
        check(closing.init(mysqlConfig, 1), "fixed pool starts a new generation");
        auto held = closing.acquire(0);
        std::promise<void> ready;
        auto entered = ready.get_future();
        auto pending = std::async(std::launch::async, [&]() {
            ready.set_value();
            return closing.acquire(-1);
        });
        entered.wait();
        check(held && pending.wait_for(std::chrono::milliseconds(20)) == std::future_status::timeout,
            "indefinite acquire waits while fixed pool is full");
        closing.close();
        held.reset();
        const bool reopened = closing.init(mysqlConfig, 1);
        check(!pending.get(), "closing wakes old generation without lending a session");
        check(reopened && closing.acquire(0) != nullptr,
            "new generation accepts new borrowers");
    }

    MysqlCppConnectionPool pool;
    bool poolInit = pool.init(mysqlConfig, 4);
    check(poolInit, "MysqlCppConnectionPool real init");
    if (poolInit)
    {
        check(pool.totalCount() == 4, "MysqlCppConnectionPool total count");
        check(pool.availableCount() == 4, "MysqlCppConnectionPool available count");
        check(pool.init(mysqlConfig, 4), "MysqlCppConnectionPool double init no-op");
        check(pool.totalCount() == 4 && pool.availableCount() == 4, "MysqlCppConnectionPool double init keep old pool");
        {
            std::shared_ptr<MysqlCpp> pooledConnection = pool.acquire(1000);
            check(pooledConnection.get() != nullptr && pooledConnection->ping(), "MysqlCppConnectionPool acquire");
            check(pool.availableCount() == 3, "MysqlCppConnectionPool borrowed count");
            check(pool.init(mysqlConfig, 4), "MysqlCppConnectionPool double init with borrowed no-op");
            check(pool.totalCount() == 4 && pool.availableCount() == 3, "MysqlCppConnectionPool double init keep borrowed state");
        }
        check(pool.availableCount() == 4, "MysqlCppConnectionPool release by shared ptr");

        const int32_t threadCount = 6;
        const int32_t loopCount = 20;
        std::atomic<int32_t> threadFailures(0);
        std::vector<std::thread> threads;
        for (int32_t i = 0; i < threadCount; ++i)
        {
            threads.push_back(std::thread([&pool, &threadFailures, &tableName, i, loopCount]() -> void
            {
                for (int32_t j = 0; j < loopCount; ++j)
                {
                    std::shared_ptr<MysqlCpp> connection = pool.acquire(3000);
                    if (connection.get() == nullptr)
                    {
                        ++threadFailures;
                        continue;
                    }
                    int32_t id = 10000 + i * 100 + j;
                    std::ostringstream oss;
                    oss << "INSERT INTO " << tableName << " (id, name, score, flag, amount) VALUES (?, ?, ?, ?, ?)";
                    std::shared_ptr<MysqlCppPrepareStatement> threadInsert = connection->prepare(oss.str());
                    if (threadInsert.get() == nullptr || !threadInsert->isValid())
                    {
                        ++threadFailures;
                        continue;
                    }
                    threadInsert->setInt(0, id);
                    threadInsert->setString(1, "thread");
                    threadInsert->setInt(2, j);
                    threadInsert->setBoolean(3, true);
                    threadInsert->setDouble(4, 1.0);
                    if (!connection->executeUpdate(threadInsert).m_success)
                    {
                        ++threadFailures;
                        continue;
                    }

                    std::ostringstream selectOss;
                    selectOss << "SELECT id FROM " << tableName << " WHERE id=?";
                    std::shared_ptr<MysqlCppPrepareStatement> threadSelect = connection->prepare(selectOss.str());
                    if (threadSelect.get() == nullptr || !threadSelect->isValid())
                    {
                        ++threadFailures;
                        continue;
                    }
                    threadSelect->setInt(0, id);
                    std::shared_ptr<MysqlCppResultSet> threadResult = connection->query(threadSelect);
                    if (threadResult.get() == nullptr || !threadResult->next() || threadResult->getInt(0) != id)
                    {
                        ++threadFailures;
                    }
                }
            }));
        }

        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        check(threadFailures.load() == 0, "MysqlCppConnectionPool multi thread pressure");
        pool.uninit();
        pool.uninit();
        check(pool.totalCount() == 0 && pool.availableCount() == 0, "MysqlCppConnectionPool duplicate uninit safe");
    }

    std::shared_ptr<MysqlCpp> borrowedAfterPoolDestroy;
    {
        MysqlCppConnectionPool scopedPool;
        bool scopedPoolInit = scopedPool.init(mysqlConfig, 1);
        check(scopedPoolInit, "MysqlCppConnectionPool scoped init");
        if (scopedPoolInit)
        {
            borrowedAfterPoolDestroy = scopedPool.acquire(1000);
            check(borrowedAfterPoolDestroy.get() != nullptr, "MysqlCppConnectionPool scoped acquire");
        }
    }
    if (borrowedAfterPoolDestroy.get() != nullptr)
    {
        check(borrowedAfterPoolDestroy->ping(), "MysqlCppConnectionPool borrowed connection after pool destroy");
        borrowedAfterPoolDestroy.reset();
        check(!borrowedAfterPoolDestroy && mysql.ping(), "MysqlCppConnectionPool borrowed connection reset after pool destroy");
    }

    // 工具仅操作本次创建的表，路径含空格及普通文件名字符，文件始终按路径参数处理。
    const std::string importPath = tableName + " input & data.sql";
    const std::string exportPath = tableName + " output & data.sql";
    {
        std::ofstream input(importPath.c_str(), std::ios::binary);
        input << "INSERT INTO " << tableName << " (id,name) VALUES(-300,'roundtrip');\n";
        check(input.good(), "write isolated SQL input");
    }
    mysql.importSql(importPath, mysqlConfig.m_ip, mysqlConfig.m_account, mysqlConfig.m_password, mysqlConfig.m_dbName);
    check(mysql.lastError().empty(), "real mysql tool imports independent argument file");
    auto imported = mysql.query(mysql.prepare("SELECT name FROM " + tableName + " WHERE id=-300"));
    check(imported && imported->next() && imported->getString(0) == "roundtrip", "imported SQL row present");
    imported.reset();
    mysql.commit();
    mysql.exportSql(exportPath, mysqlConfig.m_ip, mysqlConfig.m_account, mysqlConfig.m_password, mysqlConfig.m_dbName, tableName);
    check(mysql.lastError().empty(), "real mysqldump exports one owned table");
    {
        std::ifstream output(exportPath.c_str(), std::ios::binary);
        std::ostringstream content;
        content << output.rdbuf();
        check(content.str().find(tableName) != std::string::npos &&
            content.str().find("roundtrip") != std::string::npos, "exported table and inserted data present");
    }
    check(std::remove(importPath.c_str()) == 0 && std::remove(exportPath.c_str()) == 0, "remove owned SQL files");
    mysql.importSql(importPath, mysqlConfig.m_ip, mysqlConfig.m_account, mysqlConfig.m_password, mysqlConfig.m_dbName);
    check(!mysql.lastError().empty(), "missing import file reports error");
    if (check(executeSql("DROP TABLE IF EXISTS " + tableName).m_success, "MysqlCpp cleanup test table"))
    {
        // 仅在本次创建的表已删后复验 IF EXISTS，覆盖重复清理的原有断言。
        check(executeSql("DROP TABLE IF EXISTS " + tableName).m_success, "MysqlCpp drop old table");
    }
    mysql.close();
    mysql.uninit();
    check(!mysql.isInit(), "MysqlCpp real uninit");

    std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << " skipped=" << skipCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

/** 按功能用例号或显式性能参数运行。 */
int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        RunBenchmark();
        return 0;
    }
    if (argc == 2 && std::strcmp(argv[1], "--benchmark-db") == 0) { return RunChecks(false, true, true); }
    if (argc == 1 || (argc == 2 && std::strcmp(argv[1], "--all") == 0)) { return RunChecks(true, true, false); }
    const LibraryTestCase cases[] = {
        {1, false, []() { return RunChecks(true, false, false); }},
        {2, true, []() { return RunChecks(false, true, false); }}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}