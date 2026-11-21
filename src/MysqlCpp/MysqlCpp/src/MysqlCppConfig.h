#pragma once
#include "MysqlCppMacro.h"
#include <stdint.h>
#include <string>

/** MySQL连接配置，描述一次连接建立时需要的地址、账号、库名和超时策略
*/
struct MysqlCppAPI MysqlCppConfig
{
    /** 构造函数，会填入本库推荐的默认连接参数
    */
    MysqlCppConfig();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // MySQL 服务器 IP 或域名，不能含 NUL
    std::string m_ip;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // MySQL监听端口
    int32_t m_port;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 登录账号，不能含 NUL
    std::string m_account;
    // 登录密码，不能含 NUL
    std::string m_password;
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
};