#pragma once
#ifndef __EMSCRIPTEN__
#include "NetSender/NetSenderAPI.h"
#include <vector>

// 真实本机TLS端点观察者，受控暂停输入验证窗口和顺序
class NetLocalTestProbe : public NetSenderLocalListener
{
public:
    /** 初始化独立测试端状态
    @param [in] server 是否回显服务端
    */
    explicit NetLocalTestProbe(bool server);

    /** 记录TLS就绪
    @param [in] peer 代次
    @param [in] pid 对端进程号
    */
    virtual void onLocalReady(uint64_t peer, int32_t pid);

    /** 验证每个实际跨进程负载；服务端回显并支持暂时背压
    @param [in] peer 代次
    @param [in] frame 完整帧
    @return 是否消费
    */
    virtual bool onLocalFrame(uint64_t peer, const NetInterfaceFrame& frame);

    /** 记录唯一终态
    @param [in] peer 代次
    @param [in] reason 结果
    */
    virtual void onLocalClosed(uint64_t peer, int32_t reason);

public:
    // 本用例的本机运输，停止后才能销毁其监听者和收件箱
    NetSenderLocal* m_local;
    // 当前用例使用的真实运输代次，不以PID替代
    uint64_t m_peer;
    // 人为背压释放的单调毫秒时刻，用于验证有界等待
    uint64_t m_holdUntil;
    // 本用例已消费的实际输入记录，用于检查内容与顺序
    uint32_t m_received;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    uint32_t m_closed;
    // 最近一次实际终态原因，用于核对失败分类
    int32_t m_reason;
    // 本探针是否扮演接收服务，客户端使用同一协议的相反方向
    bool m_server;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    bool m_bad;
    // 本用例工作已完成，拥有者仍须收敛运输或回收任务
    bool m_done;
};

// 本机TLS测试资源拥有者，先收敛协议再停止ProcessWork
class NetLocalTestHost
{
public:
    /** 创建独占ProcessWork收件箱，尚未建立TLS连接
    @param [in] server 是否服务器
    */
    explicit NetLocalTestHost(bool server);

    /** 先停传输再收敛ProcessWork回调
    */
    ~NetLocalTestHost();

public:
    // 本用例拥有的IPC收件箱，接收回调收敛后才可销毁
    NetSenderLocalInbox m_inbox;
    // 本用例的事件观察者，须比被观察运输更早创建、更晚销毁
    NetLocalTestProbe m_probe;
    // 本用例的本机运输，停止后才能销毁其监听者和收件箱
    NetSenderLocal m_local;
};

// 本机TLS和IPC窗口测试集合，逐用例独立建立预期并检查实际结果
class NetLocalTestCases
{
public:
#ifdef _WIN32
    /** 实际进程间TLS和一MiB负载、接收背压、完整64位编号
    */
    static void transfer();

    /** 错误信任根不得交付业务明文
    */
    static void certificateRejection();

    /** 对端突停后在固定确认期限内回收
    */
    static void peerExit();

    /** 隐藏测试子进程入口，不启用普通测试runner
    @param [in] parent 网关测试进程号
    @param [in] mode 1传输，2错误根，3等待强制退出
    @return 0成功，非零失败
    */
    static int childMain(int32_t parent, int32_t mode);
#endif
};
#endif