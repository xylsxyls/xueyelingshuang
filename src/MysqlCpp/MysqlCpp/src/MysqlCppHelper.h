#pragma once

#include "MysqlCppConfig.h"
#include "mysql-connector-cpp/cppconn/connection.h"
#include <stdint.h>
#include <string>
#include <vector>

/** MySQL封装内部工具类，集中放置多个类会复用但不适合放入CSystem的辅助逻辑
*/
class MysqlCppHelper
{
public:
    // 绑定模板通过此别名识别驱动异常并保留错误码
    typedef sql::SQLException SqlException;

    /** 检查主机能否无歧义地作为单独的 TCP 地址传入
    @param [in] host 非空 IP 或域名，可用一对方括号包住整个地址
    @return 通过主机文本校验时返回 true；拒绝空白、控制字符、斜杠、错误方括号及无效的 IPv6 字面量
    */
    static bool validHost(const std::string& host);

    /** 为 mysql 和 mysqldump 构造独立的主机参数
    @param [in] host 与连接配置使用相同格式的 IP 或域名
    @return 去掉外层方括号的地址，格式无效时返回空串
    */
    static std::string toolHost(const std::string& host);

    /** 填写实际交给 Connector/C++ 建连的参数
    @param [in] config 已校验的连接配置
    @param [out] options 驱动参数表，覆盖本库配置的键
    */
    static void connectionOptions(const MysqlCppConfig& config, sql::ConnectOptionsMap& options);

    /** 以独立参数启动 MySQL 工具并等待退出，SQL 文件作为子进程输入或输出
    @param [in] arguments 首项为工具名或路径，后续各项原样作为独立参数
    @param [in] path SQL 文件的 UTF-8 路径
    @param [in] input true 表示从文件导入，false 表示向文件导出
    @param [in] password 数据库密码，只写入工具子进程的环境
    @return 工具的退出码，无法打开文件、启动或等待时返回 -1
    */
    static int runTool(const std::vector<std::string>& arguments, const std::string& path, bool input,
        const std::string& password);

    /** 跳过开头空白和普通注释，读取第一段未加引号的标识符字节
    @param [in] sql SQL文本
    @return 返回将 ASCII 字母转大写后的标识符，首字节为其他分隔符时返回空串
    */
    static std::string firstSqlWord(const std::string& sql);

    /** 按首个关键词粗分常见查询语句，实际执行入口按驱动的返回值处理
    @param [in] sql SQL文本
    @return 返回true表示该SQL通常会产生结果集
    */
    static bool isQuerySql(const std::string& sql);

    /** 判断SQL是否可能产生LAST_INSERT_ID
    @param [in] sql SQL文本
    @return 返回true表示该SQL通常可能产生自增ID
    */
    static bool isLastInsertIdSql(const std::string& sql);

    /** 将调用方传入的超时时间归一化为可传给底层驱动的正数秒数
    @param [in] timeoutSeconds 调用方传入的超时时间，单位秒
    @param [in] defaultSeconds timeoutSeconds无效时使用的默认值，单位秒
    @return 返回大于0的超时时间
    */
    static int32_t normalizeTimeoutSeconds(int32_t timeoutSeconds, int32_t defaultSeconds);

    /** 将本库0基参数或列下标转换成MySQL Connector使用的1基下标
    @param [in] index 调用方传入的0基下标
    @return 返回1基下标，无法转换时返回0
    */
    static uint32_t toDriverIndex(uint32_t index);

    /** 将MySQL异常转换成业务层可记录的错误文本
    @param [in] e MySQL Connector抛出的异常对象
    @return 返回包含异常说明和错误码的文本
    */
    static std::string exceptionText(const sql::SQLException& e);

    /** 给命令行路径或参数补引号并转义内部引号，避免空格或特殊字符导致执行失败
    @param [in] text 需要补引号的命令行片段
    @return 返回补引号并转义后的文本
    */
    static std::string quoteCommandText(const std::string& text);

    /** 生成MySQL命令行工具路径，优先使用MYSQL_ROOT环境变量
    @param [in] toolName 工具文件名，例如mysql或mysqldump
    @return 返回可直接放入命令行的工具路径
    */
    static std::string mysqlToolPath(const std::string& toolName);
};