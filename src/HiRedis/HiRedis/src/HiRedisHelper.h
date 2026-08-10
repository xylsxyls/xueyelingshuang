#pragma once
#include <memory>
#include <stddef.h>
#include <stdint.h>
#include <string>
#include <vector>

class HiRedisResultSet;

/** Redis封装内部工具类，集中放置多个类会复用但不适合放入CSystem的辅助逻辑
*/
class HiRedisHelper
{
public:
    /** 判断Redis整型回复是否表示命令执行成功
    @param [in] result Redis命令结果对象
    @return 返回true表示结果存在、没有错误且整型值不小于0
    */
    static bool integerReplyOk(const std::shared_ptr<HiRedisResultSet>& result);

    /** 将原始命令文本拆成Redis argv参数
    @param [in] commandText 原始命令文本，支持空白分隔、单引号、双引号和反斜杠转义
    @param [out] args 输出Redis命令参数，第一个参数为命令名
    @param [out] error 解析失败时输出错误信息
    @return 返回是否解析成功
    */
    static bool splitCommandText(const std::string& commandText, std::vector<std::string>& args, std::string& error);

    /** 将字符串参数列表转换成hiredis argv形式
    @param [in] args Redis命令参数
    @param [out] argv 输出参数地址列表，生命周期依赖args
    @param [out] argvlen 输出每个参数的字节长度
    @param [out] error 转换失败时输出错误信息
    @return 返回是否转换成功
    */
    static bool makeArgv(const std::vector<std::string>& args, std::vector<const char*>& argv, std::vector<size_t>& argvlen, std::string& error);
};