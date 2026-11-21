#pragma once
#include <string>
#include <vector>
#include "SqlStringMacro.h"

/** 生成 MySQL 语句与表达式，返回的字符串由调用方持有
表名、列名和 SQL 片段由调用方提供，本类只拼接文本，参数绑定与执行交给数据库接口
*/
class SqlStringAPI SqlString
{
public:
    /** 生成按列顺序绑定参数的 INSERT 语句
    @param [in] tableName 要插入的表名
    @param [in] fields 逗号分隔的列名列表，每个逗号增加一个占位符
    @return 包含对应问号占位符的 INSERT 语句，任一输入含 NUL 时返回空串
    */
    static std::string insertString(const std::string& tableName, const std::string& fields);

    /** 生成按列赋值的 UPDATE 语句
    @param [in] tableName 要更新的表名
    @param [in] fields 逗号分隔的列名列表，每列对应一个待绑定的值
    @param [in] whereString WHERE 后的条件表达式，默认为空，省略时更新整表
    @return UPDATE 语句，列值占位符在条件占位符之前，任一输入含 NUL 时返回空串
    */
    static std::string updateString(const std::string& tableName, const std::string& fields, const std::string& whereString = "");

    /** 生成按指定条件删除行的 DELETE 语句
    @param [in] tableName 要删除行的表名
    @param [in] whereString WHERE 后的条件表达式，应提供非空条件
    @return 始终带 WHERE 的 DELETE 语句，空条件会留下不完整的 WHERE 子句，任一输入含 NUL 时返回空串
    */
    static std::string deleteString(const std::string& tableName, const std::string& whereString);

    /** 生成查询指定列或表达式的 SELECT 语句
    @param [in] tableName 要查询的表名
    @param [in] selectFields 查询列或表达式列表，默认值为 *
    @param [in] whereString WHERE 后的条件表达式，默认为空，空值省略 WHERE
    @return SELECT 语句，任一输入含 NUL 时返回空串
    */
    static std::string selectString(const std::string& tableName, const std::string& selectFields = "*", const std::string& whereString = "");

    /** 生成包含全部列定义的建表语句
    @param [in] tableName 要创建的表名
    @param [in] fields 按建表顺序给出的完整列定义，每项可包含类型和约束
    @return CREATE TABLE 语句，列定义列表为空时返回空串
    */
    static std::string createTableString(const std::string& tableName, const std::vector<std::string>& fields);

    /** 生成仅在表尚不存在时建表的语句
    @param [in] tableName 要创建的表名
    @param [in] fields 按建表顺序给出的完整列定义，每项可包含类型和约束
    @return 带 IF NOT EXISTS 的 CREATE TABLE 语句，列定义列表为空时返回空串
    */
    static std::string createTableIfNotExistString(const std::string& tableName, const std::vector<std::string>& fields);

    /** 生成创建数据库的语句
    @param [in] databaseName 要创建的数据库名
    @return 以反引号包围库名的 CREATE DATABASE 语句
    */
    static std::string createDatabaseString(const std::string& databaseName);

    /** 生成删除整张表的语句
    @param [in] tableName 要删除的表名
    @return 以反引号包围表名的 DROP TABLE 语句
    */
    static std::string destroyTableString(const std::string& tableName);

    /** 生成删除整个数据库的语句
    @param [in] databaseName 要删除的数据库名
    @return 以反引号包围库名的 DROP DATABASE 语句
    */
    static std::string destroyDatabaseString(const std::string& databaseName);

    /** 生成清空表中全部数据的语句
    @param [in] tableName 要清空的表名
    @return 以反引号包围表名的 TRUNCATE TABLE 语句
    */
    static std::string clearTableString(const std::string& tableName);

    /** 生成把日期文本转换为 MySQL 日期的表达式
    @param [in] date 按 YYYY-MM-DD 排列的受信日期文本，直接写入单引号内
    @return 以日期字面量和 %Y-%m-%d 格式调用 STR_TO_DATE 的表达式
    */
    static std::string strToDate(const std::string& date);

    /** 生成把列或表达式的值转换为 MySQL 日期的表达式
    @param [in] field 产生 YYYY-MM-DD 文本的列名或 SQL 表达式，按原文插入
    @return 以给定表达式和 %Y-%m-%d 格式调用 STR_TO_DATE 的表达式
    */
    static std::string fieldStrToDate(const std::string& field);

    /** 生成计算最小值的聚合表达式
    @param [in] field 参与聚合的列名或 SQL 表达式，按原文插入
    @return 可用于查询的 MIN(field) 表达式
    */
    static std::string sqlMin(const std::string& field);

    /** 生成计算最大值的聚合表达式
    @param [in] field 参与聚合的列名或 SQL 表达式，按原文插入
    @return 可用于查询的 MAX(field) 表达式
    */
    static std::string sqlMax(const std::string& field);

    /** 生成从 information_schema.tables 查询指定库中表名的语句
    @param [in] dbName 受信数据库名，直接写入 table_schema 的单引号条件
    @return 返回 table_name 列的 SELECT 语句
    */
    static std::string selectAllTableName(const std::string& dbName);
};