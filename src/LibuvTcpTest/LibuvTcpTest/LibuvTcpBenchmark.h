#pragma once
#include <stdint.h>
#include "uv.h"
#include "LibuvTcp/LibuvTcpAPI.h"
#include <atomic>

class LibuvTcp;

/** 可选白盒诊断定时器，仅在 --probe 模式由测试 owner 使用
*/
struct LibuvTcpBenchmarkProbe
{
public:
    // 由测试持有至循环完全排空
    uv_timer_t m_timer;
    // 借用当前测试客户端
    LibuvTcp* m_client;
    // 本次观测起点
    uint64_t m_started;
    // 最多补一次诊断唤醒，不能把修复唤醒的运行统计成正常通过
    bool m_notified;

    /** 在 owner 回调中记录停滞状态并补发一次诊断唤醒
    @param [in] timer 本测试拥有的定时器
    */
    static void tick(uv_timer_t* timer);
};

/** 同输入实际交付基准；不把发送入队数当作吞吐量
*/
class LibuvTcpBenchmark
{
public:
    /** 独立测量单条提交各步骤的摊销成本；不将局部结果当作端到端吞吐
    @return 0全部操作成功，非零资源或校验失败
    */
    static int profile();

    /** 建立真实回环 TCP，完成内容和单生产者顺序校验后输出 CSV 指标
    @param [in] producers 并发生产者数，1 至 16
    @param [in] payload 正文字节数，16 至 65536
    @param [in] count 每生产者帧数，1 至 1000000
    @param [in] window 每生产者未接收窗口，1 至 128
    @param [in] probe 是否启用独立诊断；补发过唤醒的运行返回失败，不计正常性能
    @return 0 完整交付，非零配置或运行失败
    */
    static int run(uint32_t producers, uint32_t payload, uint32_t count, uint32_t window, bool probe = false);

    /** 完整交付六字节正文；独立于带时间戳和小窗口的延迟基准
    @param [in] producers 生产者数，1至16
    @param [in] ordered 0逐帧核对123456，1核对生产者和序号，2另混用单帧与批量入口
    @param [in] count 每生产者帧数，1至200000000
    @param [in] batch 每次提交帧数，1使用原单帧入口，2至4096使用sendBatch
    @param [in] congested 是否限制系统发送缓冲并延迟启动接收，覆盖部分写和背压
    @param [in] copy 是否逐条调用借用正文重载，独立报告而不混充原数组入口
    @param [in] post 是否使用配对post入口；单条比较仍要求batch为1
    @return 0全部逐帧校验通过，非零失败
    */
    static int tiny(uint32_t producers, uint32_t ordered, uint32_t count, uint32_t batch, bool congested = false,
        bool copy = false, bool post = false);
};

/** 六字节基准的单循环接收器；只在回调末发布计数，不用每帧原子计数和计时干扰吞吐
*/
class LibuvTinyReceiver : public LibuvTcp
{
public:
    /** 固定完整帧数量和校验方式
    @param [in] expected 必须完整接收的帧数
    @param [in] ordered 是否核对生产者序号
    @param [in] producers 有效生产者数
    */
    LibuvTinyReceiver(uint64_t expected, bool ordered, uint32_t producers);

    /** 校验TCP拆包和粘包后每帧的十个字节
    @param [in] sender 当前连接
    @param [in] buffer 仅本次回调内借用
    @param [in] length 实际字节数
    */
    virtual void receive(uv_tcp_t* sender, const char* buffer, int32_t length);

    /** 接入回调不输出日志
    @param [in] client 当前连接
    */
    virtual void uvClientConnected(uv_tcp_t* client);

    /** 断开回调不输出日志
    @param [in] client 已关闭连接
    */
    virtual void uvClientDisconnected(uv_tcp_t* client);

    // 回调末发布已经验证的帧数
    std::atomic<uint64_t> m_received;
    // 只在完整交付最后一帧时记录时间
    std::atomic<uint64_t> m_finished;
    // 内容或数量不匹配时置位
    std::atomic<bool> m_failed;
private:
    /** 核对一个完整帧，不保存输入借用
    @param [in] frame 十个有效字节
    @return 内容和序号是否正确
    */
    bool verify(const char* frame);
private:
    // 整轮固定的交付数
    uint64_t m_expected;
    // 是否进行顺序校验
    bool m_ordered;
    // 有效生产者数
    uint32_t m_producers;
    // owner专用生产者计数
    uint32_t m_sequences[16];
    // 仅保存跨回调的半帧
    char m_partial[10];
    // 半帧有效长度
    uint32_t m_partialSize;
};