#pragma once
#include <stdint.h>

class NetSenderSubmission;
struct NetInterfaceFrame;

// 通过公开入口验证紧凑头，预期由独立黄金字节和字段矩阵给出
class NetCompactTest
{
public:
    /** 验证黄金字节、任意切分、混合帧、规范整数、容量与失败原子性
    */
    static void run();

    /** 验证能力交集、真实TLS大小帧及单槽跨容量类复用
    */
    static void tls();

    /** 验证短帧入口容量、借用退出、批次复用及单/多生产者FIFO；输出不含TLS的提交层耗时
    */
    static void submission();

    /** 通过批量借用入口逐条校验并发DATA，独立记录不含TLS的吞吐
    */
    static void drainConcurrent();
};

// 唯一消费者持有的逐生产者顺序校验，不把测试共享原子计数加入热路径
struct NetCompactSequenceProbe
{
public:
    // 生产者数量，1或4
    uint32_t m_producers;
    // 本轮正文长度，只使用0或6
    uint32_t m_bodySize;
    // 各生产者已校验的连续序号
    uint32_t m_received[4];
public:
    /** 清空独立校验状态
    @param [in] producers 生产者数量，1或4
    @param [in] bodySize 正文长度，0或6
    */
    NetCompactSequenceProbe(uint32_t producers, uint32_t bodySize);

    /** 校验配置长度的正文、身份和逐线程顺序
    @param [in,out] context 唯一消费者拥有的探针
    @param [in] frame 本次回调期间借用的真实出队帧
    @return NI_OK或NI_PROTOCOL_ERROR，不修改被测库状态
    */
    static int32_t consume(void* context, const NetInterfaceFrame& frame);
};

// 提交层前缀消费探针；只在测试工程中制造背压、异常及借用中的关闭
struct NetCompactDrainProbe
{
public:
    // 测试拥有的提交层，借用至consume返回
    NetSenderSubmission* m_submission;
    // 消费方返回的接纳结果
    int32_t m_result;
    // 总回调次数，不等同于成功接纳次数
    uint32_t m_calls;
    // 在当前正文仍被借用时关闭连接
    bool m_close;
    // 模拟外部回调抛出异常
    bool m_throw;
public:
    /** 创建接受正文的测试消费方
    @param [in] submission 由测试持有的提交层
    */
    explicit NetCompactDrainProbe(NetSenderSubmission* submission);

    /** 验证独立正文并返回配置结果，允许测试关闭/异常路径
    @param [in] context 本测试探针
    @param [in] frame 借用帧，关闭后也必须保持至本回调返回
    @return 配置的NI结果
    */
    static int32_t consume(void* context, const NetInterfaceFrame& frame);
};