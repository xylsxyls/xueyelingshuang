#pragma once
#include "HiRedisMacro.h"
#include <stdint.h>
#include <string>

/** Redis连接配置，用于描述单个Redis实例的连接和命令超时策略
*/
struct HiRedisAPI HiRedisConfig
{
    /** 构造函数，填充本地Redis的默认连接参数
    */
    HiRedisConfig();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // Redis服务器IP或域名
    std::string m_ip;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // Redis监听端口
    int32_t m_port;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // Redis认证密码，空字符串表示不认证
    std::string m_password;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 连接成功后选择的数据库下标，小于0表示不切换数据库
    int32_t m_dbIndex;
    // 建立连接的超时时间，单位毫秒
    int32_t m_connectTimeoutMs;
    // 执行命令的读写超时时间，单位毫秒
    int32_t m_commandTimeoutMs;
    // 是否启用TCP KeepAlive，Windows下由hiredis支持时生效
    bool m_enableKeepAlive;
};