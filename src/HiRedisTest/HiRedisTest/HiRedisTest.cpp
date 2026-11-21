#include "../../../common/PlatformTest/LibraryCaseSelection.h"
#include "HiRedisTest.h"
#include "../../../common/PlatformTest/LibraryBenchmark.h"
#include <cstring>
#include <cstdio>
#include "HiRedis/HiRedisHelper.h"
#include "HiRedis/HiRedisAPI.h"
#include "HiRedis/HiRedisConnectionPool.h"

#include <atomic>
#include <cstdlib>
#include <ctime>
#include <clocale>
#include <functional>
#include <future>
#include <iostream>
#include <sstream>
#include <thread>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

/** 对本库的一项固定规模操作连续采样，结果用于同平台的改动前后比较。 */
static void RunBenchmark()
{
    const std::string command = "SET key " + std::string(256, 'a') + " EX 60";
    std::vector<std::string> arguments;
    std::string error;
    LibraryBenchmark::run("HiRedis", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t index = 0; index < count; ++index)
        {
            checksum += HiRedisHelper::splitCommandText(command, arguments, error) ? arguments.size() : 0;
        }
        return checksum;
    });
}

/** 比较同一连接上逐条和每批 16 条 PING 的摊销时间，核对每个回显
@param [in] redis 已认证并核实版本的 Redis 连接，计时期间由本测试独占
*/
static void RunDatabaseBenchmark(HiRedis& redis)
{
    const uint32_t width = 16;
    std::vector<HiRedisCommand> commands;
    for (uint32_t i = 0; i < width; ++i)
    {
        const std::string echo(1, static_cast<char>('A' + i));
        commands.push_back(HiRedisCommand(std::vector<std::string>{"PING", echo}));
    }
    LibraryBenchmark::run("HiRedis.Ping", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::vector<std::string>& args = commands[i % width].m_args;
            const std::shared_ptr<HiRedisResultSet> reply = redis.command(args);
            if (!reply || reply->isError() || reply->toString() != args[1])
            {
                throw std::runtime_error("Unexpected Redis PING reply");
            }
            checksum += static_cast<unsigned char>(args[1][0]);
        }
        return checksum;
    });
    LibraryBenchmark::run("HiRedis.PipelinePing16", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t i = 0; i < count; ++i)
        {
            const std::vector<std::shared_ptr<HiRedisResultSet>> replies = redis.pipeline(commands);
            if (replies.size() != width)
            {
                throw std::runtime_error("Unexpected Redis pipeline reply count");
            }
            for (uint32_t j = 0; j < width; ++j)
            {
                const std::string& echo = commands[j].m_args[1];
                if (!replies[j] || replies[j]->isError() || replies[j]->toString() != echo)
                {
                    throw std::runtime_error("Unexpected Redis pipeline reply");
                }
                checksum += static_cast<unsigned char>(echo[0]);
            }
        }
        return checksum;
    }, width);
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
    std::function<void(bool, const std::string&)> check = [&totalCount, &failCount](bool ok, const std::string& name) -> void
    {
        ++totalCount;
        std::cout << (ok ? "[PASS] " : "[FAIL] ") << name << std::endl;
        if (!ok)
        {
            ++failCount;
        }
    };
    std::function<void(const std::string&)> skip = [&skipCount](const std::string& name) -> void
    {
        ++skipCount;
        std::cout << "[SKIP] " << name << std::endl;
    };
    std::function<std::string(const char*, const std::string&)> envString = [](const char* name, const std::string& defaultValue) -> std::string
    {
        const char* value = std::getenv(name);
        return value == nullptr ? defaultValue : std::string(value);
    };
    std::function<int32_t(const char*, int32_t)> envInt = [&envString](const char* name, int32_t defaultValue) -> int32_t
    {
        std::string value = envString(name, "");
        return value.empty() ? defaultValue : static_cast<int32_t>(std::atoi(value.c_str()));
    };
    std::function<int32_t()> currentProcessId = []() -> int32_t
    {
#ifdef _WIN32
        return static_cast<int32_t>(_getpid());
#else
        return static_cast<int32_t>(getpid());
#endif
    };

    if (offline)
    {
        HiRedis disconnected;
        check(!disconnected.isConnected(), "HiRedis default disconnected");
        check(!disconnected.ping(), "HiRedis ping disconnected");
        std::shared_ptr<HiRedisResultSet> disconnectedResult = disconnected.command(std::vector<std::string>{"GET", "missing"});
        check(disconnectedResult.get() != nullptr && disconnectedResult->isError(), "HiRedis disconnected command error");
        std::vector<HiRedisCommand> disconnectedCommands;
        disconnectedCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", "missing"}));
        std::vector<std::shared_ptr<HiRedisResultSet>> disconnectedPipelineResult = disconnected.pipeline(disconnectedCommands);
        check(disconnectedPipelineResult.size() == 1 && disconnectedPipelineResult[0]->isError(), "HiRedis disconnected pipeline error");
        std::shared_ptr<HiRedisResultSet> disconnectedTransactionResult = disconnected.execTransaction(disconnectedCommands);
        check(disconnectedTransactionResult.get() != nullptr && disconnectedTransactionResult->isError(), "HiRedis disconnected transaction error");
    }
    if (!online) { return failCount == 0 ? 0 : 1; }

    HiRedisConfig config;
    config.m_ip = envString("HIREDIS_TEST_IP", "127.0.0.1");
    config.m_port = envInt("HIREDIS_TEST_PORT", 6379);
    config.m_password = envString("HIREDIS_TEST_PASSWORD", "");
    config.m_dbIndex = envInt("HIREDIS_TEST_DB", 0);
    config.m_connectTimeoutMs = envInt("HIREDIS_TEST_CONNECT_TIMEOUT_MS", 3000);
    config.m_commandTimeoutMs = envInt("HIREDIS_TEST_COMMAND_TIMEOUT_MS", 3000);

    HiRedis redis;
    if (!redis.init(config))
    {
        check(false, "HiRedis test instance connection failed: " + redis.lastError());
        std::cout << "HiRedis test " << (failCount == 0 ? "PASS" : "FAIL") <<
            ", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
        return 1;
    }
    const std::shared_ptr<HiRedisResultSet> server = redis.command(std::vector<std::string>{"INFO", "server"});
    if (!server || server->toString().find("redis_version:8.10.3\r\n") == std::string::npos)
    {
        check(false, "HiRedis requires Redis server 8.10.3");
        return 1;
    }
    check(server && server->toString().find("redis_version:8.10.3\r\n") != std::string::npos, "HiRedis Redis server 8.10.3");
    if (benchmark)
    {
        try
        {
            RunDatabaseBenchmark(redis);
        }
        catch (const std::exception& error)
        {
            std::cerr << error.what() << std::endl;
            return 1;
        }
        return failCount == 0 ? 0 : 1;
    }
    {
        HiRedis scopedRedis;
        check(scopedRedis.init(config), "HiRedis destructor scoped init");
    }
    check(redis.ping(), "HiRedis destructor uninit safe");
    {
        HiRedis keepAliveOff;
        HiRedisConfig options = config;
        options.m_enableKeepAlive = false;
        check(keepAliveOff.init(options) && keepAliveOff.ping(), "Redis connects without TCP keepalive");
        const auto high = redis.command(std::vector<std::string>{"EVAL", "return 2147483664", "0"});
        const auto low = redis.command(std::vector<std::string>{"EVAL", "return -2147483664", "0"});
        check(high->toInt64() == INT64_C(2147483664) && high->toCount() == INT32_MAX,
            "32-bit count saturates above its maximum");
        check(low->toInt64() == -INT64_C(2147483664) && low->toCount() == INT32_MIN,
            "32-bit count saturates below its minimum");
    }

    std::ostringstream prefixStream;
    prefixStream << "hiredis_test_" << static_cast<int64_t>(std::time(nullptr)) << "_" <<
        currentProcessId() << "_" << reinterpret_cast<uintptr_t>(&prefixStream) << "_";
    const std::string prefix = prefixStream.str();
    const std::string stringKey = prefix + "string";
    const std::string binaryKey = prefix + "binary";
    const std::string rawPercentKey = prefix + "raw_percent";
    const std::string rawEscapeKey = prefix + "raw_escape";
    const std::string rawTrailingEscapeKey = prefix + "raw_trailing_escape";
    const std::string nxKey = prefix + "nx";
    const std::string intKey = prefix + "int";
    const std::string mapKey1 = prefix + "map_1";
    const std::string mapKey2 = prefix + "map_2";
    const std::string deleteOneKey = prefix + "delete_one";
    const std::string deleteManyKey1 = prefix + "delete_many_1";
    const std::string deleteManyKey2 = prefix + "delete_many_2";
    const std::string hashKey = prefix + "hash";
    const std::string tableKey = prefix + "table";
    const std::string groupKey = prefix + "group";
    const std::string orderKey = prefix + "order";
    const std::string transactionKey = prefix + "transaction";

    check(redis.isConnected(), "HiRedis connected");
    check(redis.ping(), "HiRedis ping connected");
    check(redis.selectDbIndex(0), "HiRedis selectDbIndex");
    check(redis.selectDb(0), "HiRedis selectDb");
    HiRedis compatRedis;
    check(compatRedis.connect(config), "HiRedis connect compatibility");
    const int64_t originalClient = compatRedis.command(std::vector<std::string>{"CLIENT", "ID"})->toInt64();
    HiRedisConfig invalidHost = config;
    invalidHost.m_ip.append("\0suffix", 7);
    const bool hostRejected = !compatRedis.connect(invalidHost) && !compatRedis.lastError().empty();
    const int64_t currentClient = compatRedis.command(std::vector<std::string>{"CLIENT", "ID"})->toInt64();
    check(hostRejected && originalClient > 0 && originalClient == currentClient,
        "reject embedded NUL hostname before replacing active connection");
    check(compatRedis.ping(), "HiRedis connect compatibility ping");
    compatRedis.close();
    check(!compatRedis.isConnected(), "HiRedis close compatibility");
    check(redis.reconnect() && redis.isConnected() && redis.ping(), "HiRedis reconnect");
    HiRedisConfig invalidPortConfig = config;
    invalidPortConfig.m_port = 1;
    invalidPortConfig.m_connectTimeoutMs = 50;
    HiRedis invalidPortRedis;
    check(!invalidPortRedis.init(invalidPortConfig) && !invalidPortRedis.isConnected(), "HiRedis init invalid port failed");
    HiRedisConfig invalidDbConfig = config;
    invalidDbConfig.m_dbIndex = 100000;
    HiRedis invalidDbRedis;
    check(!invalidDbRedis.init(invalidDbConfig) && !invalidDbRedis.isConnected(), "HiRedis init invalid db failed");
    HiRedisConfig invalidAuthConfig = config;
    invalidAuthConfig.m_password = config.m_password.empty() ? "__hiredis_bad_password__" : config.m_password + "__bad";
    HiRedis invalidAuthRedis;
    check(!invalidAuthRedis.init(invalidAuthConfig) && !invalidAuthRedis.isConnected(), "HiRedis init invalid auth failed");
    check(redis.set(stringKey, "value"), "HiRedis set");
    check(!redis.orderGroupElementExist(stringKey, "x") && !redis.lastError().empty(),
        "wrong key type never reports sorted set membership");
    check(redis.getOrderGroupElementIndex(stringKey, "x") == -1 && !redis.lastError().empty(),
        "wrong key type has no rank");
    check(redis.getOrderGroupElementScore(stringKey, "x") == -1 && !redis.lastError().empty(),
        "wrong key type has no score");
    check(redis.get(stringKey)->toString() == "value", "HiRedis get");
    check(redis.setKeyValue(stringKey, "value2"), "HiRedis setKeyValue");
    check(redis.getValue(stringKey)->toString() == "value2", "HiRedis getValue");
    std::string binaryValue("a\0b", 3);
    check(redis.set(binaryKey, binaryValue), "HiRedis set binary value");
    std::string binaryRead = redis.get(binaryKey)->toString();
    check(binaryRead.size() == 3 && binaryRead[0] == 'a' && binaryRead[1] == '\0' && binaryRead[2] == 'b', "HiRedis get binary value");
    check(redis.command(std::string("SET ") + rawPercentKey + " \"%hello world\"")->toStatus(), "HiRedis raw command quote percent");
    check(redis.get(rawPercentKey)->toString() == "%hello world", "HiRedis raw command quote result");
    check(redis.command(std::string("SET ") + rawEscapeKey + " a\\ b")->toStatus(), "HiRedis raw command escape space");
    check(redis.get(rawEscapeKey)->toString() == "a b", "HiRedis raw command escape result");
    check(redis.command(std::string("SET ") + rawTrailingEscapeKey + " abc\\")->toStatus(), "HiRedis raw command trailing escape");
    check(redis.get(rawTrailingEscapeKey)->toString() == "abc\\", "HiRedis raw command trailing escape result");
    check(redis.command("")->isError(), "HiRedis empty raw command error");
    check(redis.command("GET \"bad")->isError(), "HiRedis unclosed quote command error");
    check(redis.command(std::vector<std::string>())->isError(), "HiRedis empty argv command error");
    check(redis.setEx(prefix + "expire", "1", 30), "HiRedis setEx");
    check(redis.ttl(prefix + "expire") > 0, "HiRedis ttl");
    check(redis.setNx(nxKey, "first"), "HiRedis setNx first");
    check(!redis.setNx(nxKey, "second"), "HiRedis setNx second false");
    check(redis.incrBy(intKey, 10) == 10, "HiRedis incrBy");
    check(redis.decrBy(intKey, 3) == 7, "HiRedis decrBy");
    check(redis.expire(intKey, 30), "HiRedis expire");
    check(redis.keyExist(intKey), "HiRedis keyExist");
    check(redis.del(std::vector<std::string>()) == 0, "HiRedis del empty");
    check(redis.set(deleteOneKey, "1") && redis.deleteKey(deleteOneKey) && !redis.keyExist(deleteOneKey), "HiRedis deleteKey");
    check(redis.set(deleteManyKey1, "1") && redis.set(deleteManyKey2, "2") &&
        redis.deleteKeys(std::vector<std::string>{deleteManyKey1, deleteManyKey2}), "HiRedis deleteKeys");

    std::map<std::string, std::string> valueMap;
    valueMap[mapKey1] = "m1";
    valueMap[mapKey2] = "m2";
    check(redis.setMap(valueMap), "HiRedis setMap");
    check(!redis.setMap(std::map<std::string, std::string>()), "HiRedis setMap empty false");
    std::map<std::string, std::string> getMap = redis.mGet(std::vector<std::string>{mapKey1, mapKey2})->toMap();
    check(getMap[mapKey1] == "m1" && getMap[mapKey2] == "m2", "HiRedis mGet toMap");
    std::map<std::string, std::string> getValuesMap = redis.getValues(std::vector<std::string>{mapKey1, mapKey2})->toMap();
    check(getValuesMap[mapKey1] == "m1" && getValuesMap[mapKey2] == "m2", "HiRedis getValues");
    std::map<std::string, std::string> missingMap = redis.mGet(std::vector<std::string>{prefix + "missing", mapKey1})->toMap();
    check(missingMap.find(prefix + "missing") == missingMap.end() && missingMap[mapKey1] == "m1", "HiRedis mGet missing nil skip");

    check(redis.command("MULTI")->toStatus(), "begin raw transaction before queued integer reply");
    check(!redis.hSet(hashKey, "pending", "value"), "queued hash write has no integer completion reply");
    check(redis.command("DISCARD")->toStatus() && redis.hGet(hashKey, "pending")->isNull(),
        "discarded hash write has no observable effect");
    check(redis.hSet(hashKey, "field1", "value1"), "HiRedis hSet");
    std::map<std::string, std::string> hashMap;
    hashMap["field2"] = "value2";
    hashMap["field3"] = "value3";
    check(redis.hmSet(hashKey, hashMap), "HiRedis hmSet");
    check(redis.setHashMap(hashKey, hashMap), "HiRedis setHashMap");
    check(!redis.hmSet(hashKey, std::map<std::string, std::string>()), "HiRedis hmSet empty false");
    check(redis.hGet(hashKey, "field1")->toString() == "value1", "HiRedis hGet");
    std::map<std::string, std::string> readHash = redis.hGetAll(hashKey)->toHashMap();
    check(readHash["field2"] == "value2" && readHash["field3"] == "value3", "HiRedis hGetAll");
    check(redis.hmGet(hashKey, std::vector<std::string>{"field1", "field2"})->toMap()["field1"] == "value1", "HiRedis hmGet");
    check(redis.getHashValues(hashKey)->toHashMap()["field1"] == "value1", "HiRedis getHashValues all");
    check(redis.getHashValues(hashKey, std::vector<std::string>{"field1"})->toMap()["field1"] == "value1", "HiRedis getHashValues fields");
    check(redis.hKeys(hashKey)->toKeys().size() >= 3, "HiRedis hKeys");
    check(redis.getHashKeys(hashKey)->toKeys().size() >= 3, "HiRedis getHashKeys");
    check(redis.hashMapExist(hashKey), "HiRedis hashMapExist");
    check(!redis.hDel(hashKey, std::vector<std::string>()), "HiRedis hDel empty false");
    check(redis.deleteHashMapField(hashKey, "field3"), "HiRedis deleteHashMapField");
    check(redis.deleteHashMapFields(hashKey, std::vector<std::string>{"field2"}), "HiRedis deleteHashMapFields");
    check(redis.deleteHashMap(hashKey) && !redis.hashMapExist(hashKey), "HiRedis deleteHashMap");

    std::vector<std::vector<std::string>> table;
    table.push_back(std::vector<std::string>{"row1", "name1", "score1"});
    table.push_back(std::vector<std::string>{"row2", "name2", "score2"});
    check(redis.setTable(tableKey, table, 0, "|"), "HiRedis setTable");
    check(!redis.setTable(prefix + "empty_table", std::vector<std::vector<std::string>>(), 0, "|"), "HiRedis setTable empty false");
    check(redis.tableExist(tableKey), "HiRedis tableExist");
    std::vector<std::vector<std::string>> tableRead = redis.getTableValues(tableKey)->toTable(0, "|");
    check(tableRead.size() == 2 && tableRead[0].size() == 3, "HiRedis getTableValues");
    std::vector<std::vector<std::string>> tableMissingRead = redis.getTableValues(tableKey, std::vector<std::string>{prefix + "missing", "row1"})->toTable(0, "|");
    check(tableMissingRead.size() == 1 && !tableMissingRead[0].empty() && tableMissingRead[0][0] == "row1", "HiRedis getTableValues missing nil skip");
    const std::vector<std::vector<std::string>> emptyColumns = {
        {"", "row3", "", "tail", ""}, {"", "row4", "", "", ""}
    };
    check(redis.setTable(tableKey, emptyColumns, 1, "||"), "HiRedis setTable accepts empty cells");
    check(redis.getTableValues(tableKey, {"row3", "row4"})->toTable(1, "||") == emptyColumns,
        "HiRedis table preserves leading repeated and trailing empty cells");

    check(redis.setGroup(groupKey, "a"), "HiRedis setGroup");
    check(redis.setGroups(groupKey, std::vector<std::string>{"b", "c"}), "HiRedis setGroups");
    check(!redis.setGroups(groupKey, std::vector<std::string>()), "HiRedis setGroups empty false");
    check(redis.groupExist(groupKey), "HiRedis groupExist");
    check(redis.groupElementExist(groupKey, "b"), "HiRedis groupElementExist");
    check(redis.getGroupCount(groupKey) >= 3, "HiRedis getGroupCount");
    check(redis.getGroup(groupKey)->toGroup().size() >= 3, "HiRedis getGroup");
    check(redis.deleteGroupElement(groupKey, "c"), "HiRedis deleteGroupElement");
    check(!redis.deleteGroupElements(groupKey, std::vector<std::string>()), "HiRedis deleteGroupElements empty false");
    check(redis.setGroups(groupKey, std::vector<std::string>{"d", "e"}), "HiRedis setGroups for deleteGroupElements");
    check(redis.deleteGroupElements(groupKey, std::vector<std::string>{"d", "e"}), "HiRedis deleteGroupElements");

    check(redis.setOrderGroup(orderKey, 10, "oa"), "HiRedis setOrderGroup");
    check(redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>{std::make_pair(20, "ob"), std::make_pair(30, "oc")}), "HiRedis setOrderGroups");
    check(!redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>()), "HiRedis setOrderGroups empty false");
    check(redis.orderGroupExist(orderKey), "HiRedis orderGroupExist");
    check(redis.orderGroupElementExist(orderKey, "ob"), "HiRedis orderGroupElementExist");
    check(redis.getOrderGroupElementIndex(orderKey, "ob") >= 0, "HiRedis getOrderGroupElementIndex");
    check(redis.getOrderGroupElementScore(orderKey, "ob") == 20, "HiRedis getOrderGroupElementScore");
    check(redis.getOrderGroupCount(orderKey) >= 3, "HiRedis getOrderGroupCount");
    check(redis.getOrderGroup(orderKey)->toKeys().size() >= 3, "HiRedis getOrderGroup");
    check(redis.getOrderGroupByIndex(orderKey, 0, -1)->toKeys().size() >= 3, "HiRedis getOrderGroupByIndex");
    check(redis.getOrderGroupByScore(orderKey, 10, 30)->toKeys().size() >= 3, "HiRedis getOrderGroupByScore");
    check(redis.getOrderGroupCountByScore(orderKey, 10, 30) >= 3, "HiRedis getOrderGroupCountByScore");
    check(redis.deleteOrderGroupElement(orderKey, "oc"), "HiRedis deleteOrderGroupElement");
    check(!redis.deleteOrderGroupElements(orderKey, std::vector<std::string>()), "HiRedis deleteOrderGroupElements empty false");
    check(redis.setOrderGroups(orderKey, std::vector<std::pair<int32_t, std::string>>{std::make_pair(40, "od"), std::make_pair(50, "oe"), std::make_pair(60, "of")}), "HiRedis setOrderGroups for range delete");
    check(redis.deleteOrderGroupElements(orderKey, std::vector<std::string>{"od"}), "HiRedis deleteOrderGroupElements");
    check(redis.deleteOrderGroupElementsByScore(orderKey, 50, 50) == 1, "HiRedis deleteOrderGroupElementsByScore");
    check(redis.deleteOrderGroupElementsByIndex(orderKey, 0, 0) >= 1, "HiRedis deleteOrderGroupElementsByIndex");

    std::vector<HiRedisCommand> pipelineCommands;
    pipelineCommands.push_back(HiRedisCommand(std::vector<std::string>{"SET", prefix + "pipeline", "pv"}));
    pipelineCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", prefix + "pipeline"}));
    std::vector<std::shared_ptr<HiRedisResultSet>> pipelineResult = redis.pipeline(pipelineCommands);
    check(pipelineResult.size() == 2 && pipelineResult[0]->toStatus() && pipelineResult[1]->toString() == "pv", "HiRedis pipeline");
    check(redis.pipeline(std::vector<HiRedisCommand>()).empty(), "HiRedis pipeline empty");
    std::vector<HiRedisCommand> invalidPipelineCommands;
    invalidPipelineCommands.push_back(HiRedisCommand());
    std::vector<std::shared_ptr<HiRedisResultSet>> invalidPipelineResult = redis.pipeline(invalidPipelineCommands);
    check(invalidPipelineResult.size() == 1 && invalidPipelineResult[0]->isError() && redis.ping(), "HiRedis pipeline invalid command safe");

    const HiRedisCommand invalidGet(std::vector<std::string>{"GET"});
    const HiRedisCommand pingCommand(std::vector<std::string>{"PING"});
    const auto mixed = redis.pipeline({invalidGet, pingCommand});
    check(mixed.size() == 2 && mixed[0]->isError() && mixed[1]->toString() == "PONG" &&
        redis.lastError() == mixed[0]->error(), "pipeline drains replies and reports server command error");
    const auto cleanBatch = redis.pipeline({pingCommand});
    check(cleanBatch.size() == 1 && cleanBatch[0]->toString() == "PONG" && redis.lastError().empty(),
        "successful pipeline clears earlier batch error");
    const auto rejected = redis.execTransaction({pingCommand, invalidGet});
    check(rejected->isError() && redis.lastError() == rejected->error() && redis.ping(),
        "transaction preserves queue error after discard and releases session");

    std::vector<HiRedisCommand> transactionCommands;
    transactionCommands.push_back(HiRedisCommand(std::vector<std::string>{"SET", transactionKey, "tv"}));
    transactionCommands.push_back(HiRedisCommand(std::vector<std::string>{"GET", transactionKey}));
    std::shared_ptr<HiRedisResultSet> transactionResult = redis.execTransaction(transactionCommands);
    check(transactionResult && transactionResult->isValid() && transactionResult->size() == 2 &&
        transactionResult->at(0) == "OK" && transactionResult->at(1) == "tv" &&
        redis.get(transactionKey)->toString() == "tv", "HiRedis execTransaction executes every queued command");
    const auto partial = redis.execTransaction({HiRedisCommand({"HGET", transactionKey, "field"}),
        HiRedisCommand({"ECHO", "WRONGTYPE"}), HiRedisCommand({"GET", transactionKey})});
    check(partial && !partial->isError() && partial->size() == 3 && partial->isErrorAt(0) &&
        partial->at(0).find("WRONGTYPE") == 0 && !partial->isErrorAt(1) && partial->at(1) == "WRONGTYPE" &&
        !partial->isErrorAt(2) && partial->at(2) == "tv" && !partial->isErrorAt(3) &&
        !redis.get(transactionKey)->isErrorAt(0), "EXEC reports individual errors and continues later commands");
    const auto nested = redis.execTransaction({HiRedisCommand({"SET", transactionKey, "changed"}), HiRedisCommand({"eXeC"})});
    check(nested->isError() && redis.lastError() == nested->error() && redis.get(transactionKey)->toString() == "tv",
        "transaction rejects nested EXEC before any write");
    bool localeReady = true;
