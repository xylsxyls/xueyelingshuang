#pragma once
#include "HiRedisMacro.h"
#include <string>
#include <vector>

/** Redis命令参数封装，使用argv形式避免字符串拼接导致的转义问题
*/
struct HiRedisAPI HiRedisCommand
{
    /** 构造函数，创建空命令
    */
    HiRedisCommand();

    /** 构造函数
    @param [in] argv Redis命令及参数列表，第一个元素通常是命令名
    */
    explicit HiRedisCommand(const std::vector<std::string>& argv);

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // Redis命令及参数，按hiredis argv方式发送
    std::vector<std::string> m_args;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};