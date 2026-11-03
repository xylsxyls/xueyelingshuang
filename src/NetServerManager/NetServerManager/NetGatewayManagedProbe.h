#pragma once
#include "NetSender/NetSenderAPI.h"
#include <deque>

// 固定的两级RPC测试宿主；正常网关运行分支不创建业务端点。
class NetGatewayManagedProbe : private NetSenderServiceListener
{
public:
    /** 初始化受控服务状态，不注册端点或创建线程
    */
    NetGatewayManagedProbe();

    /** 运行独立的两级服务测试宿主，正常网关不进入此路径
    @param [in] pid 已启动的隔离网关PID
    @param [in] stopFile 测试独占停止文件路径
    @param [in] duration 最长运行时间，1至60000毫秒
    @param [in] internal 是否扮演公共内部服务
    @param [in] capacity 是否启用容量限制场景
    @return 0正常结束，非零参数或测试失败
    */
    static int run(int32_t pid, const std::string& stopFile, uint32_t duration, bool internal, bool capacity = false);

private:
    /** 记录服务真实注册就绪，并检查未知客户端事件被拒绝
    @param [in] endpoint 网关已分配的逻辑端点号
    */
    virtual void onServiceReady(uint64_t endpoint);

    /** 执行受控业务回显、内部代理或人工背压场景
    @param [in] frame 本次调用期间借用的完整帧，背压时由调用方保留
    @param [in] origin 本次调用期间借用的网关可信来源
    @return true已消费，false表示本次未消费并保持背压
    */
    virtual bool onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 清理测试代理并记录异常服务终态
    @param [in] reason 本连接或服务的NI关闭原因
    */
    virtual void onServiceClosed(int32_t reason);

    /** 编码与原请求绑定的响应，并交给实际服务发送入口
    @param [in] request 本次同步调用期间借用的请求，保留原请求号及路由标记
    @param [in] status RPC终态状态，不等同于底层发送返回值
    @param [in] data 本次同步调用期间借用的正文
    @param [in] size 正文字节数，不能用C字符串长度代替
    @return NI_OK已交给服务发送入口，NI_LIMIT_EXCEEDED表示未接纳
    */
    int32_t reply(const NetInterfaceFrame& request, uint32_t status, const uint8_t* data, uint32_t size);

    /** 推进测试代理及有界定向事件队列，背压时保留待发送项
    */
    void pumpEvents();

    // 本用例服务接入对象，生命周期覆盖服务回调与协议推进
    NetSenderService* m_service;
    // 借用实际公共聚合代理，生命周期覆盖测试回调和poll。
    NetSenderServiceProxy* m_proxy;
    // 内部服务等待发送定向事件的可信来源队列，最多128项
    std::deque<NetInterfaceOrigin> m_events;
    // 测试宿主独占的背压控制文件路径
    std::string m_holdFile;
    // 本轮已经报告人为背压，避免重复打印控制信号
    bool m_held;
    // true扮演公共内部服务，false扮演产品业务服务
    bool m_internal;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
    // 本用例首次失败后保留的错误标志，后续成功不能清除
    bool m_failed;
};