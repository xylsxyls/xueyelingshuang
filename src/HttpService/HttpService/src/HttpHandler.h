#pragma once
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpServiceMacro.h"

/** HTTP请求处理器接口，业务模块通过继承它接入HttpService路由
*/
class HttpServiceAPI HttpHandler
{
public:
    /** 析构函数
    */
    virtual ~HttpHandler();

public:
    /** 处理HTTP请求并返回响应
    @param [in] request 已解析的HTTP请求对象
    @return 返回要发送给客户端的HTTP响应
    */
    virtual HttpResponse handle(const HttpRequest& request) = 0;
};