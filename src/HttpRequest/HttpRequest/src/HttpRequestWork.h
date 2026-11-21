#pragma once
#include "HttpRequest.h"
#include <memory>
#include <vector>
#ifndef __EMSCRIPTEN__
#include <future>
#include <mutex>
#endif

#ifndef __EMSCRIPTEN__
struct HttpRequestWork;

/** 独占一个 easy 句柄及其连接缓存，绑定到精确的源和证书信任配置
*/
struct HttpNativeConnection
{
    /** 建立可接收新请求配置的句柄
    @param [in] origin 协议及主机、端口
    @param [in] authority 本次私有 CA PEM 或表示系统 CA 的空串
    */
    HttpNativeConnection(const std::string& origin, const std::string& authority);

    /** 关闭本句柄持有的连接
    */
    ~HttpNativeConnection();

    // 本连接独占的源，不含路径、查询和片段
    std::string m_origin;
    // 当前源使用的证书信任配置
    std::string m_authority;
    // libcurl easy，由本对象独占
    void* m_handle;

private:
    HttpNativeConnection(const HttpNativeConnection& other);
    HttpNativeConnection& operator=(const HttpNativeConnection& other);
};

// 原生请求的工作线程集合，停止时等待正在执行的回调退出。
class HttpNativeWorkers
{
public:
    /** 创建可接纳请求的原生线程集合
    */
    HttpNativeWorkers();

    /** 等待未结束的工作和已进入的完成回调
    */
    ~HttpNativeWorkers();

    /** 关闭接纳入口并等待，必须从完成回调以外的线程调用
    */
    void stop();

    /** 发送原生请求并交付一次结果
    @param [in] work 正在工作线程中运行的请求记录
    */
    void send(const HttpRequestWork& work);

public:
    // 由请求登记表的互斥锁保护
    bool m_stopping;
    // 已启动的异步任务及其完成状态
    std::vector<std::future<void>> m_workers;

private:
    /** 工作者已全部返回后释放空闲连接及本库取得的 libcurl 全局引用
    */
    void closeTransport();

private:
    // 串行化多个所有者的 stop 调用
    std::mutex m_shutdownMutex;
    // 仅保护一个空闲句柄的移交，网络传输和回调在锁外执行
    std::mutex m_connectionMutex;
    // 最多保留一个空闲 easy 句柄，每个句柄最多保留一条连接
    std::unique_ptr<HttpNativeConnection> m_idle;
    // 进程内此封装只初始化一次 libcurl
    std::once_flag m_initOnce;
    // 全局初始化成功时为 true，停止时清除
    bool m_initialized;
};

// libcurl的回调在单一工作线程内填写此响应，m_limit为正文长度上限。
struct HttpTransfer
{
public:
    // 该连接当前收到的状态、正文和可返回的响应头
    HttpClientResponse m_response;
    // 允许积累的最大正文大小，单位字节
    size_t m_limit;

public:
    /** 创建接收上下文
    @param [in] limit 最大响应字节数
    */
    explicit HttpTransfer(size_t limit);
};
#endif

// 内部在途请求，完成通知由登记表保护；运输层仅借用不可变输入。
struct HttpRequestWork
{
    // 登记表中的非零标识
    uint32_t m_id;
    // true 发送 POST，false 发送 GET
    bool m_post;
    // 登记时判定是否允许缓存本次原生传输的信任材料
    bool m_cacheAuthority;
    // 登记时复制的目标地址
    std::string m_url;
    // POST 二进制正文
    std::string m_body;
    // 此次传输使用的选项快照
    HttpClientOptions m_options;
    // 同一在途传输的各个调用方
    std::vector<HttpRequestClient::Completion> m_callbacks;
};

#ifdef __EMSCRIPTEN__
/** 在 Wasm 宿主事件循环中发送一个已登记的请求
@param [in] work 发送期间有效的请求记录
*/
void HttpRequestSend(const HttpRequestWork& work);
#endif

/** 交付一次结果，先释放在途记录再执行回调，允许回调重入
@param [in] id 登记时分配的标识
@param [in] response 本次传输的完成状态及正文
*/
void HttpRequestFinish(uint32_t id, const HttpClientResponse& response);