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
    // MySQL服务器IP或域名
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
    // 登录账号
    std::string m_account;
    // 登录密码
    std::string m_password;
    // 连接成功后默认选择的数据库名，空字符串表示不自动选择
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
};