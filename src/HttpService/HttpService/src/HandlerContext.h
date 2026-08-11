#pragma once
#include <memory>
#include <stdint.h>

class HttpHandler;
class HttpService;

/** HTTP路由回调上下文，传给底层HTTP库用于定位服务对象和路由处理器
*/
class HandlerContext
{
public:
    /** 构造函数，初始化回调指针为空，避免异常路径下出现野指针
    */
    HandlerContext();

    /** 构造函数，绑定所属HTTP服务和路由处理器
    @param [in] service 所属HTTP服务对象，不负责释放，生命周期必须长于当前上下文
    @param [in] handler 当前路由处理器，内部会持有智能指针保证回调期间有效
    */
    HandlerContext(HttpService* service, const std::shared_ptr<HttpHandler>& handler);

    /** 调用当前上下文绑定的路由处理器
    @param [in] conn 底层HTTP连接，具体类型由HttpService.cpp解释
    @return 返回1表示请求已处理，返回0表示未处理
    */
    int32_t operator()(void* conn) const;

private:
    /** 禁止拷贝构造，避免底层回调上下文被复制后出现重复管理语义
    @param [in] other 另一个HTTP路由回调上下文
    */
    HandlerContext(const HandlerContext& other);

    /** 禁止赋值，避免回调上下文所属服务和处理器被覆盖
    @param [in] other 另一个HTTP路由回调上下文
    @return 返回当前对象引用
    */
    HandlerContext& operator=(const HandlerContext& other);

private:
    // 所属HTTP服务对象
    HttpService* m_service;
    // 当前路由绑定的处理器，持有引用保证回调期间对象有效
    std::shared_ptr<HttpHandler> m_handler;
};