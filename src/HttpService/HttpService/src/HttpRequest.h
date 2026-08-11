#pragma once
#include <stdint.h>
#include <map>
#include <string>
#include "HttpServiceMacro.h"

/** HTTP请求数据对象，由HttpService从底层连接中解析后传给业务处理器
*/
struct HttpServiceAPI HttpRequest
{
    /** 构造函数，初始化数值字段为安全默认值
    */
    HttpRequest();

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // HTTP方法，例如GET、POST、OPTIONS
    std::string m_method;
    // 请求路径，不包含查询字符串
    std::string m_uri;
    // 原始查询字符串，不包含问号
    std::string m_queryString;
    // 请求体内容
    std::string m_body;
    // 客户端IP地址
    std::string m_remoteIp;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 客户端端口
    int32_t m_remotePort;
    // 请求体长度，单位字节
    int64_t m_contentLength;
    // 当前请求是否通过SSL连接进入
    bool m_isSsl;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // HTTP请求头映射，键名保持客户端传入形式
    std::map<std::string, std::string> m_headers;
    // URL解码后的查询参数映射
    std::map<std::string, std::string> m_query;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};