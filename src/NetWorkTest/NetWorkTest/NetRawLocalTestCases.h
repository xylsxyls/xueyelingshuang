#pragma once
#ifndef __EMSCRIPTEN__
#include "NetSender/NetSenderAPI.h"
#include <atomic>

// 只在测试中延迟已提交END的回调交付，制造对端先退出的确定性交错
class NetRawLocalTestInbox : public NetSenderLocalInbox
{
public:
    /** 初始化受控END闸门，不创建线程
    @param [in] gateEnd 是否在END回调中等待测试owner放行
    @param [in] dropEnd 是否在已完成往返后故意不交付END
    */
    explicit NetRawLocalTestInbox(bool gateEnd, bool dropEnd);

    /** 借用原回调数据；END最多等待两秒，随后仍交给正式收件箱
    @param [in] sendPid 实际发送PID
    @param [in] buffer 回调期间借用的字节
    @param [in] length 正文字节数
    @param [in] type 原ProcessWork消息类型
    */
    virtual void receive(int32_t sendPid, const char* buffer, int32_t length, MessageType type);

public:
    // 构造后不变，只由本测试的原ProcessWork回调读取
    bool m_gateEnd;
    // 构造后不变，只丢本测试的END，不改ProcessWork共享提交
    bool m_dropEnd;
    // 回调已停在END入口，由回调发布给owner
    std::atomic<bool> m_waiting;
    // owner允许回调继续，关闭前也必须放行
    std::atomic<bool> m_release;
};

// 透明本机IPC记录观察者，独立核对顺序和完整密文字节
class NetRawLocalTestProbe : public NetSenderLocalRawListener
{
public:
    /** 初始化真实进程宿主的断言状态
    @param [in] server 是否回显端
    */
    explicit NetRawLocalTestProbe(bool server);

    /** 记录真实IPC对端并发起本用例的透明记录往返
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] pid 当前已验证的本机对端PID
    */
    virtual void onRawLocalReady(uint64_t peer, int32_t pid);

    /** 检查完整密文字节和顺序，按用例返回背压或消费结果
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] record 回调期间借用的完整透明记录，背压时不消费
    @return true已消费；false让调用方保留当前记录重试
    */
    virtual bool onRawLocalRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 记录真实IPC的终态次数及原因，检查不会重复回调
    @param [in] peer 本次实际运输连接号，不按PID或句柄地址推算
    @param [in] reason 本连接或服务的NI关闭原因
    */
    virtual void onRawLocalClosed(uint64_t peer, int32_t reason);

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
    // 最近本地关闭原因，用于区分尾包丢失与测试背压条件失败。
    int32_t m_reason;
    // 本场景独立预期的终态原因
    int32_t m_expectedReason;
    // 本探针是否扮演接收服务，客户端使用同一协议的相反方向
    bool m_server;
    // 本用例观察到的实际就绪状态，不代替业务响应成功
    bool m_ready;
    // 已收到预期的首个透明握手记录，防止重复接受
    bool m_hello;
    // 输入、顺序、线程或回调次数不符合用例独立预期时置位
    bool m_bad;
};

// 透明本机IPC测试资源拥有者，回调目标存活到接收器停止
class NetRawLocalTestHost
{
public:
    /** 准备原ProcessWork收件箱，不修改原实现
    @param [in] server 是否回显端
    @param [in] gateEnd 是否暂缓END回调
    @param [in] dropEnd 是否跳过本测试END交付
    */
    explicit NetRawLocalTestHost(bool server, bool gateEnd = false, bool dropEnd = false);

    /** 先收敛本地传输，再停止旧接收回调
    */
    ~NetRawLocalTestHost();

public:
    // 本用例拥有的IPC收件箱，接收回调收敛后才可销毁
    NetRawLocalTestInbox m_inbox;
    // 本用例的事件观察者，须比被观察运输更早创建、更晚销毁
    NetRawLocalTestProbe m_probe;
    // 本用例的本机运输，停止后才能销毁其监听者和收件箱
    NetSenderLocal m_local;
};

// 透明IPC记录和窗口测试集合，逐用例独立建立预期并检查实际结果
class NetRawLocalTestCases
{
public:
#ifdef _WIN32
    /** 两个真实进程发送完整16KiB记录，含分片/背压/完整64位号/唯一关闭
    */
    static void transfer();

    /** 对端退出后仍保留已提交尾包，END回调延迟期间不误报超时
    */
    static void delayedEnd();

    /** 完整数据之后缺失END必须在五秒排空期限结束，不能误报正常或无限等待
    */
    static void missingEnd();

    /** 共用实际双进程测试，完整核对正文和唯一正常关闭
    @param [in] mode 0正常、1延迟END、2丢弃END
    */
    static void runTransfer(uint32_t mode);

    /** 独占测试子进程入口
    @param [in] parent 父进程PID
    @return 0通过，否则失败
    */
    static int childMain(int32_t parent);
#endif
};
#endif