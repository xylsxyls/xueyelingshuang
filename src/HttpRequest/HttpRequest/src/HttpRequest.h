#pragma once
#include "HttpRequestMacro.h"
#include <stdint.h>
#include <functional>
#include <string>

// 原生端留空使用系统CA，可为一次HTTPS请求传入受信根证书PEM。Wasm端须留空，私有CA由宿主配置。
struct HttpRequestAPI HttpClientOptions
{
public:
    // 原生 HTTPS 本次信任的 CA PEM，空串选择系统信任库
    std::string m_authorityPem;
    // 完整 Range 值，例如 bytes=0-1023
    std::string m_range;
    // 原资源的强 ETag 或 HTTP 日期，用于 If-Range 校验
    std::string m_ifRange;
    // 条件请求的 If-None-Match 值；保留 ETag 自带的引号
    std::string m_ifNoneMatch;
    // 请求正文的媒体类型
    std::string m_contentType;
    // 单次传输的总超时毫秒数，取值 1 至 2147483647
    uint32_t m_timeoutMs;
#ifndef __EMSCRIPTEN__
    // 原生端可跟随的重定向次数，默认 8，0 拒绝重定向，-1 仅受本次总超时约束
    int32_t m_maxRedirects;
#endif
    // 可交付响应正文的最大字节数，必须大于零
    uint32_t m_maxResponseBytes;

public:
    /** 创建默认选项，超时 30000 毫秒，响应上限 16 MiB
    */
    HttpClientOptions();

    /** 设置一次传输的总超时，包括连接、重定向和接收。
    @param [in] milliseconds 毫秒，1 至 2147483647
    @return 合法时保存并返回 true，否则保留原值
    */
    bool setTimeoutMs(uint32_t milliseconds);
#ifndef __EMSCRIPTEN__
    /** 设置原生客户端允许跟随的重定向次数。
    @param [in] redirects 0 禁止重定向，正数限定次数，-1 取消次数限制
    @return 合法时保存并返回 true，否则保留原值
    */
    bool setMaxRedirects(int32_t redirects);
#endif

    /** 设置本次响应正文的字节上限
    @param [in] bytes 1 至 UINT32_MAX，-1 表示接受接口可表示的全部长度
    @return 合法时保存新值并返回 true
    */
    bool setMaxResponseBytes(int64_t bytes);
};

// m_error为空表示收到完整HTTP响应，包括304、404、416；正文允许包含零字节。
struct HttpRequestAPI HttpClientResponse
{
public:
    // 完整 HTTP 响应的状态码
    int32_t m_status;
    // 按字节交付，可含 NUL
    std::string m_body;
    // 可读取的 Content-Range 值
    std::string m_contentRange;
    // 可读取的 ETag 值，保留引号
    std::string m_etag;
    // 参数已接纳后发生的传输、证书验证、超时或响应上限错误
    std::string m_error;

public:
    /** 创建状态为零、文本和正文为空的响应
    */
    HttpClientResponse();
};

/** HTTP客户端。原生回调在后台线程执行，Wasm回调在宿主事件循环执行。
相同URL、方法、正文及选项的在途请求合并，每位调用方仍收到独立完成通知。
原生端可复用同一源和同一 CA 配置的空闲连接。每次重新配置方法、证书、头、正文与回调。
进程最多保留一条空闲连接。默认超过 64 KiB 的私有 CA PEM 只用于本次传输。
回调引用仅在通知期间有效；调用方负责被捕获对象的生命周期。
*/
class HttpRequestAPI HttpRequestClient
{
public:
    // 每位调用方收到一次完成通知；需要异步保存时复制响应
    typedef std::function<void(const HttpClientResponse&)> Completion;

    /** 在首个有效请求前设置进程的并发及原生连接缓存限制
    @param [in] concurrent 不同在途传输及原生工作线程数，默认 32，正数限制数量，-1 取消软件上限
    @param [in] cachedAuthorityBytes 可缓存的单份私有 CA 字节数，默认 65536，0 仅缓存系统信任，-1 取消此上限
    @return 已保存时为 true，参数无效、首个请求已接纳或原生客户端已关闭时返回 false
    */
    static bool setLimits(int32_t concurrent = 32, int64_t cachedAuthorityBytes = 65536);

#ifndef __EMSCRIPTEN__
    /** 原生客户端停止接纳任务并等待回调全部返回。卸载动态库前由所有者线程调用；
    回调内部不调用，关闭后该进程中的客户端不再接受请求。
    */
    static void shutdown();
#endif

    /** 异步发送 GET
    @param [in] url 带主机的 http 或 https 绝对地址，最多 8192 字节
    @param [in] options 按值保存的超时、证书、请求头和响应上限
    @param [in] completion 非空完成回调，引用的响应仅在调用期间有效
    @return true 表示已接纳，false 表示校验失败、原生工作线程启动失败、已关闭或并发数已满
    */
    static bool httpGet(const std::string& url, const HttpClientOptions& options, const Completion& completion);

    /** 异步发送 POST，生成与旧客户端相同的小写 MD5 请求标识
    @param [in] url 带主机的 http 或 https 绝对地址，最多 8192 字节
    @param [in] body 按长度复制的二进制正文，可含 NUL
    @param [in] options 按值保存的超时、证书、请求头和响应上限
    @param [in] completion 非空完成回调，引用的响应仅在调用期间有效
    @return 接纳时返回 MD5(url + body) 的 32 位小写文本，拒绝时返回空串
    */
    static std::string httpPost(const std::string& url, const std::string& body,
        const HttpClientOptions& options, const Completion& completion);
};