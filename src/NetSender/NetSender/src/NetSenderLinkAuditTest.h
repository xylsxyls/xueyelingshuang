#pragma once
#include "NetSender.h"
#include "NetSenderLinkState.h"
#include <atomic>

// 库内白盒回调观察者：只验证真实协议接纳后的IPC终态入口，不替代ProcessWork联调
class NetSenderLinkAuditListener : public NetLinkTransportListener
{
public:
    /** 初始化观察状态
    */
    NetSenderLinkAuditListener();

    /** 记录协议已认证；本测试先直接驱动实际TLS准备此状态
    @param [in] peer 代次
    @param [in] identity 认证身份
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 检查收到的完整管理正文
    @param [in] peer 代次
    @param [in] frame 完整帧
    @return true已接纳
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 检查密文正文并按模式注入背压或主动关闭
    @param [in] peer 代次
    @param [in] record 完整记录
    @return 是否接纳
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 核对回调前代次已移除
    @param [in] peer 结束代次
    @param [in] reason 最终NI原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

public:
    // 借用被测私有状态；只在注释main的白盒测试中使用
    NetSenderLinkState* m_state;
    // 测试场景
    uint32_t m_mode;
    // 记录回调次数，包含背压重试
    uint32_t m_attempts;
    // 实际接纳的业务记录数
    uint32_t m_records;
    // 实际接纳的管理帧数
    uint32_t m_controls;
    // 关闭通知数
    uint32_t m_closed;
    // 最终NI原因
    int32_t m_reason;
    // 内容及回调顺序是否正确
    bool m_valid;
};

// 独立测试进程的服务注册探针；仅由注释main和NetWorkTest定义，不进入正式库导出。
class NetSenderGrantAudit : public NetLinkTransportListener
{
public:
    /** 初始化单执行者探针
    */
    NetSenderGrantAudit();

    /** 检查唯一认证完成回调
    @param [in] peer 本次代次
    @param [in] identity 已验证的节点身份
    */
    virtual void onLinkReady(uint64_t peer, const NetInterfaceLinkPeer& identity);

    /** 接纳测试期间的管理消息
    @param [in] peer 本次代次
    @param [in] frame 完整管理帧
    @return true已消费
    */
    virtual bool onLinkControl(uint64_t peer, const NetInterfaceFrame& frame);

    /** 未注册业务通道收到数据视为测试失败
    @param [in] peer 代次
    @param [in] record 数据记录
    @return true已消费
    */
    virtual bool onLinkRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 记录唯一结束通知
    @param [in] peer 本次代次
    @param [in] reason 关闭原因
    */
    virtual void onLinkClosed(uint64_t peer, int32_t reason);

    /** 真实IPC注册后注入当前执行者的证书复制分配失败，再验证重试和关闭
    @param [in] pid 测试宿主创建的隔离NetServerManager PID
    @param [in] proof 测试服务端程序提供的注册证明，不由客户端SDK提供
    @return 0通过，非0失败；独立进程执行，拥有本次ProcessWork启动与关闭
    */
    static int run(int32_t pid, NetInterfaceLinkServiceProof proof);

private:
#if defined(_WIN32) && defined(_DEBUG)
    /** CRT故障钩子只拒绝测试执行者的普通堆，不干扰IPC工作线程
    @param [in] type CRT操作
    @param [in] data CRT传入的旧地址
    @param [in] size 请求字节数
    @param [in] block CRT块类型
    @param [in] request CRT请求号
    @param [in] file 来源文件
    @param [in] line 来源行
    @return 0拒绝分配，1允许
    */
    static int allocation(int type, void* data, size_t size, int block, long request,
        const unsigned char* file, int line);
    // 钩子在任意线程调用，原子发布目标执行者；计数只由目标执行者读写。
    static std::atomic<uint32_t> m_thread;
    // 普通堆分配故障注入倒计数，负数关闭注入
    static int m_failAfter;
#endif
    // 回调状态仅由本测试的poll执行者访问。
    bool m_ready;
    // 本用例观察到的终态记录，用于检查关闭不丢失且不重复
    bool m_closed;
    // 本轮管理链路的校验结果，失败后不能报告通过
    bool m_valid;
    // 当前用例使用的真实运输代次，不以PID替代
    uint64_t m_peer;
};