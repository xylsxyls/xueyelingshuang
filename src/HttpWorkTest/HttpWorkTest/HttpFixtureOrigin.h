#pragma once
#include <atomic>
#include <condition_variable>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <stdint.h>

// 仅监听宿主回环的测试源站，持有监听套接字和固定数量的工作线程。
class HttpFixtureOrigin
{
public:
    /** 启动宿主测试源站
    @param [in] port 回环端口，0 表示向系统请求空闲端口
    @param [in] certificate 服务器证书 PEM，空串表示 HTTP
    @param [in] key 与本次测试证书配对的私钥 PEM
    */
    HttpFixtureOrigin(unsigned port, const std::string& certificate, const std::string& key);

    /** 等待工作者结束并关闭本次源站
    */
    ~HttpFixtureOrigin();

    /** 获取系统分配或调用方选定的端口
    @return 当前监听端口
    */
    unsigned port() const;

    /** 获取完成协议解析后抵达源站的 POST 数量
    @return 自启动以来收到的 POST 总数
    */
    unsigned posts() const;

private:
    /** 接收连接并由当前工作线程处理有界的请求响应
    */
    void serve();

    /** 按客户端测试契约生成完整 HTTP 响应
    @param [in] headers 请求行和头部
    @param [in] body 请求正文，允许 NUL
    @param [in] identity 当前 accept 分配的连接编号
    @return 将发回调用者的字节
    */
    std::string response(const std::string& headers, const std::string& body, uint64_t identity);

private:
    // 原生监听描述符，以无损的指针宽度跨系统保存
    uintptr_t m_listener;
    // 服务实际绑定的端口
    unsigned m_port;
    // TLS 服务器 PEM，空表示 HTTP
    std::string m_certificate;
    // 本次证书对应的临时私钥
    std::string m_key;
    // 工作者应退出的信号
    std::atomic<bool> m_stopping;
    // 完成协议解析后抵达源站的 POST 次数
    std::atomic<unsigned> m_posts;
    // 每次实际 accept 递增的端点连接号
    std::atomic<uint64_t> m_connections;
    // 保护 held/release 端点的同步状态
    std::mutex m_mutex;
    // 唤醒等待 held 响应的工作者
    std::condition_variable m_released;
    // 抵达 held 端点的请求数
    unsigned m_heldCount;
    // release 已允许返回 held 的响应
    bool m_open;
    // 本次源站独占的工作线程
    std::vector<std::thread> m_workers;
};