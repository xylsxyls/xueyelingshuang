#pragma once
#include <atomic>
#include <memory>
#include <mutex>
#include <stdint.h>
#include <string>
#include <utility>
#include <vector>
#include "HttpRequest.h"
#include "HttpResponse.h"
#include "HttpServiceOptions.h"
#include "HttpServiceMacro.h"

class HttpHandler;
class HttpInterface;
class HandlerContext;
struct mg_context;

/** 基于civetweb的HTTP服务封装，负责路由注册、请求解析和响应发送
*/
class HttpServiceAPI HttpService
{
public:
    /** 构造函数，创建未启动的HTTP服务
    */
    HttpService();

    /** 析构函数，停止服务并释放内部资源
    */
    ~HttpService();

public:
    /** 注册路由处理器，推荐业务使用该接口，HttpService会持有handler生命周期
    @param [in] uri 路由路径，例如/api/recipes
    @param [in] handler 处理器智能指针，服务运行期会持有引用
    @return 返回是否注册成功
    */
    bool addHandler(const std::string& uri, const std::shared_ptr<HttpHandler>& handler);

    /** 注册路由处理器，兼容旧代码的裸指针入口，不接管handler生命周期
    @param [in] uri 路由路径，例如/api/recipes
    @param [in] handler 处理器对象，调用者负责保证生命周期长于服务运行期
    @return 返回是否注册成功
    */
    bool addHandler(const std::string& uri, HttpHandler* handler);

    /** 清空全部路由处理器并停止服务
    */
    void clearHandlers();

    /** 启动HTTP服务
    @param [in] options 服务启动配置
    @return 返回是否启动成功
    */
    bool start(const HttpServiceOptions& options);

    /** 停止HTTP服务
    */
    void stop();

    /** 判断HTTP服务是否正在运行
    @return 返回服务是否已启动且未停止
    */
    bool isRunning() const;

    /** 获取最近一次错误信息
    @return 返回错误文本，没有错误返回空字符串
    */
    std::string lastError();

    /** 兼容旧接口的启动入口，注册HttpInterface列表并启动服务
    @param [in] port 监听端口
    @param [in] vecInterface 路由路径和接口对象列表
    */
    void run(uint16_t port, const std::vector<std::pair<std::string, HttpInterface*>>& vecInterface);

    /** 兼容旧接口风格的智能指针启动入口，HttpService会持有接口对象生命周期
    @param [in] port 监听端口
    @param [in] vecInterface 路由路径和接口对象智能指针列表
    */
    void run(uint16_t port, const std::vector<std::pair<std::string, std::shared_ptr<HttpInterface>>>& vecInterface);

    /** 阻塞当前线程，直到服务停止
    */
    void loop();

    /** 内部路由上下文使用的连接处理入口，普通业务代码不需要直接调用
    @param [in] conn 底层HTTP连接，具体类型由HttpService.cpp解释
    @param [in] handler 当前路由处理器
    @return 返回1表示请求已处理，返回0表示未处理
    */
    int32_t handleContextConnection(void* conn, const std::shared_ptr<HttpHandler>& handler);

protected:
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 旧接口Run入口注册的路由和处理对象
    std::vector<std::pair<std::string, HttpInterface*>> m_vecInterface;
#ifdef _MSC_VER
#pragma warning(pop)
#endif

private:
    /** 拷贝构造函数禁用，服务对象不能被复制
    @param [in] other 另一个HTTP服务对象
    */
    HttpService(const HttpService& other);

    /** 赋值函数禁用，服务对象不能被复制
    @param [in] other 另一个HTTP服务对象
    @return 不会被调用
    */
    HttpService& operator=(const HttpService& other);

    /** 注册单个路由到civetweb
    @param [in] uri 路由路径
    @param [in] handler 路由处理器
    @return 返回是否注册成功
    */
    bool registerRoute(const std::string& uri, const std::shared_ptr<HttpHandler>& handler);

    /** 处理单个HTTP连接
    @param [in] conn 底层HTTP连接，cpp内部转换为civetweb连接对象
    @param [in] handler 当前路由处理器
    @return 返回1表示请求已处理
    */
    int32_t handleConnection(void* conn, const std::shared_ptr<HttpHandler>& handler);

    /** 从底层HTTP连接解析请求
    @param [in] conn 底层HTTP连接，cpp内部转换为civetweb连接对象
    @param [in] request 输出请求对象
    @param [in] errorResponse 解析失败时输出的错误响应
    @return 返回是否解析成功
    */
    bool buildRequest(void* conn, HttpRequest& request, HttpResponse& errorResponse);

    /** 根据服务配置写入跨域响应头
    @param [in] conn 底层HTTP连接，cpp内部转换为civetweb连接对象
    @param [in] response 要发送的响应对象
    */
    void writeCorsHeaders(void* conn, const HttpResponse& response) const;

    /** 将响应写回底层HTTP连接
    @param [in] conn 底层HTTP连接，cpp内部转换为civetweb连接对象
    @param [in] response 响应对象
    */
    void writeResponse(void* conn, const HttpResponse& response);

private:
    // 底层civetweb服务上下文
    struct mg_context* m_context;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 服务是否正在运行
    std::atomic<bool> m_running;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
    // 当前服务启动配置
    HttpServiceOptions m_options;
#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4251)
#endif
    // 保护服务启动、停止、路由注册和错误信息的递归锁
    std::recursive_mutex m_mutex;
    // 最近一次服务错误信息
    std::string m_lastError;
    // 已注册的路由和处理器，持有智能指针保证回调期间对象有效
    std::vector<std::pair<std::string, std::shared_ptr<HttpHandler>>> m_routes;
    // 已分配给civetweb回调使用的路由上下文
    std::vector<std::shared_ptr<HandlerContext>> m_contexts;
#ifdef _MSC_VER
#pragma warning(pop)
#endif
};