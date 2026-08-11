#pragma once
#include <stdint.h>
#include <map>
#include <string>
#include "HttpStatus.h"
#include "HttpServiceMacro.h"

/** HTTP响应数据对象，业务处理器填充后由HttpService写回客户端
*/
struct HttpServiceAPI HttpResponse
{
    /** 构造函数，默认创建成功JSON响应
    */
    HttpResponse();

    /** 构造函数，使用指定状态码创建JSON响应
    @param [in] code HTTP状态码
    */
    explicit HttpResponse(int32_t code);

    /** 获取状态码默认文本
    @param [in] code HTTP状态码
    @return 返回常见状态码对应的英文状态文本
    */
    static std::string defaultStatusText(int32_t code);

    /** 创建JSON响应
    @param [in] body 响应体内容
    @param [in] code HTTP状态码
    @return 返回JSON类型响应对象
    */
    static HttpResponse json(const std::string& bodyText, int32_t code = kHttpStatusOk);

    /** 创建纯文本响应
    @param [in] body 响应体内容
    @param [in] code HTTP状态码
    @return 返回文本类型响应对象
    */
    static HttpResponse text(const std::string& bodyText, int32_t code = kHttpStatusOk);

    /** 设置响应状态码，并同步默认状态文本
    @param [in] code HTTP状态码
    */
    void setStatus(int32_t code);

    /** 设置响应头
    @param [in] name 响应头名称
    @param [in] value 响应头内容
    */
    void setHeader(const std::string& name, const std::string& value);

    // HTTP状态码
    int32_t m_statusCode;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // HTTP状态文本，为空时发送前会按状态码补默认文本
    std::string m_statusText;
    // 响应内容类型，未显式设置Content-Type头时使用
    std::string m_contentType;
    // 响应体内容
    std::string m_body;
    // 额外响应头映射
    std::map<std::string, std::string> m_headers;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 是否在响应后关闭连接
    bool m_closeConnection;
};