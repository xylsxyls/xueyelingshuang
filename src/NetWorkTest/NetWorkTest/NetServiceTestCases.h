#pragma once
#include "NetSenderTestClient.h"
#ifdef _WIN32
#include "NetSender/NetSenderAPI.h"
#include "Semaphore/SemaphoreAPI.h"
#include <windows.h>

// 两级业务与Account请求的实际服务观察者
class NetServiceTestProbe : public NetSenderServiceListener
{
public:
    /** 准备一个独立测试服务
    @param [in] product 0账号或4业务
    @param [in] ready 父进程的就绪信号
    */
    NetServiceTestProbe(uint32_t product, Semaphore& ready);

    /** 信号通知认证已完成
    @param [in] endpoint 服务代次
    */
    virtual void onServiceReady(uint64_t endpoint);

    /** 业务服务发B，账号服务回B，业务服务再回A
    @param [in] frame 原始RPC
    @param [in] origin 可信转发来源
    @return 消费或背压
    */
    virtual bool onServiceFrame(const NetInterfaceFrame& frame, const NetInterfaceOrigin& origin);

    /** 记录终止
    @param [in] reason 原因
    */
    virtual void onServiceClosed(int32_t reason);

public:
    // 本用例服务接入对象，生命周期覆盖服务回调与协议推进
    NetSenderService* m_service;
    // 测试子进程已完成真实认证的通知信号
    Semaphore* m_readySignal;
    // 本用例固定的服务产品号，不从回包中反推期望值
    uint32_t m_product;
    // 独立设置的客户端平台期望值，不从输入生成期望
    uint32_t m_expectedPlatform;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    bool m_closed;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    bool m_bad;
    // 本轮存在尚未完成的根请求与内部依赖关联
    bool m_pending;
    // 本次投递的可信根来源副本，用于核对来源隔离
    NetInterfaceOrigin m_origin;
    // 根请求标量副本，内部子响应回程时还原原请求身份
    NetInterfaceFrame m_outer;
    // 已发起的Account子请求号，不能与根请求号混用
    uint64_t m_accountChild;
};

// 独立服务进程内的IPC、探针及服务接入资源拥有者
class NetServiceTestHost
{
public:
    /** 拥有子进程收件箱及原ProcessWork生命周期
    @param [in] product 产品
    @param [in] ready 认证信号
    */
    NetServiceTestHost(uint32_t product, Semaphore& ready);

    /** 停止本地协议后再停ProcessWork线程
    */
    ~NetServiceTestHost();

public:
    // 本用例拥有的IPC收件箱，接收回调收敛后才可销毁
    NetSenderLocalInbox m_inbox;
    // 本用例的事件观察者，须比被观察运输更早创建、更晚销毁
    NetServiceTestProbe m_probe;
    // 本用例服务接入对象，生命周期覆盖服务回调与协议推进
    NetSenderService m_service;
};

// 仅拥有本用例启动的服务子进程及其停止通知
class NetServiceTestChild
{
public:
    /** 启动本用例拥有的隐藏独立服务
    @param [in] product 服务产品
    @param [in] mode 1有效，2错误凭据，3重复产品
    */
    NetServiceTestChild(uint32_t product, uint32_t mode);

    /** 尽力正常停止；失败只清理本实例拥有的子进程
    */
    ~NetServiceTestChild();

    /** 接纳认证完成信号
    @return 是否完成，之后保持true
    */
    bool ready();

    /** 读取进程状态
    @return STILL_ACTIVE或退出码
    */
    uint32_t exitCode() const;

    /** 发停止信号并等待子进程正常退出
    @return 正常退出0
    */
    uint32_t stop();

private:
    // 本用例启动并拥有的进程句柄，禁止据此清理其他业务进程
    HANDLE m_process;
    // 测试子进程已完成真实认证的通知信号
    Semaphore m_readySignal;
    // 拥有者请求本测试子进程正常停止的通知信号
    Semaphore m_stopSignal;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
};

// 多进程业务与内部服务测试集合，逐用例独立建立预期并检查实际结果
class NetServiceTestCases
{
public:
    /** 两个独立服务经实际ProcessWork和TLS完成完整A/B回程及满容量消息
    */
    static void nestedProcesses();

    /** 错误凭据和重复产品均不得注册或替换在线服务
    */
    static void rejectedServices();

    /** 子进程内部入口
    @param [in] parent 网关PID
    @param [in] product 0或4
    @param [in] mode 1有效，2错误，3重复
    @return 测试结果
    */
    static int childMain(int32_t parent, uint32_t product, uint32_t mode);

    /** 外部真实网关上的测试服务，固定10秒受限运行；不提供生产默认凭据
    @param [in] gatewayPid 真实网关
    @param [in] product 0或4
    @return 正常0
    */
    static int standaloneMain(int32_t gatewayPid, uint32_t product);

    /** 实际桌面业务客户端，经IPC和NetClientManager完成端到端往返
    @param [in] gatewayPid 实际NetClientManager
    @return 0仅完整二进制结果及有序停止通过
    */
    static int desktopClientMain(int32_t gatewayPid);
};

// 通过桌面Sender完成端到端业务往返的独立预期观察者
class NetDesktopClientProbe : public NetSessionListener
{
public:
    /** 初始化测试监听，不拥有session
    */
    NetDesktopClientProbe();

    /** 发起唯一基础请求
    @param [in] generation 代次
    @param [in] maxRequests 额度
    @param [in] maxChunk 块大小
    */
    virtual void onSessionReady(uint64_t generation, uint32_t maxRequests, uint32_t maxChunk);

    /** 对照独立预期字节
    @param [in] generation 代次
    @param [in] requestId 原编号
    @param [in] product 产品
    @param [in] type 类型
    @param [in] status 结果
    @param [in] data 借用体
    @param [in] size 长度
    */
    virtual void onSessionResult(uint64_t generation, uint64_t requestId, uint32_t product,
        uint32_t type, uint32_t status, const uint8_t* data, uint32_t size);

    /** 非预期推送记录错误
    @param [in] generation 代次
    @param [in] frame 帧
    @return 已消费
    */
    virtual bool onSessionFrame(uint64_t generation, const NetInterfaceFrame& frame);

    /** 记录关闭
    @param [in] generation 代次
    @param [in] reason 原因
    @param [in] detail 诊断
    */
    virtual void onSessionClosed(uint64_t generation, int32_t reason, int32_t detail);

public:
    // 借用本用例Sender，关闭及回调收敛之前保持有效
    NetSenderTestClient<NetSender, NetSenderClient>* m_sender;
    // 本轮实际分配的请求号，回包必须精确匹配
    uint64_t m_id;
    // 实际收到的请求终态次数，用于检查一请求一终态
    uint32_t m_results;
    // 实际关闭通知次数，用于检查唯一终态
    uint32_t m_closes;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    bool m_bad;
};
#endif