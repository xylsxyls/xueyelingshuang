#include "PlatformTest/PlatformTestAPI.h"
#include "SqlStringTest.h"
#include <cstring>
#include "SqlString/SqlStringAPI.h"
#include <cstdio>
#define CHECK(expression) do { if (!(expression)) { std::fprintf(stderr, "FAIL line %d: %s\n", __LINE__, #expression); return 1; } } while (false)
/** 对本库的一项固定规模操作连续采样，结果用于同平台的改动前后比较。 */
static void RunBenchmark()
{
    const std::string table("items");
    std::string fields("f0");
    for (int index = 1; index < 32; ++index)
    {
        fields += ",field" + std::to_string(index);
    }
    LibraryBenchmark::run("SqlString", [&](uint32_t count) -> uint64_t
    {
        uint64_t checksum = 0;
        for (uint32_t index = 0; index < count; ++index)
        {
            checksum += SqlString::insertString(table, fields).size();
        }
        return checksum;
    });
}

/** 核对数据操作语句，占位符和文本拒绝条件。 */
static int TestDml()
{
    const std::string t = "items";
    CHECK(SqlString::insertString(t, "id") == "insert into `items`(id) values(?)");
    CHECK(SqlString::insertString(t, "id,name") == "insert into `items`(id,name) values(?,?)");
    CHECK(SqlString::insertString(t, ",x,") == "insert into `items`(,x,) values(?,?,?)");
    CHECK(SqlString::insertString(t, std::string("id\0x,y", 6)).empty());
    CHECK(SqlString::insertString(std::string("table\0shadow", 12), "id").empty());
    CHECK(SqlString::updateString(t, "id,name") == "update `items` set id=?,name=?");
    CHECK(SqlString::updateString(t, "name", "id=?") == "update `items` set name=? where id=?");
    CHECK(SqlString::deleteString(t, "id=?") == "delete from `items` where id=?");
    CHECK(SqlString::selectString(t) == "select * from `items`");
    CHECK(SqlString::selectString(t, "id", "name=?") == "select id from `items` where name=?");
    const std::string invalidText("a\0b", 3);
    CHECK(SqlString::updateString(invalidText, "id", "id=?").empty());
    CHECK(SqlString::updateString(t, invalidText, "id=?").empty());
    CHECK(SqlString::updateString(t, "id", invalidText).empty());
    CHECK(SqlString::deleteString(invalidText, "id=?").empty());
    CHECK(SqlString::deleteString(t, invalidText).empty());
    CHECK(SqlString::selectString(invalidText).empty());
    CHECK(SqlString::selectString(t, invalidText).empty());
    CHECK(SqlString::selectString(t, "id", invalidText).empty());
    return 0;
}

/** 核对 DDL 及表达式的完整文本。 */
static int TestSchema()
{
    const std::string t = "items";
    const std::vector<std::string> emptyFields;
    CHECK(SqlString::createTableString(t, emptyFields).empty());
    CHECK(SqlString::createTableIfNotExistString(t, emptyFields).empty());
    const std::vector<std::string> fields = {"id INT", "name TEXT"};
    CHECK(SqlString::createTableString(t, fields) == "create table `items`(id INT,name TEXT);");
    CHECK(SqlString::createTableIfNotExistString(t, fields) == "create table if not exists `items`(id INT,name TEXT);");
    CHECK(SqlString::createDatabaseString(t) == "create database `items`");
    CHECK(SqlString::destroyTableString(t) == "drop table `items`");
    CHECK(SqlString::destroyDatabaseString(t) == "drop database `items`");
    CHECK(SqlString::clearTableString(t) == "truncate table `items`");
    CHECK(SqlString::sqlMin("id") == "MIN(id)" && SqlString::sqlMax("id") == "MAX(id)");
    CHECK(SqlString::strToDate("2024-02-29") == "STR_TO_DATE('2024-02-29', '%Y-%m-%d')");
    CHECK(SqlString::fieldStrToDate("d") == "STR_TO_DATE(d, '%Y-%m-%d')");
    CHECK(SqlString::selectAllTableName(t) == "select table_name from information_schema.tables where table_schema='items'");
    return 0;
}

/** 按稳定功能用例号或显式性能参数运行。 */
int main(int argc, char** argv)
{
    if (argc == 2 && std::strcmp(argv[1], "--benchmark") == 0)
    {
        RunBenchmark();
        return 0;
    }
    const LibraryTestCase cases[] = {
        {1, false, TestDml}, {2, false, TestSchema}
    };
    return LibraryCaseSelection::run(argc, argv, cases);
}