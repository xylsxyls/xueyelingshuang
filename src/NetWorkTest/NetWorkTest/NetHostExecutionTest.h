#pragma once
#include <stdint.h>

/** 共同宿主循环的黑盒用例；验证多个通道、空闲、唤醒与回收，不向生产库嵌入测试
*/
class NetHostExecutionTest
{
public:
    /** 执行事件等待与真实回环TCP用例
    @return 0通过，非零为失败步骤
    */
    static int run();

    /** 接入统一测试runner
    失败抛出测试异常
    */
    static void check();

    /** 记录连接号
    @param [in] context 测试接收者
    @param [in] peer 当前通道连接号
    */
    static void connected(void* context, uint64_t peer);

    /** 累计实际收到的字节数
    @param [in] context 测试接收者
    @param [in] peer 当前通道连接号
    @param [in] bytes 回调期间借用的正文
    @param [in] size 本次接收字节数
    */
    static void data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 记录写完成
    @param [in] context 测试接收者
    @param [in] peer 当前通道连接号
    */
    static void writable(void* context, uint64_t peer);

    /** 记录通道终态
    @param [in] context 测试接收者
    @param [in] peer 当前通道连接号
    @param [in] reason 底层关闭原因
    */
    static void closed(void* context, uint64_t peer, int32_t reason);

    /** 初始化观测值
    */
    NetHostExecutionTest();

public:
    // 最后接纳的连接号，仅共同执行者访问
    uint64_t m_peer;
    // 实际接收字节数，按TCP流校验固定正文
    uint32_t m_received;
    // 关闭次数
    uint32_t m_closed;
    // 数据错误或回调中错误接纳重入操作
    bool m_failed;
    // 当前共同循环，回调中验证拒绝嵌套wait和destroy
    void* m_loop;
};