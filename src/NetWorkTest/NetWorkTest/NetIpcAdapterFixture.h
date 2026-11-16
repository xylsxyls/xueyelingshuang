#pragma once
#include "NetSenderTestAccess.h"

// 真实共享区与接收副本的测试所有者，断言失败也先回收副本和映射。
class NetIpcAdapterFixture
{
public:
    /** 建立空观察状态，尚未发布接收器
    */
    NetIpcAdapterFixture();

    /** 停止发送、撤销接收器，回收未被断言读取的副本
    */
    ~NetIpcAdapterFixture();

    /** 发布真实接收器，先通过一次空消息完成身份登记
    @param [in] slots 固定正文槽数，不允许增长
    */
    void open(int32_t slots);

    /** 验证实际副本的完整头和二进制正文，一条只取一次
    @param [in] bytes 独立预期正文，size为0时可空
    @param [in] size 预期正文长度
    @param [in] type 预期消息类型
    */
    void checkPacket(const uint8_t* bytes, int32_t size, int32_t type);
public:
    // 本用例独占接收器，pump在主线程进行，不与析构并发。
    ProcessWorkChannel m_receiver;
    // pump移交的副本，逐个由unique_ptr或析构释放。
    LockFreeQueue<char*> m_packets;
    // 接收通知，仅用作真实pump依赖，不模拟网络行为。
    Semaphore m_delivered;
    // 本次真实满容量观察。
    std::atomic<int32_t> m_full;
    // 被测网关IPC适配器。
    NetSenderIpcChannel m_sender;
};