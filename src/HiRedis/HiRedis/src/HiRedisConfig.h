#pragma once
#include "HiRedisMacro.h"
#include <stdint.h>
#include <string>

/** Redis连接配置，用于描述单个Redis实例的连接和命令超时策略
*/
struct HiRedisAPI HiRedisConfig
{
public:
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
    // 默认借用等待毫秒数，零不等待，负数无限等待
    int32_t m_poolAcquireTimeoutMs;
    // 启用 TCP 保活时首次探测前的空闲秒数，默认 15
    int32_t m_keepAliveIdleSeconds;
    // 空闲读取缓冲保留阈值（字节），默认 16384，-1 关闭主动回收
    int64_t m_readerMaxUnusedBytes;
    // 单个聚合回复可声明的元素数上限，默认 4294967295，-1 取消此项限制
    int64_t m_readerMaxElements;

public:
    /** 构造本机 Redis 的默认配置
    */
    HiRedisConfig();

    /** 设置下次连接使用的服务器地址和端口
    @param [in] ip 非空 IP 或主机名，不能包含 NUL
    @param [in] port TCP 端口，范围 1～65535
    */
    void setEndpoint(const std::string& ip, int32_t port);

    /** 设置下次连接使用的密码
    @param [in] password 完整密码字节，空串跳过 AUTH
    */
    void setPassword(const std::string& password);

    /** 设置连接时选择的逻辑库
    @param [in] index 非负数据库号，负数跳过 SELECT
    */
    void setDatabase(int32_t index);

    /** 设置建立连接及套接字读写超时
    @param [in] connectMs 建连超时毫秒数，非正数使用 3000
    @param [in] commandMs 读写超时毫秒数，非正数使用 3000
    */
    void setTimeouts(int32_t connectMs, int32_t commandMs);

    /** 设置建连后是否开启 TCP KeepAlive
    @param [in] enabled 为 true 时开启套接字保活
    */
    void setKeepAlive(bool enabled);

    /** 设置启用保活时首次探测前的空闲时间
    @param [in] seconds 正秒数，转换为毫秒须能用 int32_t 表示，系统可进一步限制取值
    */
    void setKeepAliveIdleSeconds(int32_t seconds);

    /** 设置每个连接的 hiredis 回复读取器
    @param [in] maxUnusedBytes 空闲缓冲保留阈值，正字节数须适合目标 size_t，-1 关闭阈值回收
    @param [in] maxElements 单个聚合回复的正元素上限，-1 跳过此项检查，仍受系统资源限制
    */
    void setReaderLimits(int64_t maxUnusedBytes, int64_t maxElements);

    /** 设置初始化时预建的连接数、空闲保留数及总上限
    @param [in] minimum 非负预建数，同时作为空闲保留数
    @param [in] maximum 不小于 minimum 的正数，或 -1 表示按需增长
    */
    void setPoolBounds(int32_t minimum, int32_t maximum);

    /** 设置无参 acquire 等待池名额的期限
    @param [in] timeoutMs 毫秒数，零表示不等待，负数表示一直等待
    */
    void setPoolAcquireTimeout(int32_t timeoutMs);
};