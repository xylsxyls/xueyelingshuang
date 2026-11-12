#pragma once
#include "LibuvTcp/LibuvTcpAPI.h"
#ifdef LIBUV_TEST_PUBLIC_DLL
#include "CTaskThreadManager/CTaskThreadManagerAPI.h"
#include <atomic>

// DLL消费者的专用网络任务，线程生命周期由CTaskThreadManager管理
class LibuvTcpDllTask : public CTask
{
public:
    /** 借用存活至任务线程回收的TCP实例
    @param [in,out] tcp 由测试主线程拥有的实例
    */
    explicit LibuvTcpDllTask(LibuvTcp& tcp);

    /** 在专用工作线程驱动真实DLL循环
    */
    virtual void doTask();

    /** 合作停止，仅发送停止请求，不回收宿主或等待自身
    */
    virtual void stopTask();

private:
    // 宿主必须覆盖任务及线程退出，本任务不拥有实例
    LibuvTcp& m_tcp;
};

// 只包含公开API的真实DLL发送者；网络循环与提交线程分离，检查DLL内TLS缓存及正文重载
class LibuvTcpDllSender : public LibuvTcp
{
public:
    /** 创建未连接的验证者
    */
    LibuvTcpDllSender();

    /** 通过DLL真实提交10000个独立小帧，交替普通与try的send/post
    @return 完整交付且线程和宿主均收敛为true
    */
    static bool run();

    /** 发布由DLL创建的当前连接
    @param [in] target 本实例连接，借用至断开
    */
    virtual void uvServerConnected(uv_tcp_t* target);

    /** 清除结束的连接借用
    @param [in] target 已断开的本实例连接
    */
    virtual void uvServerDisconnected(uv_tcp_t* target);

    /** 标记连接失败，解除测试等待
    */
    virtual void uvServerNotFind();

private:
    /** 接收原始长度帧，跨回调持续验证十字节帧模式
    @param [in,out] context 当前测试实例，借用至宿主销毁
    @param [in] peer 当前宿主连接号
    @param [in] bytes 本次借用字节
    @param [in] size 本次字节数
    */
    static void data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 宿主接纳与可写通知，本用例不通过宿主反向发送
    @param [in,out] context 当前测试实例
    @param [in] peer 宿主连接号
    */
    static void notify(void* context, uint64_t peer);

    /** 宿主关闭通知，资源由hostDestroy统一回收
    @param [in,out] context 当前测试实例
    @param [in] peer 宿主连接号
    @param [in] reason 关闭原因
    */
    static void closed(void* context, uint64_t peer, int32_t reason);

private:
    // DLL网络线程发布，仅在本次不主动断开的连接期用于提交
    std::atomic<uv_tcp_t*> m_target;
    // 网络线程与宿主校验的合并失败状态，只置位
    std::atomic<bool> m_failed;
    // 仅测试主线程驱动宿主接收和读取，累计实际字节数
    uint64_t m_received;
};
#endif

/** 只经公开 API 的 DLL/静态库调用验证，所有实例由同一执行者驱动
*/
class LibuvTcpPublicTest
{
public:
    /** 创建空观测器
    */
    LibuvTcpPublicTest();

    /** 配对释放库内实例
    */
    ~LibuvTcpPublicTest();

    /** 建立回环连接并逐字节验证真实交付，性能计时不包含建立和销毁
    @param [in] payload 单次写长度，1 至 32768
    @param [in] count 写入次数，1 至 1000000
    @param [out] milliseconds 从第一笔写入到最后一字节验证的毫秒数
    @return 0 完整交付并正常销毁，非零失败
    */
    static int run(uint32_t payload, uint32_t count, double& milliseconds);

private:
    /** 创建库内实例
    @return 是否成功
    */
    bool initialize();

    /** 记录唯一连接
    @param [in] context 当前观测器
    @param [in] peer 新连接号
    */
    static void connected(void* context, uint64_t peer);

    /** 验证连续字节，不假定 TCP 回调与发送一一对应
    @param [in] context 当前观测器
    @param [in] peer 当前连接号
    @param [in] bytes 借用缓冲
    @param [in] size 本次字节数
    */
    static void data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 验证完成通知的连接归属
    @param [in] context 当前观测器
    @param [in] peer 当前连接号
    */
    static void writable(void* context, uint64_t peer);

    /** 记录唯一终态
    @param [in] context 当前观测器
    @param [in] peer 当前连接号
    @param [in] reason 关闭原因
    */
    static void closed(void* context, uint64_t peer, int32_t reason);

private:
    // 成功初始化后由本观测器拥有
    void* m_instance;
    // 当前连接号
    uint64_t m_peer;
    // 已校验的连续字节数
    uint64_t m_received;
    // 连接建立次数
    uint32_t m_connected;
    // 终态通知次数
    uint32_t m_closed;
    // 校验失败后保持置位
    bool m_failed;
};