#ifdef _WIN32
    const std::string savedCType = std::setlocale(LC_CTYPE, nullptr);
    localeReady = std::setlocale(LC_CTYPE, "tr-TR") != nullptr;
#endif
    const auto discard = redis.execTransaction({HiRedisCommand({"SET", transactionKey, "changed"}), HiRedisCommand({"discard"})});
#ifdef _WIN32
    localeReady = std::setlocale(LC_CTYPE, savedCType.c_str()) != nullptr && localeReady;
#endif
    check(localeReady && discard->isError() && discard->error() == "redis transaction cannot contain session control commands" &&
        redis.get(transactionKey)->toString() == "tv", "Redis command names are ASCII independent of process locale");
    std::shared_ptr<HiRedisResultSet> invalidTransactionResult = redis.execTransaction(invalidPipelineCommands);
    check(invalidTransactionResult.get() != nullptr && invalidTransactionResult->isError() && redis.ping(), "HiRedis execTransaction invalid command safe");

    HiRedisConnectionPool emptyPool;
    check(!emptyPool.init(config, 0), "HiRedisConnectionPool init zero false");
    check(emptyPool.acquire(1).get() == nullptr, "HiRedisConnectionPool acquire before init false");
    emptyPool.uninit();
    emptyPool.uninit();
    check(emptyPool.totalCount() == 0 && emptyPool.availableCount() == 0, "HiRedisConnectionPool duplicate uninit without init safe");
    {
        HiRedisConfig limits = config;
        HiRedisConnectionPool elastic;
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
            std::unique_ptr<HiRedisConnectionPool> owner(new HiRedisConnectionPool);
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
            HiRedis invalid;
            check(!invalid.connect(limits) && !invalid.lastError().empty(), "reject port outside TCP range");
        }
    }

    {
        HiRedisConnectionPool closing;
        check(closing.init(config, 1), "fixed pool starts a new generation");
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
        const bool reopened = closing.init(config, 1);
        check(!pending.get(), "closing wakes old generation without lending a session");
        check(reopened && closing.acquire(0) != nullptr,
            "new generation accepts new borrowers");
    }

    HiRedisConnectionPool pool;
    bool poolOk = pool.init(config, 8);
    check(poolOk, "HiRedisConnectionPool init");
    if (poolOk)
    {
        check(pool.init(config, 8), "HiRedisConnectionPool double init no-op");
        check(pool.totalCount() == 8 && pool.availableCount() == 8, "HiRedisConnectionPool double init keep old pool");
        std::shared_ptr<HiRedis> poolRedis = pool.acquire(1000);
        check(poolRedis.get() != nullptr && poolRedis->ping(), "HiRedisConnectionPool acquire");
        poolRedis.reset();
        check(pool.availableCount() == 8, "HiRedisConnectionPool release");

        HiRedisConnectionPool onePool;
        bool onePoolOk = onePool.init(config, 1);
        check(onePoolOk, "HiRedisConnectionPool single init");
        if (onePoolOk)
        {
            std::shared_ptr<HiRedis> holdConnection = onePool.acquire(1000);
            std::shared_ptr<HiRedis> timeoutConnection = onePool.acquire(1);
            check(holdConnection.get() != nullptr && timeoutConnection.get() == nullptr, "HiRedisConnectionPool acquire timeout");
            holdConnection.reset();
            onePool.uninit();
        }

        HiRedisConnectionPool borrowedPool;
        bool borrowedPoolOk = borrowedPool.init(config, 1);
        check(borrowedPoolOk, "HiRedisConnectionPool borrowed init");
        if (borrowedPoolOk)
        {
            std::shared_ptr<HiRedis> borrowedConnection = borrowedPool.acquire(1000);
            check(borrowedConnection.get() != nullptr && borrowedPool.init(config, 1), "HiRedisConnectionPool double init with borrowed no-op");
            check(borrowedPool.totalCount() == 1 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool double init keep borrowed state");
            borrowedPool.uninit();
            borrowedPool.uninit();
            check(borrowedPool.totalCount() == 1 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool double uninit kept borrowed state");
            borrowedConnection.reset();
            check(borrowedPool.totalCount() == 0 && borrowedPool.availableCount() == 0, "HiRedisConnectionPool borrowed release after uninit");
        }

        HiRedisConnectionPool* destroyPool = new HiRedisConnectionPool;
        bool destroyPoolOk = destroyPool->init(config, 1);
        check(destroyPoolOk, "HiRedisConnectionPool destroy with borrowed init");
        if (destroyPoolOk)
        {
            std::shared_ptr<HiRedis> borrowedAfterDestroy = destroyPool->acquire(1000);
            check(borrowedAfterDestroy.get() != nullptr, "HiRedisConnectionPool acquire before destroy");
            delete destroyPool;
            borrowedAfterDestroy.reset();
            check(!borrowedAfterDestroy && redis.ping(), "HiRedisConnectionPool borrowed release after pool destroy safe");
        }
        else
        {
            delete destroyPool;
        }

        HiRedisConnectionPool* idleDestroyPool = new HiRedisConnectionPool;
        bool idleDestroyPoolOk = idleDestroyPool->init(config, 1);
        check(idleDestroyPoolOk, "HiRedisConnectionPool destroy with idle init");
        delete idleDestroyPool;
        check(redis.ping(), "HiRedisConnectionPool idle destroy safe");

        std::atomic<int32_t> threadFailCount(0);
        std::vector<std::thread> threads;
        for (int32_t threadIndex = 0; threadIndex < 6; ++threadIndex)
        {
            threads.push_back(std::thread([threadIndex, &pool, &prefix, &threadFailCount]() -> void
            {
                for (int32_t loopIndex = 0; loopIndex < 40; ++loopIndex)
                {
                    std::shared_ptr<HiRedis> connection = pool.acquire(10000);
                    if (connection.get() == nullptr)
                    {
                        threadFailCount.fetch_add(1);
                        continue;
                    }
                    std::ostringstream keyStream;
                    keyStream << prefix << "thread_" << threadIndex << "_" << loopIndex;
                    std::string key = keyStream.str();
                    if (!connection->set(key, "thread") || connection->get(key)->toString() != "thread")
                    {
                        threadFailCount.fetch_add(1);
                    }
                }
            }));
        }
        for (size_t i = 0; i < threads.size(); ++i)
        {
            threads[i].join();
        }
        check(threadFailCount.load() == 0, "HiRedisConnectionPool multithread pressure");
        pool.uninit();
    }
    else
    {
        skip("HiRedisConnectionPool multithread pressure");
    }

    std::vector<std::string> cleanKeys;
    cleanKeys.push_back(stringKey);
    cleanKeys.push_back(binaryKey);
    cleanKeys.push_back(rawPercentKey);
    cleanKeys.push_back(rawEscapeKey);
    cleanKeys.push_back(rawTrailingEscapeKey);
    cleanKeys.push_back(nxKey);
    cleanKeys.push_back(intKey);
    cleanKeys.push_back(mapKey1);
    cleanKeys.push_back(mapKey2);
    cleanKeys.push_back(deleteOneKey);
    cleanKeys.push_back(deleteManyKey1);
    cleanKeys.push_back(deleteManyKey2);
    cleanKeys.push_back(hashKey);
    cleanKeys.push_back(tableKey);
    cleanKeys.push_back(groupKey);
    cleanKeys.push_back(orderKey);
    cleanKeys.push_back(transactionKey);
    cleanKeys.push_back(prefix + "expire");
    cleanKeys.push_back(prefix + "pipeline");
    for (int32_t threadIndex = 0; threadIndex < 6; ++threadIndex)
    {
        for (int32_t loopIndex = 0; loopIndex < 40; ++loopIndex)
        {
            std::ostringstream keyStream;
            keyStream << prefix << "thread_" << threadIndex << "_" << loopIndex;
            cleanKeys.push_back(keyStream.str());
        }
    }
    redis.del(cleanKeys);
    redis.uninit();
    check(!redis.isConnected(), "HiRedis uninit disconnected");

    std::cout << "HiRedis test " << (failCount == 0 ? "PASS" : "FAIL") <<
        ", total=" << totalCount << ", failed=" << failCount << ", skipped=" << skipCount << std::endl;
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
