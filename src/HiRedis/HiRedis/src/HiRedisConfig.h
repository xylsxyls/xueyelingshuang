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
    // Redis 服务器 IP 或域名，不能含 NUL
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
    // Redis 认证密码按完整字节长度发送，空字符串表示不认证
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
    // 是否启用TCP KeepAlive，Windows 和 Linux 均在连接时设置；设置失败则连接失败
    bool m_enableKeepAlive;
    // 初始化时预建、归还时保留的空闲连接数，默认 1；允许 0 以按需连接
    int32_t m_poolMinConnections;
    // 总连接上限，默认 1；-1 允许随需求增长，正值不得小于预建数量
    int32_t m_poolMaxConnections;
};