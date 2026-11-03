#pragma once
#include "NetPeer.h"
#include "NetHostSocketTest.h"
#include <vector>

// 只观察实际交给宿主的字节；不实现待发队列或被测协议
class NetRelayTransportTestDriver : public NetHostSocketTestDriver
{
public:
    /** 默认捕获原始输出，压力测试可关闭捕获
    */
    NetRelayTransportTestDriver();

    /** 在既有确定性宿主上替换write观察入口
    @return 借用本对象的宿主表
    */
    NetHostFunctions relayFunctions();

    /** 完整接纳时复制原始输出；背压/失败不得复制
    @param [in] instance 测试宿主
    @param [in] peer 代次
    @param [in] data 原始字节
    @param [in] size 字节数
    @return 指定宿主结果
    */
    static int32_t capture(void* instance, uint64_t peer, const uint8_t* data, uint32_t size);

public:
    // 是否把真实write的字节保存在独立观察缓冲
    bool m_capture;
    // 按真实write顺序拼接的字节
    std::vector<uint8_t> m_wire;
};

// 上层观察者，可注入背压、异常和回调内停止
class NetRelayTransportTestListener : public NetRelayTransportListener
{
public:
    /** 创建无注入的观察者
    */
    NetRelayTransportTestListener();

    /** 记录建立并按mode注入；同时检查递归poll不重复通知
    @param [in] peer 当前代次
    */
    virtual void onRelayConnected(uint64_t peer);

    /** 保存接纳记录载荷；拒绝阶段只检查重复借用保持不变
    @param [in] peer 当前代次
    @param [in] record 完整记录
    @return 是否接纳
    */
    virtual bool onRelayRecord(uint64_t peer, const NetInterfaceTunnelRecord& record);

    /** 检查回调前代次已失效，并按mode重入关闭或抛异常
    @param [in] peer 已结束代次
    @param [in] reason 首个原因
    */
    virtual void onRelayClosed(uint64_t peer, int32_t reason);

public:
    // 借用被测对象直到停止
    NetRelayTransport* m_relay;
    // 0正常，1/2/3连接时停止/关闭/异常，4/5/6记录时停止/关闭/异常，7终态异常
    uint32_t m_mode;
    // 还需拒绝当前记录的次数
    uint32_t m_reject;
    // 连接回调次数
    uint32_t m_connected;
    // 接纳记录次数
    uint32_t m_records;
    // 记录回调总次数，包括背压重试
    uint32_t m_attempts;
    // 终态次数
    uint32_t m_closed;
    // 最终NI原因
    int32_t m_reason;
    // 重入检查和重复借用比较均通过
    bool m_valid;
    // 第一次拒绝时的独立副本
    std::vector<uint8_t> m_rejected;
    // 按接纳顺序保存的载荷
    std::vector<uint8_t> m_received;
};