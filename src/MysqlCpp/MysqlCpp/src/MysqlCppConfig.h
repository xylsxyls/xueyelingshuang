#pragma once
#include "MysqlCppMacro.h"
#include <stdint.h>
#include <string>

/** MySQL连接配置，描述一次连接建立时需要的地址、账号、库名和超时策略
*/
struct MysqlCppAPI MysqlCppConfig
{
public:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // MySQL 服务器 IP 或域名，非空，无空白、ASCII 控制字符或斜杠
    std::string m_ip;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // MySQL 监听端口，范围 1～65535，默认 3306
    int32_t m_port;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 登录账号，不能含 NUL
    std::string m_account;
#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 登录密码，不能含 NUL
    std::string m_password;
#ifdef _MSC_VER
#pragma warning(pop)
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 连接成功后默认选择的数据库名，空字符串表示不自动选择，库名不能含 NUL
    std::string m_dbName;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 是否自动提交SQL事务
    bool m_autoCommit;
    // 底层连接失效时是否尝试自动重连
    bool m_autoReconnect;
    // 建立连接超时时间，单位秒
    int32_t m_connectTimeoutSeconds;
    // 读取MySQL响应超时时间，单位秒
    int32_t m_readTimeoutSeconds;
    // 写入MySQL请求超时时间，单位秒
    int32_t m_writeTimeoutSeconds;
    // 初始化时预建、归还时保留的空闲连接数，默认 1；允许 0 以按需连接
    int32_t m_poolMinConnections;
    // 总连接上限，默认 1；-1 允许随需求增长，正值不得小于预建数量
    int32_t m_poolMaxConnections;
    // 无参数 acquire 等待池名额的毫秒数，默认 3000，0 即时返回，负数持续等待
    int32_t m_poolAcquireTimeoutMs;

public:
    /** 填入连接和池的默认参数
    */
    MysqlCppConfig();

    /** 设置连接端点，非法输入保留原值
    @param [in] host 非空独立主机名或 IP，无空白、ASCII 控制字符或斜杠。方括号须成对包住整个地址，端口单独传入
    @param [in] port TCP 端口，1～65535
    @return 端点有效时返回 true
    */
    bool setEndpoint(const std::string& host, int32_t port);

    /** 设置登录凭据，非法输入保留原值
    @param [in] account 登录账号，不含 NUL
    @param [in] password 登录密码，不含 NUL
    @return 两项均有效时返回 true
    */
    bool setCredentials(const std::string& account, const std::string& password);

    /** 设置默认数据库
    @param [in] database 库名，空串表示连接时跳过选库
    @return 不含 NUL 时保存并返回 true
    */
    bool setDatabase(const std::string& database);

    /** 设置会话初始的自动提交模式
    @param [in] enabled 是否启用自动提交
    */
    void setAutoCommit(bool enabled);

    /** 设置传入驱动的自动重连选项
    @param [in] enabled 是否允许驱动在连接失效时尝试重连
    */
    void setAutoReconnect(bool enabled);

    /** 设置三个客户端超时
    @param [in] connectSeconds 建连秒数，非正值使用默认值
    @param [in] readSeconds 读取秒数，非正值使用默认值
    @param [in] writeSeconds 写入秒数，非正值使用默认值
    */
    void setTimeouts(int32_t connectSeconds, int32_t readSeconds, int32_t writeSeconds);

    /** 同时设置池的预建数和总量上限，非法输入保留原值
    @param [in] minimum 非负的预建及空闲保留数
    @param [in] maximum 不小于 minimum 的正整数，或 -1 表示无配置上限
    @return 边界合法时返回 true
    */
    bool setPoolLimits(int32_t minimum, int32_t maximum);

    /** 设置无参数借用的等待期限
    @param [in] milliseconds 等待名额的毫秒数，0 即时返回，负数持续等待
    */
    void setPoolAcquireTimeout(int32_t milliseconds);

    /** 检查端口范围和连接文本能否传给驱动
    @return 端点满足 setEndpoint 的约束，且账号、密码及库名不含 NUL 时返回 true
    */
    bool validConnection() const;
};