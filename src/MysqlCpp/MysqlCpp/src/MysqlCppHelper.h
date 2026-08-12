#pragma once

#include <stdint.h>
#include <string>

namespace sql
{
    class SQLException;
}

/** MySQL封装内部工具类，集中放置多个类会复用但不适合放入CSystem的辅助逻辑
*/
class MysqlCppHelper
{
public:
    /** 获取SQL文本中第一个有效单词，会跳过开头空白、行注释和块注释
    @param [in] sql SQL文本
    @return 返回大写后的第一个SQL单词，无法识别时返回空字符串
    */
    static std::string firstSqlWord(const std::string& sql);

    /** 判断SQL是否应该走查询接口
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