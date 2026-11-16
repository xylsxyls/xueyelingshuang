#pragma once
#include "NetHostSocketTest.h"
#include "NetRelayTransportProbe.h"

// 单执行者宿主替身，观察真实运输层提交；不实现运输队列或编码算法
class NetRelayBatchTest : public NetHostSocketTestDriver
{
public:
    /** 临时选择确定性宿主；仅在测试调度器已收敛其他实例时使用
    */
    NetRelayBatchTest();

    /** 被测运输析构后恢复默认宿主
    */
    ~NetRelayBatchTest();

    /** 验证批量字节、FIFO、容量边界、拒绝原子性及失败终态
    */
    static void run();

    /** 记录完整接纳的字节，按每次和总容量模拟宿主背压
    @param [in] instance 借用本测试直到运输销毁
    @param [in] peer 当前真实连接号
    @param [in] data 借用到返回
    @param [in] size 本次提交字节数
    @return 0完整接纳、1完全拒绝、负数失败
    */
    static int32_t capture(void* instance, uint64_t peer, const uint8_t* data, uint32_t size);

    /** 提交带序号的记录并生成逐条编码参考，不复用被测合并逻辑
    @param [in,out] relay 实际运输层
    @param [in] peer 实际连接号
    @param [in] count 记录数
    @param [in] size 每条正文长度，至少4
    @param [in] mixed 是否交错控制记录
    */
    void enqueue(NetRelayTransport& relay, uint64_t peer, uint32_t count, uint32_t size, bool mixed);

public:
    // 测试前宿主表；测试结束无论成功失败都恢复
    NetHostFunctions m_previous;
    // 实际完整接纳的字节
    std::vector<uint8_t> m_wire;
    // 独立逐条编码的参考字节
    std::vector<uint8_t> m_expected;
    // 本场景实际宿主调用长度，包括被拒绝的调用
    std::vector<uint32_t> m_attempts;
    // 单次可接纳容量，默认32768
    uint32_t m_limit;
    // 累计尚可接纳的容量，默认UINT32_MAX
    uint32_t m_budget;
};