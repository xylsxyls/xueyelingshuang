#pragma once
#include "HttpHandler.h"
#include <mutex>
#include <string>

/** 兼容旧接口风格的HTTP处理基类，按GET/POST分发并缓存当前请求和响应
*/
class HttpServiceAPI HttpInterface : public HttpHandler
{
public:
    /** 构造函数
    */
    HttpInterface();

    /** 析构函数
    */
    virtual ~HttpInterface();

public:
    /** 接收GET请求
    @param [in] request 查询字符串原文
    @return 返回是否已成功处理并写入响应
    */
    virtual bool httpGetReceive(const std::string& request);

    /** 接收POST请求
    @param [in] request 请求体原文
    @return 返回是否已成功处理并写入响应
    */
    virtual bool httpPostReceive(const std::string& request);

    /** 处理HTTP请求，根据方法分发到HttpGetReceive或HttpPostReceive
    @param [in] request 已解析的HTTP请求对象
    @return 返回要发送给客户端的HTTP响应
    */
    virtual HttpResponse handle(const HttpRequest& request);

    /** 将当前响应设置为成功JSON响应
    */
    void writeSuccess();

    /** 向当前响应体追加内容
    @param [in] result 要追加的响应内容
    */
    void writeResult(const std::string& result);

    /** 设置当前响应状态码和可选状态文本
    @param [in] statusCode HTTP状态码
    @param [in] statusText 状态文本，空字符串表示使用默认文本
    */
    void setStatus(int32_t statusCode, const std::string& statusText = std::string());

    /** 设置当前响应头
    @param [in] name 响应头名称
    @param [in] value 响应头内容
    */
    void setHeader(const std::string& name, const std::string& value);

    /** 获取当前正在处理的请求
    @return 返回当前请求对象引用
    */
    const HttpRequest& currentRequest() const;

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 保护当前请求和响应缓存的互斥锁
    std::mutex m_mutex;
    // 当前正在处理的HTTP请求快照
    HttpRequest m_currentRequest;
    // 当前处理函数正在构造的HTTP响应
    HttpResponse m_currentResponse;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};