#pragma once
#include <stdint.h>
#include <string>
#include "HttpServiceMacro.h"

/** HTTP服务启动配置，用于控制监听地址、线程数和请求限制
*/
struct HttpServiceAPI HttpServiceOptions
{
    /** 构造函数，填充适合普通业务服务的默认配置
    */
    HttpServiceOptions();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 监听IP，0.0.0.0表示监听全部网卡
    std::string m_listenHost;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 监听端口，0表示配置非法
    uint16_t m_port;
    // civetweb工作线程数量，小于等于0时启动时修正为默认值
    int32_t m_workerThreads;
    // 单个请求超时时间，单位毫秒
    int32_t m_requestTimeoutMs;
    // 允许读取的最大请求体大小，单位字节
    int64_t m_maxRequestBodyBytes;
    // 是否允许HTTP长连接
    bool m_enableKeepAlive;
    // 是否自动补充跨域响应头
    bool m_enableCors;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 跨域允许的来源，m_enableCors为true时生效
    std::string m_corsAllowOrigin;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};