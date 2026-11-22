#pragma once
#include "LibuvTcp/LibuvTcpAPI.h"
#include <atomic>
#include <thread>
#include <vector>

/** 公开三/四参数post的同条件真实TCP基准；收端按字节流独立验收，测试状态不进入生产库
*/
class LibuvRawPostBenchmark : public LibuvTcp
{
public:
    /** 创建一个未启动端点
    */
    LibuvRawPostBenchmark();

    /** 停止并回收自己拥有的网络线程及宿主
    */
    ~LibuvRawPostBenchmark();

    /** 执行预热和单向交付计时，不把入队耗时当成交付耗时
    @param [in] argc 6个参数
    @param [in] argv --hostbenchmark 生产者数 正文长度 总条数 raw/framed/framedpool/ordered
    @return 0校验和回收成功，非零失败
    */
    static int run(int argc, char** argv);

    /** 发布普通三参数post的目标；借用覆盖本轮所有生产者
    @param [in] target 已连接TCP对象
    */
    virtual void uvServerConnected(uv_tcp_t* target);

    /** 标记异常连接失败
    */
    virtual void uvServerNotFind();

private:
    /** 创建并在本线程驱动宿主；监听使用端口0，连接使用已发布监听端口
    @param [in] port 0监听，非零连接
    */
    void network(uint32_t port);

    /** 发布本端连接号
    @param [in] context 当前端点
    @param [in] peer 连接号
    */
    static void connected(void* context, uint64_t peer);

    /** 校验分片/粘连的正文及可选生产者序号，完成时间在最后一字节校验后取得
    @param [in] context 接收端点
    @param [in] peer 当前连接
    @param [in] bytes 借用字节
    @param [in] size 字节数
    */
    static void data(void* context, uint64_t peer, const uint8_t* bytes, uint32_t size);

    /** 接收写完成通知；不充当远端收到的证据
    @param [in] context 当前端点
    @param [in] peer 当前连接
    */
    static void writable(void* context, uint64_t peer);

    /** 计时期间意外断开记为失败
    @param [in] context 当前端点
    @param [in] peer 关闭连接
    @param [in] reason 关闭原因
    */
    static void closed(void* context, uint64_t peer, int32_t reason);

private:
    // 网络线程创建并销毁；m_peer发布后生产者只借用到全部join
    void* m_host;
    // 网络线程拥有的事件循环
    void* m_loop;
    // 自有线程，析构前必须回收
    std::thread m_network;
    // 原始连接号兼作宿主发布屏障
    std::atomic<uint64_t> m_peer;
    // 三参数接口的已连接目标
    std::atomic<uv_tcp_t*> m_target;
    // 监听端口发布屏障
    std::atomic<uint32_t> m_port;
    // 跨线程合作停止标记
    std::atomic<bool> m_stop;
    // 任一校验或库调用失败只置位
    std::atomic<bool> m_failed;
    // 接收回调按块发布已验收总条数
    std::atomic<uint64_t> m_received;
    // 最后一字节验证后的单调时钟纳秒值
    std::atomic<uint64_t> m_finished;
    // 包含预热的预期总条数
    uint64_t m_expected;
    // 使用六字节生产者编号和序号审计模式
    bool m_ordered;
    // 跨TCP回调保存的一帧，回调线程独占
    std::vector<uint8_t> m_partial;
    // 固定正文期望帧，回调线程只读
    std::vector<uint8_t> m_pattern;
    // 当前半帧已收到字节数
    uint32_t m_offset;
    // 审计模式每生产者独立序号，回调线程独占
    uint32_t m_sequences[16];
};