#include "PlatformTest/PlatformTestAPI.h"
#include "MysqlCppTest.h"
#include <cerrno>
#include <climits>
#include <cstring>
#include <cstdio>
#include <fstream>
#include "MysqlCpp/MysqlCppAPI.h"
#include "MysqlCpp/MysqlCppHelper.h"
#include <chrono>

#include "mysql-connector-cpp/cppconn/exception.h"
#include <atomic>
#include <cstdlib>
#include <stdexcept>
#include <future>
#include <iostream>
#include <sstream>
#include <thread>
#ifndef _WIN32
#include <sys/wait.h>
#include <unistd.h>
#endif

#ifndef _WIN32
/** 检查清空环境后的宿主仍能给工具传递独立的凭据
@return 子进程成功运行工具，且工具读到期望的测试值时返回 true
*/
static bool CheckEmptyToolEnvironment()
{
    const std::vector<std::string> arguments = {"/bin/sh", "-c", R"(test "$MYSQL_PWD" = mysqlcpp_fixture)"};
    const pid_t child = fork();
    if (child < 0)
    {
        return false;
    }
    if (child == 0)
    {
        // 离线用例在创建工作线程前运行，只清空这个子进程的环境。
        const bool passed = clearenv() == 0 && MysqlCppHelper::runTool(arguments, "/dev/null", true, "mysqlcpp_fixture") == 0;
        _exit(passed ? 0 : 1);
    }
    int status = 0;
    pid_t waited;
    do
    {
        waited = waitpid(child, &status, 0);
    } while (waited < 0 && errno == EINTR);
    return waited == child && WIFEXITED(status) && WEXITSTATUS(status) == 0;
}
#endif

/** 用工具启动入口运行测试程序自身的一个子进程步骤
@param [in] executable 当前测试程序的启动路径
@param [in] step 子进程执行的测试步骤
@param [in] password 仅用于本次子进程的合成密码
@return 返回子进程退出状态，无法启动时返回非零值
*/
static int RunEnvironmentStep(const char* executable, const char* step, const char* password)
{
#ifdef _WIN32
    const char* input = "NUL";
#else
    const char* input = "/dev/null";
#endif
    return MysqlCppHelper::runTool({executable, step}, input, true, password);
}

/** 在独立宿主中设置模拟环境，再检查真实子进程与宿主的环境隔离
@param [in] executable 当前测试程序的启动路径
@return 子进程继承自定义变量且宿主密码未被改写时返回 0
*/
static int CheckToolEnvironment(const char* executable)
{
    const char* marker = "MYSQL_PWD_MYSQLCPP_TEST_MARKER";
#ifdef _WIN32
    const bool prepared = _putenv_s(marker, "retained") == 0;
#else
    const bool prepared = setenv(marker, "retained", 1) == 0;
#endif
    if (!prepared || RunEnvironmentStep(executable, "--read-tool-environment", "child_fixture") != 0)
    {
        return 1;
    }
    const char* password = std::getenv("MYSQL_PWD");
    return password && std::strcmp(password, "parent_fixture") == 0 ? 0 : 1;
}

/** 测量固定 SQL 样本的首词分类，逐条核对预期
*/
static void RunBenchmark()
{
    const std::string inputs[] = {" /* caller */ -- comment\n select value FROM user_info", "insert into t values(1)",
        "# meta\nSHOW TABLES", "select_count"};
    const bool expected[] = {true, false, true, false};
    // count 是本批操作数，每次选择一个已构造样本；返回本批分类正确的条数。
    LibraryBenchmark::run("MysqlCpp", [&](uint32_t count) -> uint64_t
    {
        uint64_t matched = 0;
        for (uint32_t index = 0; index < count; ++index)
        {
            const uint32_t sample = index % 4;
            matched += MysqlCppHelper::isQuerySql(inputs[sample]) == expected[sample] ? 1 : 0;
        }
        if (matched != count)
        {
            throw std::runtime_error("SQL classification mismatch");
        }
        return matched;
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

/** 在零预建的池上同时借用，检查容量上限、真实会话身份及归还后的回收
@param [in] config 可用测试数据库的连接参数副本
@return 全部容量和会话检查通过时返回 0
*/
static int CheckConcurrentGrowth(const MysqlCppConfig& config)
{
    MysqlCppConfig limits = config;
    MysqlCppConnectionPool pool;
    if (!limits.setPoolLimits(0, 2) || !pool.init(limits) || pool.totalCount() != 0)
    {
        return 1;
    }
    const size_t kWorkers = 6;
    std::future<std::shared_ptr<MysqlCpp>> requests[kWorkers];
    // 先销毁启动信号，再等待已启动的任务，允许部分线程创建失败时自然退出。
    std::promise<void> start;
    const std::shared_future<void> signal = start.get_future().share();
    for (size_t i = 0; i < kWorkers; ++i)
    {
        // 全部请求使用同一个起点；返回值由 future 持有，检查容量前不会归还。
        requests[i] = std::async(std::launch::async, [&pool, signal]() -> std::shared_ptr<MysqlCpp>
        {
            signal.wait();
            return pool.acquire(0);
        });
    }
    start.set_value();
    std::vector<std::shared_ptr<MysqlCpp>> leases;
    for (size_t i = 0; i < kWorkers; ++i)
    {
        const auto lease = requests[i].get();
        if (lease)
        {
            leases.push_back(lease);
        }
    }
    if (leases.size() != 2 || pool.totalCount() != 2 || pool.availableCount() != 0)
    {
        std::cerr << "Concurrent pool capacity mismatch, successful leases: " << leases.size() << std::endl;
        return 1;
    }
    uint64_t ids[2] = {};
    for (size_t i = 0; i < leases.size(); ++i)
    {
        const auto rows = leases[i]->query(leases[i]->prepare("SELECT CONNECTION_ID()"), false);
        if (!rows || !rows->next())
        {
            return 1;
        }
        ids[i] = rows->getUInt64(0);
    }
    leases.clear();
    if (ids[0] == 0 || ids[1] == 0 || ids[0] == ids[1] || pool.totalCount() != 0 || pool.availableCount() != 0)
    {
        return 1;
    }
    const auto next = pool.acquire(0);
    return next && next->ping() ? 0 : 1;
}

/** 按选定用例核对离线接口或连接隔离的数据库服务
@param [in] testCase 离线、在线集成或独立会话用例编号
@param [in] benchmark 连接成功后执行数据库计时
@return 成功返回 0，断言失败返回 1
*/
static int RunChecks(MysqlCppTestCase testCase, bool benchmark = false)
{
    int32_t totalCount = 0;
    int32_t failCount = 0;

    // 比较本次观察值并按名称累计失败，返回本项是否通过。
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

    // 指定环境变量为空或缺失时返回传入的默认文本。
    auto envString = [](const char* name, const std::string& defaultValue) -> std::string
    {
        const char* value = std::getenv(name);
        if (value == nullptr || value[0] == '\0')
        {
            return defaultValue;
        }
        return value;
    };

    // 从环境变量读取完整的有符号整数，非法输入结束本次用例。
    auto envInt32 = [&envString](const char* name, int32_t defaultValue) -> int32_t
    {
        std::string value = envString(name, "");
        if (value.empty())
        {
            return defaultValue;
        }
        char* end = nullptr;
        errno = 0;
        const long parsed = std::strtol(value.c_str(), &end, 10);
        if (errno == ERANGE || end == value.c_str() || *end != 0 || parsed < INT32_MIN || parsed > INT32_MAX)
        {
            throw std::invalid_argument(std::string("Invalid integer in ") + name);
        }
        return static_cast<int32_t>(parsed);
    };

    if (testCase == MysqlCppOffline)
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
        check(MysqlCppHelper::firstSqlWord(" select_key2") == "SELECT_KEY2", "complete unquoted identifier");
        check(!MysqlCppHelper::isQuerySql("select_1") && !MysqlCppHelper::isQuerySql("SHOW2") &&
            !MysqlCppHelper::isLastInsertIdSql("INSERT$log"), "identifier suffix cannot end a SQL keyword");
        check(!MysqlCppHelper::isQuerySql("--label\nSELECT 1") && MysqlCppHelper::isQuerySql("--\tlabel\nSELECT 1"),
            "MySQL double dash comment requires whitespace or control");
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
        check(CheckEmptyToolEnvironment(), "tool runs with only its credential in a cleared host environment");
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
        const std::string plainImport = MysqlSqlString::importString("input.sql", "::1", "user", "pwd", "db");
        const std::string plainExport = MysqlSqlString::exportString("output.sql", "::1", "user", "pwd", "db");
        check(!plainImport.empty() && plainImport == MysqlSqlString::importString("input.sql", "[::1]", "user", "pwd", "db"),
            "both IPv6 forms produce the same import host argument");
        check(!plainExport.empty() && plainExport == MysqlSqlString::exportString("output.sql", "[::1]", "user", "pwd", "db"),
            "both IPv6 forms produce the same export host argument");
        check(MysqlSqlString::importString("input.sql", "", "user", "pwd", "db").empty() &&
            MysqlSqlString::exportString("output.sql", "[::1]other", "user", "pwd", "db").empty(),
            "command previews reject empty and malformed hosts");
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
        std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << std::endl;
        return failCount == 0 ? 0 : 1;
    }

    MysqlCppConfig mysqlConfig;
    if (!mysqlConfig.setEndpoint(envString("MYSQLCPP_TEST_IP", "127.0.0.1"), envInt32("MYSQLCPP_TEST_PORT", 33841)) ||
        !mysqlConfig.setCredentials(envString("MYSQLCPP_TEST_ACCOUNT", "root"), envString("MYSQLCPP_TEST_PASSWORD", "")) ||
        !mysqlConfig.setDatabase(envString("MYSQLCPP_TEST_DB", "test")))
    {
        std::cerr << "Invalid MySQL test connection parameters" << std::endl;
        return 1;
    }
    mysqlConfig.setAutoCommit(false);
    mysqlConfig.setAutoReconnect(true);
    mysqlConfig.setTimeouts(envInt32("MYSQLCPP_TEST_CONNECT_TIMEOUT_SECONDS", 3),
        envInt32("MYSQLCPP_TEST_READ_TIMEOUT_SECONDS", 5), envInt32("MYSQLCPP_TEST_WRITE_TIMEOUT_SECONDS", 5));

    MysqlCpp mysql;
    if (!mysql.configure(mysqlConfig) || !mysql.init() || !mysql.connect())
    {
        check(false, "MysqlCpp test instance connection failed: " + mysql.lastError());
        std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << std::endl;
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
    if (testCase == MysqlCppConcurrentGrowth)
    {
        return CheckConcurrentGrowth(mysqlConfig);
    }
    if (testCase == MysqlCppPoolAuthentication)
    {
        MysqlCppConfig options = mysqlConfig;
        // 使用本次生成的账号名制造认证失败，避免触发已配置账号的登录失败锁定策略。
        const auto unique = mysql.query(mysql.prepare("SELECT REPLACE(UUID(), '-', '')"), false);
        if (!check(unique->next(), "generate an independent login name for the refused connection"))
        {
            return 1;
        }
        const std::string unknownAccount = unique->getString(0);
        if (!check(unknownAccount.size() == 32 && unknownAccount != mysqlConfig.m_account &&
            options.setCredentials(unknownAccount, "unused-test-password"), "configure the unprovisioned login"))
        {
            return 1;
        }
        MysqlCppConnectionPool pool;
        const bool refused = !pool.init(options, 1);
        check(refused && pool.lastError().find("code=1045") != std::string::npos &&
            pool.totalCount() == 0 && pool.availableCount() == 0, "failed prewarm leaves no session or slot");
        options.setPoolLimits(0, 1);
        check(pool.init(options), "zero prewarm defers authentication");
        const auto lease = pool.acquire(0);
        check(!lease && pool.lastError().find("code=1045") != std::string::npos &&
            pool.totalCount() == 0 && pool.availableCount() == 0, "failed lazy authentication releases its reservation");
        pool.close();
        options.setCredentials(mysqlConfig.m_account, mysqlConfig.m_password);
        options.setAutoCommit(true);
        const bool saved = pool.init(options);
        options.setCredentials("unused", "unused");
        options.setDatabase("information_schema");
        options.setAutoCommit(false);
        auto recovered = pool.acquire(0);
        check(saved && recovered && pool.lastError().empty() && pool.totalCount() == 1,
            "corrected credentials reopen the pool after authentication failure");
        if (recovered)
        {
            const auto row = recovered->query(recovered->prepare("SELECT DATABASE(), @@session.autocommit, CURRENT_USER()"), false);
            check(row->next() && row->getString(0) == mysqlConfig.m_dbName && row->getInt(1) == 1 &&
                row->getString(2).substr(0, mysqlConfig.m_account.size() + 1) == mysqlConfig.m_account + "@",
                "lazy session uses the original database mode and account");
        }
        recovered.reset();
        check(pool.totalCount() == 0 && pool.availableCount() == 0, "corrected zero-prewarm pool releases the last session");
        std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << std::endl;
        return failCount == 0 ? 0 : 1;
    }
    if (testCase == MysqlCppPoolRecovery)
    {
        MysqlCppConnectionPool pool;
        if (!check(pool.init(mysqlConfig, 1), "initialize recovery pool"))
        {
            return 1;
        }
        auto borrowed = pool.acquire(0);
        if (!check(borrowed && borrowed->isConnected(), "obtain initial live lease"))
        {
            return 1;
        }
        const auto initial = borrowed->query(borrowed->prepare("SELECT DATABASE(), @@session.autocommit"), false);
        check(initial->next() && initial->getString(0) == mysqlConfig.m_dbName && initial->getInt(1) == 0,
            "initial lease uses the configured schema and commit mode");
        MysqlCppConfig other = mysqlConfig;
        other.setDatabase("information_schema");
        other.setAutoCommit(true);
        borrowed->uninit();
        check(borrowed->configure(other), "closed borrower accepts different local configuration");
        borrowed.reset();
        auto recovered = pool.acquire(0);
        if (!check(recovered && recovered->isConnected(), "recover a closed pooled connection"))
        {
            return 1;
        }
        const auto state = recovered->query(recovered->prepare("SELECT DATABASE(), @@session.autocommit"), false);
        check(state->next() && state->getString(0) == mysqlConfig.m_dbName && state->getInt(1) == 0,
            "recovery applies the pool's saved schema and autocommit");
        // 不存在的普通标识符应得到数据库不存在或无权限错误，原会话仍然可用。
        const bool refused = !mysql.selectDb("mysqlcpp_absent`name");
        const std::string reason = mysql.lastError();
        const auto unchanged = mysql.query(mysql.prepare("SELECT DATABASE()"), false);
        check(refused && (reason.find("code=1049") != std::string::npos || reason.find("code=1044") != std::string::npos) &&
            unchanged->next() && unchanged->getString(0) == mysqlConfig.m_dbName, "literal schema failure preserves current database");
        std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << std::endl;
        return failCount == 0 ? 0 : 1;
    }
    if (testCase == MysqlCppSession)
    {
        // 将借用对象保留到连接析构之后，再调用其公开入口并按不同顺序释放。
        std::unique_ptr<MysqlCppTransaction> ended;
        std::shared_ptr<MysqlCppPrepareStatement> prepared;
        std::shared_ptr<MysqlCppResultSet> rows;
        {
            MysqlCpp owner;
            if (!check(owner.connect(mysqlConfig), "connect short-lived session"))
            {
                return 1;
            }
            ended.reset(new MysqlCppTransaction(owner));
            prepared = owner.prepare("SELECT 47");
            rows = owner.query(prepared, false);
            check(ended->isActive() && rows->next() && rows->getInt(0) == 47,
                "short-lived owner has active transaction and readable rows");
        }
        check(!ended->isActive() && !ended->commit() && !prepared->isValid() && !rows->isValid() &&
            !rows->next() && rows->getInt(0) == 0, "all borrowed handles expire when owner is destroyed");
        ended->rollback();
        ended.reset();
        prepared.reset();
        rows.reset();
        const auto write = mysql.prepare("SET @mysqlcpp_bind_marker=?");
        write->setInt(0, 41);
        const bool seeded = mysql.executeUpdate(write, false).m_success;
        auto oldValue = mysql.query(mysql.prepare("SELECT @mysqlcpp_bind_marker"), false);
        check(seeded && oldValue->next() && oldValue->getInt(0) == 41, "valid binding reaches server");
        const bool cleared = mysql.executeUpdate(mysql.prepare("SET @mysqlcpp_bind_marker=NULL"), false).m_success;
        write->setInt(1, 99);
        const std::string bindingError = write->lastError();
        const auto refused = mysql.executeUpdate(write, false);
        const auto marker = mysql.query(mysql.prepare("SELECT @mysqlcpp_bind_marker IS NULL"), false);
        check(cleared && !bindingError.empty() && !refused.m_success && refused.m_errorMessage == bindingError && marker->next() && marker->getInt(0) == 1,
            "failed rebind cannot execute a retained value");
        auto query = mysql.prepare("SELECT CAST(? AS SIGNED)");
        query->setInt(0, 17);
        query->setInt(UINT32_MAX, 31);
        check(!mysql.query(query, false)->isValid() && !mysql.lastError().empty(), "overflow binding blocks query");
        query->setInt(0, 33);
        check(!mysql.execute(query, false)->isValid() && !mysql.lastError().empty(), "later setter cannot mask a failed binding");
        auto blob = mysql.prepare("SELECT ?");
        blob->setBlob(0, nullptr);
        check(!blob->isValid() && !blob->lastError().empty(), "null BLOB stream rejected before execution");
        auto fresh = mysql.prepare("SELECT CAST(? AS SIGNED)");
        fresh->setInt(0, 23);
        const auto value = mysql.query(fresh, false);
        check(fresh->lastError().empty() && value->next() && value->getInt(0) == 23, "fresh statement recovers from binding failure");

        const auto malformed = mysql.prepare("SELECT (");
        const std::string prepareError = malformed->lastError();
        mysql.prepare("SELECT 1");
        check(!malformed->isValid() && !prepareError.empty() && !mysql.query(malformed, false)->isValid() &&
            mysql.lastError() == prepareError, "failed prepare retains its own driver error after another operation");
        MysqlCpp observer;
        auto id = mysql.query(mysql.prepare("SELECT CONNECTION_ID()"), false);
        if (!check(observer.connect(mysqlConfig) && id->next(), "independent observer and source connection ID"))
        {
            return 1;
        }
        // 驱动为同一语句复用游标和列缓冲，检查旧包装不能读到新一轮结果。
        const auto repeated = mysql.prepare("SELECT CAST(? AS SIGNED) AS value_read");
        repeated->setInt(0, 13);
        auto previous = mysql.query(repeated, false);
        const auto separate = mysql.query(mysql.prepare("SELECT 19"), false);
        const bool foreign = !observer.query(repeated, false)->isValid() && !observer.lastError().empty();
        check(foreign && separate->next() && separate->getInt(0) == 19 && previous->isValid() &&
            previous->next() && previous->getInt(0) == 13, "foreign or unrelated execution preserves the current cursor");
        repeated->setInt(0, 29);
        const auto replacement = mysql.execute(repeated, false);
        check(!previous->isValid() && !previous->next() && previous->getInt(0) == 0,
            "re-execution retires the previous cursor");
        previous.reset();
        check(replacement->isValid() && replacement->next() && replacement->getInt(0) == 29 && !replacement->next(),
            "destroying old rows leaves the replacement cursor intact");
        const auto wrongEntry = mysql.executeUpdate(repeated, false);
        check(!wrongEntry.m_success && !wrongEntry.m_errorMessage.empty() && replacement->isValid() &&
            replacement->previous() && replacement->getInt(0) == 29, "wrong update entry preserves the unexecuted query cursor");
        replacement->close();
        replacement->close();
        const auto reopened = mysql.query(repeated, false);
        check(!replacement->isValid() && reopened->next() && reopened->getInt(0) == 29,
            "closing a result twice leaves the statement reusable");
        repeated->close();
        repeated->close();
        check(!repeated->isValid() && !reopened->isValid() && !mysql.query(repeated, false)->isValid(),
            "closing a statement twice releases its cursor and blocks execution");
        const uint64_t originalId = id->getUInt64(0);
        std::ostringstream name;
        name << "mysqlcpp_session_" << originalId;
        const std::string table = name.str();
        if (!check(observer.executeUpdate(observer.prepare("CREATE TABLE " + table + " (n INT) ENGINE=InnoDB")).m_success,
            "create owned transaction table"))
        {
            return 1;
        }
        MysqlCppTransaction transaction(mysql);
        const bool inserted = transaction.isActive() &&
            mysql.executeUpdate(mysql.prepare("INSERT INTO " + table + " VALUES (17)"), false).m_success;
        const auto before = observer.query(observer.prepare("SELECT COUNT(*) FROM " + table));
        check(inserted && before->next() && before->getInt(0) == 0, "pending insert is private before reconnect");
        const bool replaced = mysql.reconnect();
        auto after = mysql.query(mysql.prepare("SELECT CONNECTION_ID(), DATABASE(), @@session.autocommit"), false);
        check(replaced && !transaction.isActive() && !fresh->isValid() && !id->isValid() && after->next() &&
            after->getUInt64(0) != originalId && after->getString(1) == mysqlConfig.m_dbName && after->getInt(2) == 0,
            "explicit reconnect opens a distinct configured server session");
        const bool committed = mysql.commit();
        const auto remaining = observer.query(observer.prepare("SELECT COUNT(*) FROM " + table));
        check(committed && remaining->next() && remaining->getInt(0) == 0, "new session cannot commit old transaction");
        check(observer.executeUpdate(observer.prepare("DROP TABLE " + table)).m_success, "remove owned transaction table");
        std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << std::endl;
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
    {
        const auto statement = mysql.prepare("SELECT CONNECTION_ID()");
        uint64_t originalId = 0;
        {
            const auto before = mysql.query(statement, false);
            if (before->next())
            {
                originalId = before->getUInt64(0);
            }
        }
        MysqlCppConfig invalid = mysqlConfig;
        invalid.m_ip = "[::1]" + mysqlConfig.m_ip;
        const bool refused = !mysql.connect(invalid) && !mysql.lastError().empty();
        const auto after = mysql.query(statement, false);
        check(refused && originalId != 0 && after->next() && after->getUInt64(0) == originalId,
            "malformed bracketed endpoint preserves the live session and its statement");
        invalid.m_ip = "localhost:3306";
        const bool singleHost = !mysql.connect(invalid) && !mysql.lastError().empty();
        const auto retained = mysql.query(statement, false);
        check(singleHost && originalId != 0 && retained->next() && retained->getUInt64(0) == originalId,
            "a port embedded in the host does not replace the active session");
    }
    check(mysql.ping(), "MysqlCpp real ping");
    {
        const auto aliases = mysql.query(mysql.prepare("SELECT 37 AS Amount, CAST('xy' AS BINARY) AS Payload"), false);
        const bool positioned = aliases->next();
        std::unique_ptr<std::istream> bytes(aliases->getBlob("PAYLOAD"));
        std::ostringstream contents;
        if (bytes)
        {
            contents << bytes->rdbuf();
        }
        check(positioned && aliases->getInt("amount") == 37 && aliases->getUInt("AMOUNT") == 37 &&
            aliases->getInt64("Amount") == 37 && aliases->getUInt64("Amount") == 37 &&
            aliases->getDouble("amount") == 37 && aliases->getBoolean("amount") &&
            aliases->getString("PAYLOAD") == "xy" && contents.str() == "xy", "resolve complete case-insensitive aliases");
        const std::string invalidLabels[] = {std::string("Amount\0suffix", 13), "missing"};
        for (size_t i = 0; i < sizeof(invalidLabels) / sizeof(invalidLabels[0]); ++i)
        {
            const std::string& label = invalidLabels[i];
            std::unique_ptr<std::istream> invalid(aliases->getBlob(label));
            check(!invalid && !aliases->getBoolean(label) && aliases->getDouble(label) == 0 &&
                aliases->getInt(label) == 0 && aliases->getUInt(label) == 0 && aliases->getInt64(label) == 0 &&
                aliases->getUInt64(label) == 0 && aliases->getString(label).empty() && aliases->getInt("Amount") == 37,
                i == 0 ? "embedded NUL cannot select an alias prefix" : "unknown label leaves the current row readable");
        }
    }
    // 两条独立会话执行相同的只读语句，以服务器结果和错误码判定读超时配置。
    for (int32_t i = 0; i < 2; ++i)
    {
        MysqlCppConfig options = mysqlConfig;
        options.setAutoReconnect(false);
        options.setTimeouts(5, i == 0 ? 1 : 10, 10);
        MysqlCpp timed;
        if (!check(timed.configure(options) && timed.connect(), "connect with independent read deadline"))
        {
            return 1;
        }
        const auto wait = timed.prepare("SELECT SLEEP(3)");
        if (!check(wait->isValid(), "prepare delayed response outside the read operation"))
        {
            return 1;
        }
        const auto reply = timed.query(wait, false);
        if (i == 0)
        {
            const std::string failure = timed.lastError();
            check(!reply->isValid() && (failure.find("code=2013") != std::string::npos ||
                failure.find("code=2006") != std::string::npos), "short read deadline reports a lost server response");
        }
        else
        {
            check(reply->isValid() && reply->next() && reply->getInt(0) == 0 && timed.lastError().empty(),
                "long read deadline permits the same server response");
        }
    }
    {
        const auto cursor = mysql.query(mysql.prepare("SELECT 11 AS n UNION ALL SELECT 22 UNION ALL SELECT 33 ORDER BY n"), false);
        check(cursor->isValid() && cursor->rowsCount() == 3 && cursor->columnCount() == 1 &&
            !cursor->previous(), "buffered result starts before three rows");
        check(cursor->next() && cursor->getInt("n") == 11 && cursor->next() && cursor->getInt(0) == 22 &&
            cursor->previous() && cursor->getInt(0) == 11, "cursor returns from second row to first");
        check(!cursor->previous() && cursor->next() && cursor->getInt(0) == 11, "cursor resumes after moving before first row");
        const auto tail = cursor->toVector();
        check(tail == std::vector<std::vector<std::string>>({{"22"}, {"33"}}) && !cursor->next() &&
            cursor->previous() && cursor->getInt(0) == 33 && cursor->rowsCount() == 3,
            "conversion consumes remaining rows and cursor can move back from end");
    }
    {
        MysqlCpp second;
        MysqlCppConfig options = mysqlConfig;
        options.setAutoCommit(true);
        options.setAutoReconnect(false);
        MysqlCppConfig earlier;
        const bool staged = earlier.setEndpoint("127.0.0.1", 1) && earlier.setCredentials("unused", "unused") &&
            earlier.setDatabase("information_schema") && second.configure(earlier);
        const bool saved = second.configure(options);
        MysqlCppConfig rejected = earlier;
        rejected.m_port = 0;
        check(staged && saved && !second.configure(rejected) && !second.lastError().empty() && !second.isInit(),
            "last valid pre-init configuration survives a rejected replacement");
        options.setEndpoint("127.0.0.1", 1);
        options.setCredentials("unused", "unused");
        options.setDatabase("unused");
        options.setAutoCommit(false);
        check(saved && second.init() && !second.configure(options) && second.connect(),
            "pre-init settings are copied and remain active after refused reconfiguration");
        const auto actual = second.query(second.prepare("SELECT DATABASE(), @@session.autocommit, CURRENT_USER()"), false);
        check(actual->next() && actual->getString(0) == mysqlConfig.m_dbName && actual->getInt(1) == 1 &&
            actual->getString(2).substr(0, mysqlConfig.m_account.size() + 1) == mysqlConfig.m_account + "@",
            "configured database account and autocommit observed by server");
        const auto types = second.prepare("SELECT CAST(? AS SIGNED), CAST(? AS UNSIGNED), CAST(? AS SIGNED), "
            "CAST(? AS UNSIGNED), CAST(? AS SIGNED), CAST(? AS DATETIME), ? IS NULL");
        types->setInt64(0, -5000000000LL);
        types->setUInt64(1, 12000000000000000000ULL);
        types->setBigInt(2, "6000000000");
        types->setUInt(3, 4000000000U);
        types->setBoolean(4, true);
        types->setDateTime(5, "2026-01-02 03:04:05");
        types->setNull(6, 0);
        const auto typed = second.query(types, false);
        check(typed->next() && typed->getInt64(0) == -5000000000LL && typed->getUInt64(1) == 12000000000000000000ULL &&
            typed->getInt64(2) == 6000000000LL && typed->getUInt(3) == 4000000000U && typed->getBoolean(4) &&
            typed->getString(5) == "2026-01-02 03:04:05" && typed->getInt(6) == 1 && !typed->next(),
            "typed setters deliver exact values and SQL NULL to server");
        auto query = mysql.prepare("SELECT CONNECTION_ID()");
        auto write = mysql.prepare("SET @mysqlcpp_cross_session=7");
        check(!second.query(query)->isValid() && !second.lastError().empty(), "foreign query rejected by receiving session");
        check(!second.execute(query)->isValid() && !second.lastError().empty(), "foreign generic statement rejected");
        check(!second.executeUpdate(write).m_success && !second.lastError().empty(), "foreign update rejected");
        const auto receivingMark = second.query(second.prepare("SELECT @mysqlcpp_cross_session IS NULL"));
        check(receivingMark->next() && receivingMark->getInt(0) == 1, "foreign write never reached receiving session");
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

    // 共享工作目录或数据库的测试进程须使用各自的表、存储过程与文件名。
    const auto fixtureId = mysql.query(mysql.prepare("SELECT REPLACE(UUID(), '-', '')"), false);
    const std::string suffix = fixtureId && fixtureId->next() ? fixtureId->getString(0) : std::string();
    if (suffix.size() != 32 || suffix.find_first_not_of("0123456789abcdefABCDEF") != std::string::npos)
    {
        std::cerr << "Cannot allocate a unique fixture name" << std::endl;
        return 1;
    }
    const std::string tableName = "mysqlcpp_test_" + suffix;

    // 对本轮独占表执行 SQL 并请求提交，返回实际更新状态。
    auto executeSql = [&mysql](const std::string& sqlText) -> MysqlCppExecuteResult
    {
        return mysql.executeUpdate(mysql.prepare(sqlText));
    };

    // 在当前会话的事务中执行一条 SQL，由用例显式控制提交点。
    auto executeSqlNoCommit = [&mysql](const std::string& sqlText) -> MysqlCppExecuteResult
    {
        return mysql.executeUpdate(mysql.prepare(sqlText), false);
    };

    if (!check(executeSql("CREATE TABLE " + tableName + " (id INT PRIMARY KEY, name VARCHAR(64), score INT, flag TINYINT, amount DOUBLE)").m_success, "MysqlCpp create table")) { return 1; }

    // 8.4 的预处理协议允许 CALL，但不支持直接准备 XA。经独立 CLI 建立并清理事务和多结果测试过程。
    const std::string xaPath = tableName + " xa.sql";
    // create 决定生成创建或清理脚本，返回真实 CLI 的执行状态。
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
        const std::string dynamicRows = tableName + "_rows";
        if (create)
        {
            sqlFile << "DELIMITER //\nCREATE PROCEDURE " << dynamicRows
                << "() BEGIN SELECT 61; SELECT 62; END//\n"
                << "CREATE PROCEDURE " << dynamicRows << "_error() BEGIN SELECT 63; "
                << "SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT='tail failure'; END//\nDELIMITER ;\n";
        }
        else
        {
            sqlFile << "DROP PROCEDURE IF EXISTS " << dynamicRows << ";\n"
                << "DROP PROCEDURE IF EXISTS " << dynamicRows << "_error;\n";
        }
        sqlFile.close();
        if (!sqlFile) { return false; }
        mysql.importSql(xaPath, mysqlConfig.m_ip, mysqlConfig.m_account, mysqlConfig.m_password, mysqlConfig.m_dbName);
        return mysql.lastError().empty();
    };
    const bool xaInstalled = installXa(true);
    const auto dynamicCall = mysql.prepare("CALL " + tableName + "_rows()");
    const auto unexpectedRows = mysql.executeUpdate(dynamicCall, false);
    const auto healthy = mysql.query(mysql.prepare("SELECT 67"), false);
    check(xaInstalled && !unexpectedRows.m_success && !unexpectedRows.m_errorMessage.empty() &&
        healthy->next() && healthy->getInt(0) == 67, "unexpected procedure rows do not block the next command");
    const auto procedure = mysql.prepare("CALL " + tableName + "_rows()");
    const auto procedureRows = mysql.query(procedure, false);
    const std::string readFailure = mysql.lastError();
    const bool firstRow = procedureRows->isValid() && procedureRows->next() && procedureRows->getInt(0) == 61;
    const bool consumed = procedureRows->close();
    const bool finished = mysql.commit();
    const std::string commitFailure = mysql.lastError();
    const auto nextCommand = mysql.query(mysql.prepare("SELECT 71"), false);
    if (!firstRow || !consumed || !finished || !nextCommand->isValid())
    {
        std::cerr << "CALL read=" << firstRow << " " << readFailure << ", commit=" << finished << " "
            << commitFailure << ", next=" << mysql.lastError() << std::endl;
    }
    check(firstRow && consumed && finished && nextCommand->next() && nextCommand->getInt(0) == 71,
        "closing the first result consumes all remaining procedure responses before commit");
    const auto badBindRows = mysql.query(procedure, false);
    procedure->setInt(0, 9);
    const bool bindConsumed = badBindRows->close();
    const auto afterBadBind = mysql.query(mysql.prepare("SELECT 72"), false);
    check(!procedure->isValid() && bindConsumed && afterBadBind->next() && afterBadBind->getInt(0) == 72,
        "a failed rebind still permits draining the live procedure result");
    procedure->close();
    const auto failingCall = mysql.prepare("CALL " + tableName + "_rows_error()");
    const auto errorRows = mysql.query(failingCall, false);
    const bool hasErrorRow = errorRows->next() && errorRows->getInt(0) == 63;
    const bool tailFailed = !errorRows->close();
    const auto afterTail = mysql.query(mysql.prepare("SELECT 74"), false);
    check(hasErrorRow && tailFailed && errorRows->close() && afterTail->next() && afterTail->getInt(0) == 74,
        "a later procedure error is reported on close and the connection accepts the next query");
    const auto firstRun = mysql.query(failingCall, false);
    const auto secondRun = mysql.query(failingCall, false);
    check(!firstRun->isValid() && !secondRun->isValid() && !mysql.lastError().empty(),
        "reusing a statement reports the previous unconsumed procedure error");
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

    {
        const auto created = executeSql("CREATE TEMPORARY TABLE mysqlcpp_generated_ids "
            "(id BIGINT UNSIGNED AUTO_INCREMENT PRIMARY KEY, marker INT UNIQUE) AUTO_INCREMENT=37");
        const auto inserted = executeSql("INSERT INTO mysqlcpp_generated_ids(marker) VALUES (7)");
        const auto replaced = executeSql("REPLACE INTO mysqlcpp_generated_ids(marker) VALUES (8)");
        const auto ids = mysql.query(mysql.prepare("SELECT id, marker FROM mysqlcpp_generated_ids ORDER BY marker"));
        check(created.m_success && inserted.m_success && inserted.m_affectedRows == 1 && inserted.m_lastInsertId > 0 &&
            ids->next() && ids->getInt(1) == 7 && ids->getUInt64(0) == inserted.m_lastInsertId, "generated INSERT ID matches stored row");
        check(replaced.m_success && replaced.m_affectedRows == 1 && replaced.m_lastInsertId > inserted.m_lastInsertId &&
            ids->next() && ids->getInt(1) == 8 && ids->getUInt64(0) == replaced.m_lastInsertId && !ids->next(),
            "generated REPLACE ID matches second stored row");
        check(executeSql("DROP TEMPORARY TABLE mysqlcpp_generated_ids").m_success, "release generated ID fixture");
    }
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
    check(selectResult.get() != nullptr && selectResult->columnNames() == std::vector<std::string>({"id", "name", "score", "flag", "amount"}), "MysqlCpp result column names");

    std::shared_ptr<MysqlCppPrepareStatement> vectorStatement = mysql.prepare("SELECT id, name FROM " + tableName + " ORDER BY id");
    std::shared_ptr<MysqlCppResultSet> vectorResult = mysql.query(vectorStatement);
    std::vector<std::vector<std::string>> vectorRows = vectorResult.get() == nullptr ? std::vector<std::vector<std::string>>() : vectorResult->toVector();
    check(vectorRows.size() == 1 && vectorRows[0] == std::vector<std::string>({"1", "alpha"}), "MysqlCpp result to vector");

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
    check(executeUpdateResult.get() != nullptr && !executeUpdateResult->isValid() && mysql.lastError().empty(), "MysqlCpp execute update dispatcher");

    MysqlCpp observer;
    check(observer.connect(mysqlConfig), "independent transaction observer");
    MysqlCppTransaction transaction(mysql);
    check(transaction.isActive(), "MysqlCppTransaction begin");
    check(executeSqlNoCommit("INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (2, 'beta', 88, 0, 2.5)").m_success, "MysqlCppTransaction insert before commit");
    const auto beforeCommit = observer.query(observer.prepare("SELECT COUNT(*) FROM " + tableName + " WHERE id=2"));
    check(beforeCommit->next() && beforeCommit->getInt(0) == 0, "uncommitted insert hidden from other session");
    check(transaction.commit(), "MysqlCppTransaction commit");
    std::shared_ptr<MysqlCppResultSet> commitResult = observer.query(observer.prepare("SELECT id FROM " + tableName + " WHERE id=2"));
    check(commitResult.get() != nullptr && commitResult->next(), "MysqlCppTransaction commit visible");

    MysqlCppTransaction rollbackTransaction(mysql);
    check(rollbackTransaction.isActive(), "MysqlCppTransaction rollback begin");
    check(executeSqlNoCommit("INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (3, 'gamma', 77, 1, 3.5)").m_success, "MysqlCppTransaction insert before rollback");
    rollbackTransaction.rollback();
    std::shared_ptr<MysqlCppResultSet> rollbackResult = observer.query(observer.prepare("SELECT id FROM " + tableName + " WHERE id=3"));
    check(rollbackResult.get() != nullptr && rollbackResult->isValid() && observer.lastError().empty() && !rollbackResult->next(), "MysqlCppTransaction rollback invisible");

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
        check(limits.setPoolLimits(0, 2), "configure finite pool");
        limits.setPoolAcquireTimeout(35);
        check(elastic.init(limits) && elastic.totalCount() == 0, "pool lazy initialization");
        auto first = elastic.acquire(0);
        auto second = elastic.acquire(0);
        check(first && second && first != second && elastic.totalCount() == 2, "pool growth to limit");
        check(!elastic.acquire(0), "pool limit immediate timeout");
        const auto waitStart = std::chrono::steady_clock::now();
        const bool timedOut = !elastic.acquire() && !elastic.lastError().empty();
        const auto waited = std::chrono::steady_clock::now() - waitStart;
        check(timedOut && waited >= std::chrono::milliseconds(35) && waited < std::chrono::seconds(2),
            "configured default pool wait applies at capacity");
        first.reset();
        second.reset();
        check(elastic.totalCount() == 0 && elastic.availableCount() == 0, "pool zero idle retention");
        elastic.close();
        check(limits.setPoolLimits(1, -1), "configure unlimited pool");
        check(elastic.init(limits) && elastic.availableCount() == 1, "pool unbounded warm start");
        first = elastic.acquire();
        second = elastic.acquire();
        auto third = elastic.acquire();
        check(first && second && third && elastic.totalCount() == 3, "pool unbounded growth");
        first.reset();
        second.reset();
        third.reset();
        check(elastic.totalCount() == 1 && elastic.availableCount() == 1, "pool retains configured minimum");
        elastic.close();
        // 重开池时检查已归还的空闲会话与仍有共享引用的外借会话分别计数。
        check(limits.setPoolLimits(1, 2) && elastic.init(limits) && elastic.totalCount() == 1 &&
            elastic.availableCount() == 1, "reopen with one idle session and a two-session limit");
        auto primary = elastic.acquire(0);
        auto shared = primary;
        auto spare = elastic.acquire(0);
        uint64_t keptId = 0;
        {
            const auto sharedId = shared ? shared->query(shared->prepare("SELECT CONNECTION_ID()"), false) : nullptr;
            const auto spareId = spare ? spare->query(spare->prepare("SELECT CONNECTION_ID()"), false) : nullptr;
            keptId = sharedId && sharedId->next() ? sharedId->getUInt64(0) : 0;
            primary.reset();
            check(keptId && spareId && spareId->next() && keptId != spareId->getUInt64(0) && !elastic.acquire(0) &&
                elastic.totalCount() == 2 && elastic.availableCount() == 0,
                "releasing one shared owner does not return an occupied slot");
        }
        spare.reset();
        check(elastic.totalCount() == 2 && elastic.availableCount() == 1, "last spare owner returns one idle session");
        elastic.close();
        {
            const auto survivor = shared ? shared->query(shared->prepare("SELECT CONNECTION_ID()"), false) : nullptr;
            check(elastic.totalCount() == 1 && elastic.availableCount() == 0 && !elastic.acquire(0) &&
                !elastic.init(limits) && survivor && survivor->next() && survivor->getUInt64(0) == keptId,
                "closing removes only the idle session and leaves the shared borrower usable");
        }
        shared.reset();
        check(elastic.totalCount() == 0 && elastic.availableCount() == 0 && elastic.init(limits) &&
            elastic.totalCount() == 1 && elastic.availableCount() == 1, "last outstanding owner permits a fresh generation");
        elastic.close();
        limits.setPoolAcquireTimeout(-1);
        check(limits.setPoolLimits(1, 1) && elastic.init(limits), "configure one reusable session and unlimited default wait");
        auto held = elastic.acquire(0);
        if (check(held && !elastic.acquire(0), "explicit zero overrides indefinite default on full pool"))
        {
            uint64_t id = 0;
            {
                const auto identity = held->query(held->prepare("SELECT CONNECTION_ID()"), false);
                id = identity->next() ? identity->getUInt64(0) : 0;
            }
            std::promise<void> entered;
            auto started = entered.get_future();
            // 等待任务使用池的默认期限，返回实际借到的连接，随后由主线程核对服务器会话。
            auto waiting = std::async(std::launch::async, [&elastic, &entered]() {
                entered.set_value();
                return elastic.acquire();
            });
            started.wait();
            check(waiting.wait_for(std::chrono::milliseconds(3200)) == std::future_status::timeout,
                "default indefinite acquire waits for the occupied slot");
            held.reset();
            const bool completed = waiting.wait_for(std::chrono::seconds(5)) == std::future_status::ready;
            if (!completed)
            {
                elastic.close();
            }
            const auto nextLease = waiting.get();
            const auto actual = nextLease ? nextLease->query(nextLease->prepare("SELECT CONNECTION_ID()"), false) : nullptr;
            check(completed && id != 0 && actual && actual->next() && actual->getUInt64(0) == id,
                "returned session satisfies indefinite default acquire");
        }
        elastic.close();
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
        // 先观察旧等待者的返回，再重新开放池，避免尚未调度到的线程误入新一代。
        check(!pending.get(), "closing wakes old generation without lending a session");
        check(!closing.init(mysqlConfig, 1), "outstanding old lease prevents reopening");
        held.reset();
        const bool reopened = closing.init(mysqlConfig, 1);
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
        threads.reserve(threadCount);
        try
        {
            for (int32_t i = 0; i < threadCount; ++i)
            {
                threads.emplace_back([&pool, &threadFailures, &tableName, i, loopCount]() -> void
                {
                    try
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
                            std::shared_ptr<MysqlCppPrepareStatement> threadInsert = connection->prepare(
                                "INSERT INTO " + tableName + " (id, name, score, flag, amount) VALUES (?, ?, ?, ?, ?)");
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

                            std::shared_ptr<MysqlCppPrepareStatement> threadSelect = connection->prepare(
                                "SELECT id FROM " + tableName + " WHERE id=?");
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
                    }
                    catch (...)
                    {
                        ++threadFailures;
                    }
                });
            }
        }
        catch (const std::exception& error)
        {
            ++threadFailures;
            std::cerr << "Cannot start pool worker: " << error.what() << std::endl;
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
        input.close();
        check(input.good(), "write isolated SQL input");
    }
    mysql.importSql(importPath, "", mysqlConfig.m_account, mysqlConfig.m_password, mysqlConfig.m_dbName);
    const bool rejectedImport = !mysql.lastError().empty();
    {
        const auto row = mysql.query(mysql.prepare("SELECT COUNT(*) FROM " + tableName + " WHERE id=-300"));
        check(rejectedImport && row->next() && row->getUInt(0) == 0, "empty import host cannot fall back to localhost");
    }
    {
        const std::string marker = "existing export contents";
        std::ofstream output(exportPath.c_str(), std::ios::binary | std::ios::trunc);
        output << marker;
        output.close();
        const bool preparedFile = output.good();
        mysql.exportSql(exportPath, "", mysqlConfig.m_account, mysqlConfig.m_password, mysqlConfig.m_dbName, tableName);
        const bool rejectedExport = !mysql.lastError().empty();
        std::ifstream original(exportPath.c_str(), std::ios::binary);
        std::ostringstream contents;
        contents << original.rdbuf();
        check(preparedFile && rejectedExport && contents.str() == marker, "invalid export host preserves the destination file");
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
    const bool removedInput = std::remove(importPath.c_str()) == 0;
    const bool removedOutput = std::remove(exportPath.c_str()) == 0;
    check(removedInput && removedOutput, "remove owned SQL files");
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

    std::cout << "[SUMMARY] total=" << totalCount << " failed=" << failCount << std::endl;
    return failCount == 0 ? 0 : 1;
}

/** 检查配置的默认值、失败原子性和最终驱动参数
@return 每项符合预期时返回 0
*/
static int RunConfigurationChecks()
{
    MysqlCppConfig config;
    if (!config.m_account.empty() || !config.m_password.empty() || !config.m_dbName.empty() ||
        config.m_poolAcquireTimeoutMs != 3000 || config.m_poolMinConnections != 1 || config.m_poolMaxConnections != 1)
    {
        std::cerr << "Incorrect default configuration" << std::endl;
        return 1;
    }
    if (!config.setCredentials("alpha", "beta") || !config.setCredentials(config.m_password, config.m_account) ||
        config.m_account != "beta" || config.m_password != "alpha")
    {
        std::cerr << "Credential setter lost aliased input" << std::endl;
        return 1;
    }
    // 每次只覆盖一个驱动期限，同时核对另两项以及默认的自动重连策略。
    const int32_t timeouts[][3] = {{5, 10, 10}, {17, 10, 10}, {5, 23, 10}, {5, 10, 31}};
    for (size_t i = 0; i < sizeof(timeouts) / sizeof(timeouts[0]); ++i)
    {
        MysqlCppConfig isolated;
        if (i != 0)
        {
            isolated.setTimeouts(timeouts[i][0], timeouts[i][1], timeouts[i][2]);
        }
        sql::ConnectOptionsMap properties;
        MysqlCppHelper::connectionOptions(isolated, properties);
        if (*properties.at("OPT_CONNECT_TIMEOUT").get<int>() != timeouts[i][0] ||
            *properties.at("OPT_READ_TIMEOUT").get<int>() != timeouts[i][1] ||
            *properties.at("OPT_WRITE_TIMEOUT").get<int>() != timeouts[i][2] ||
            !*properties.at("OPT_RECONNECT").get<bool>())
        {
            std::cerr << "Independent driver option failed at index " << i << std::endl;
            return 1;
        }
    }
    MysqlCppConfig endpoint;
    const bool invalidHost = !endpoint.setEndpoint("db.example/schema", 4407) &&
        endpoint.m_ip == "127.0.0.1" && endpoint.m_port == 3306;
    endpoint.m_ip = "tcp://db.example";
    endpoint.setPoolLimits(0, 1);
    MysqlCpp uninitialized;
    MysqlCppConnectionPool lazyPool;
    const bool validFields = endpoint.validConnection();
    const bool configured = uninitialized.configure(endpoint);
    const bool opened = lazyPool.init(endpoint);
    if (!invalidHost || validFields || configured || opened)
    {
        std::cerr << "Path or URI in the host field accepted: setter=" << !invalidHost
            << " validConnection=" << validFields << " configure=" << configured << " lazyPool=" << opened << std::endl;
        return 1;
    }
    const char* invalidHosts[] = {"", "db example", "db\tname", "[::1", "::1]", "[]", "[::1]other", "db[::1]", "[[::1]]",
        "localhost:3306", "127.0.0.1:3306", "[localhost:3306]", "2001:db8:::1", "1:2:3:4:5:6:7",
        "[2001:db8::gg]", "1:2:3:4:5:6:7:8:9", "2001::db8::1", "::ffff:192.0.2.300", "fe80::1%", "fe80::1%2%3"};
    for (size_t i = 0; i < sizeof(invalidHosts) / sizeof(invalidHosts[0]); ++i)
    {
        MysqlCppConfig candidate;
        candidate.setPoolLimits(0, 1);
        const bool accepted = candidate.setEndpoint(invalidHosts[i], 4407);
        const bool retained = candidate.m_ip == "127.0.0.1" && candidate.m_port == 3306;
        candidate.m_ip = invalidHosts[i];
        MysqlCpp single;
        MysqlCppConnectionPool pool;
        if (accepted || !retained || candidate.validConnection() || single.configure(candidate) || pool.init(candidate))
        {
            std::cerr << "Malformed host accepted at index " << i << std::endl;
            return 1;
        }
    }
    const char* ipv6[] = {"::", "[2001:db8::1]", "1:2:3:4:5:6:7:8", "::ffff:192.0.2.1", "fe80::1%3", "[fe80::1%eth0]"};
    for (size_t i = 0; i < sizeof(ipv6) / sizeof(ipv6[0]); ++i)
    {
        if (!endpoint.setEndpoint(ipv6[i], 3306) || endpoint.m_ip != ipv6[i] || !endpoint.validConnection())
        {
            std::cerr << "Valid IPv6 address rejected at index " << i << std::endl;
            return 1;
        }
    }
    if (!endpoint.setEndpoint("::1", 65535) || !endpoint.validConnection() || !endpoint.setEndpoint("[::1]", 1) ||
        !endpoint.validConnection() || !endpoint.setEndpoint("db.example", 3306))
    {
        std::cerr << "Valid TCP endpoint rejected" << std::endl;
        return 1;
    }
    // 负的上限只有 -1 合法。直接填写公开字段也要通过同一组池容量边界。
    const int32_t bounds[][2] = {{0, 0}, {0, -2}, {-1, -1}, {2, 1}};
    for (size_t i = 0; i < sizeof(bounds) / sizeof(bounds[0]); ++i)
    {
        MysqlCppConfig capacity;
        if (capacity.setPoolLimits(bounds[i][0], bounds[i][1]) ||
            capacity.m_poolMinConnections != 1 || capacity.m_poolMaxConnections != 1)
        {
            std::cerr << "Invalid pool limits replaced previous settings" << std::endl;
            return 1;
        }
        capacity.m_poolMinConnections = bounds[i][0];
        capacity.m_poolMaxConnections = bounds[i][1];
        MysqlCppConnectionPool pool;
        if (pool.init(capacity) || pool.lastError().empty() || pool.totalCount() != 0 || pool.availableCount() != 0)
        {
            std::cerr << "Invalid raw pool limits accepted" << std::endl;
            return 1;
        }
    }
    const std::string invalid("x\0y", 3);
    if (!config.setEndpoint("db.example", 4406) || !config.setCredentials("test-user", "test-password") ||
        !config.setDatabase("test-db") || !config.setPoolLimits(0, -1) || config.setPoolLimits(3, 2) ||
        config.setPoolLimits(-1, -1) || config.setEndpoint(invalid, 123) || config.setEndpoint("a", 65536) ||
        config.setCredentials(invalid, "other") || config.setCredentials("other", invalid) || config.setDatabase(invalid))
    {
        std::cerr << "Incorrect setter acceptance" << std::endl;
        return 1;
    }
    config.setAutoCommit(true);
    config.setAutoReconnect(false);
    config.setPoolAcquireTimeout(-1);
    config.setTimeouts(2, 7, 13);
    sql::ConnectOptionsMap driver;
    MysqlCppHelper::connectionOptions(config, driver);
    if (*driver.at("hostName").get<sql::SQLString>() != "tcp://db.example:4406" ||
        *driver.at("userName").get<sql::SQLString>() != "test-user" || *driver.at("password").get<sql::SQLString>() != "test-password" ||
        *driver.at("OPT_CONNECT_TIMEOUT").get<int>() != 2 || *driver.at("OPT_READ_TIMEOUT").get<int>() != 7 ||
        *driver.at("OPT_WRITE_TIMEOUT").get<int>() != 13 || *driver.at("OPT_RECONNECT").get<bool>() ||
        !config.m_autoCommit || config.m_dbName != "test-db" || config.m_poolAcquireTimeoutMs != -1 ||
        config.m_poolMinConnections != 0 || config.m_poolMaxConnections != -1)
    {
        std::cerr << "Incorrect driver or pool settings" << std::endl;
        return 1;
    }
    MysqlCppConfig reset = config;
    if (!reset.setEndpoint("127.0.0.1", 3306) || !reset.setCredentials("", "") || !reset.setDatabase("") ||
        !reset.setPoolLimits(1, 1))
    {
        std::cerr << "Cannot restore default endpoint, credentials, database or pool capacity" << std::endl;
        return 1;
    }
    reset.setAutoCommit(false);
    reset.setAutoReconnect(true);
    reset.setTimeouts(5, 10, 10);
    reset.setPoolAcquireTimeout(3000);
    sql::ConnectOptionsMap restored;
    MysqlCppHelper::connectionOptions(reset, restored);
    if (*restored.at("hostName").get<sql::SQLString>() != "tcp://127.0.0.1:3306" ||
        *restored.at("userName").get<sql::SQLString>() != "" || *restored.at("password").get<sql::SQLString>() != "" ||
        !*restored.at("OPT_RECONNECT").get<bool>() || *restored.at("OPT_CONNECT_TIMEOUT").get<int>() != 5 ||
        *restored.at("OPT_READ_TIMEOUT").get<int>() != 10 || *restored.at("OPT_WRITE_TIMEOUT").get<int>() != 10 ||
        reset.m_autoCommit || !reset.m_dbName.empty() || reset.m_poolMinConnections != 1 ||
        reset.m_poolMaxConnections != 1 || reset.m_poolAcquireTimeoutMs != 3000 ||
        config.m_ip != "db.example" || config.m_port != 4406 || config.m_account != "test-user" ||
        config.m_password != "test-password" || config.m_dbName != "test-db" || !config.m_autoCommit ||
        config.m_autoReconnect || config.m_connectTimeoutSeconds != 2 || config.m_readTimeoutSeconds != 7 ||
        config.m_writeTimeoutSeconds != 13 || config.m_poolMinConnections != 0 || config.m_poolMaxConnections != -1 ||
        config.m_poolAcquireTimeoutMs != -1)
    {
        std::cerr << "Resetting a configuration retained obsolete settings or modified its source" << std::endl;
        return 1;
    }
    // 逐项改变超时，其中两项回退，第三项必须保留显式值。
    config.setTimeouts(0, -1, 19);
    MysqlCppHelper::connectionOptions(config, driver);
    if (*driver.at("OPT_CONNECT_TIMEOUT").get<int>() != 5 || *driver.at("OPT_READ_TIMEOUT").get<int>() != 10 ||
        *driver.at("OPT_WRITE_TIMEOUT").get<int>() != 19)
    {
        std::cerr << "One timeout changed another option" << std::endl;
        return 1;
    }
    config.setTimeouts(0, -1, 0);
    MysqlCppHelper::connectionOptions(config, driver);
    MysqlCpp mysql;
    if (*driver.at("OPT_CONNECT_TIMEOUT").get<int>() != 5 || *driver.at("OPT_READ_TIMEOUT").get<int>() != 10 ||
        *driver.at("OPT_WRITE_TIMEOUT").get<int>() != 10 || !mysql.configure(config) || !mysql.init() || mysql.configure(config))
    {
        std::cerr << "Incorrect timeout fallback or configuration lifecycle" << std::endl;
        return 1;
    }
    mysql.uninit();
    config.m_port = -1;
    MysqlCppConnectionPool lazy;
    if (mysql.configure(config) || lazy.init(config) || !mysql.configure(MysqlCppConfig()))
    {
        std::cerr << "Invalid endpoint accepted or configure after uninit failed" << std::endl;
        return 1;
    }
    return 0;
}

/** 根据命令行执行指定功能用例或显式请求的性能基准
@param [in] argc 命令行参数个数
@param [in] argv 参数数组
@return 成功 0、用例失败 1、选择无效 2
*/
int main(int argc, char** argv)
{
    try
    {
        if (argc == 2 && std::strcmp(argv[1], "--prepare-tool-environment") == 0)
        {
            return CheckToolEnvironment(argv[0]);
        }
        if (argc == 2 && std::strcmp(argv[1], "--read-tool-environment") == 0)
        {
            const char* password = std::getenv("MYSQL_PWD");
            const char* marker = std::getenv("MYSQL_PWD_MYSQLCPP_TEST_MARKER");
            return password && marker && std::strcmp(password, "child_fixture") == 0 &&
                std::strcmp(marker, "retained") == 0 ? 0 : 1;
        }
        if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
        {
            RunBenchmark();
            return 0;
        }
        if (argc == 2 && std::strcmp(argv[1], "--benchmark-db") == 0)
        {
            return RunChecks(MysqlCppOnline, true);
        }
        // 命令行路径在 main 内始终有效，供无参用例回调再次启动本程序。
        static const char* const s_executable = argv[0];
        const LibraryTestCase cases[] = {
            {MysqlCppOffline, false, []() { return RunChecks(MysqlCppOffline); }},
            {MysqlCppOnline, true, []() { return RunChecks(MysqlCppOnline); }},
            {MysqlCppConfiguration, false, RunConfigurationChecks},
            {MysqlCppSession, true, []() { return RunChecks(MysqlCppSession); }},
            {MysqlCppPoolRecovery, true, []() { return RunChecks(MysqlCppPoolRecovery); }},
            {MysqlCppPoolAuthentication, true, []() { return RunChecks(MysqlCppPoolAuthentication); }},
            {MysqlCppToolEnvironment, false, []()
            {
                return RunEnvironmentStep(s_executable, "--prepare-tool-environment", "parent_fixture");
            }},
            {MysqlCppConcurrentGrowth, true, []() { return RunChecks(MysqlCppConcurrentGrowth); }}
        };
        // 同步执行一项，异常计为该项失败，其他独立用例继续运行。
        const auto execute = [](const LibraryTestCase& item) -> int
        {
            try
            {
                return item.run();
            }
            catch (const std::exception& error)
            {
                std::cerr << "case " << item.id << ": " << error.what() << std::endl;
            }
            catch (...)
            {
                std::cerr << "case " << item.id << ": unexpected exception" << std::endl;
            }
            return 1;
        };
        if (argc == 1 || (argc == 2 && std::strcmp(argv[1], "--all") == 0))
        {
            // 共用同一用例登记和调度器，全量先运行瞬时用例，两组分别记录失败。
            char instant[] = "--instant";
            char stress[] = "--stress";
            char* instantArgs[] = {argv[0], instant, nullptr};
            char* stressArgs[] = {argv[0], stress, nullptr};
            const int instantCode = LibraryCaseSelection::run(2, instantArgs, cases, execute);
            const int stressCode = LibraryCaseSelection::run(2, stressArgs, cases, execute);
            return instantCode == 0 && stressCode == 0 ? 0 : 1;
        }
        return LibraryCaseSelection::run(argc, argv, cases, execute);
    }
    catch (const std::exception& error)
    {
        std::cerr << error.what() << std::endl;
        return 1;
    }
}